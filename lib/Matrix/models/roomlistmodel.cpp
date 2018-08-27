/**************************************************************************
 *                                                                        *
 * Copyright (C) 2016 Felix Rohrbach <kde@fxrh.de>                        *
 *                                                                        *
 * This program is free software; you can redistribute it and/or          *
 * modify it under the terms of the GNU General Public License            *
 * as published by the Free Software Foundation; either version 3         *
 * of the License, or (at your option) any later version.                 *
 *                                                                        *
 * This program is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 *                                                                        *
 **************************************************************************/

#include "roomlistmodel.h"

#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtCore/QDebug>
#include <QtGui/QIcon>

#include "libqmatrixclient/lib/connection.h"
#include "libqmatrixclient/lib/room.h"
#include "libqmatrixclient/lib/events/callinviteevent.h"
#include "libqmatrixclient/lib/events/callcandidatesevent.h"
#include "libqmatrixclient/lib/events/callanswerevent.h"
#include "libqmatrixclient/lib/events/callhangupevent.h"
#include "libqmatrixclient/lib/logging.h"

using namespace QMatrixClient;

const int RoomEventStateRole = Qt::UserRole + 1;

RoomListModel::RoomListModel(QObject* parent)
    : QAbstractListModel(parent)
{ }

void RoomListModel::addConnection(QMatrixClient::Connection* connection)
{
    Q_ASSERT(connection);

    using QMatrixClient::Room;
    beginResetModel();
    m_connections.push_back(connection);
    connect( connection, &QMatrixClient::Connection::loggedOut,
             this, [=]{ deleteConnection(connection); } );
    connect( connection, &QMatrixClient::Connection::invitedRoom,
             this, &RoomListModel::updateRoom);
    connect( connection, &QMatrixClient::Connection::joinedRoom,
             this, &RoomListModel::updateRoom);
    connect( connection, &QMatrixClient::Connection::leftRoom,
             this, &RoomListModel::updateRoom);
    connect( connection, &QMatrixClient::Connection::aboutToDeleteRoom,
             this, &RoomListModel::deleteRoom);

    for( auto r: connection->roomMap() )
        doAddRoom(r);
    endResetModel();
}

void RoomListModel::deleteConnection(QMatrixClient::Connection* connection)
{
    Q_ASSERT(connection);

    // TODO: Save selection
    beginResetModel();
    connection->disconnect(this);
    for( QMatrixClient::Room* room: m_rooms )
        room->disconnect( this );
    m_rooms.erase(
        std::remove_if(m_rooms.begin(), m_rooms.end(),
            [=](const QMatrixClient::Room* r) { return r->connection() == connection; }),
        m_rooms.end());
    m_connections.removeOne(connection);
    endResetModel();
    // TODO: Restore selection
}

QMatrixClient::Room* RoomListModel::roomAt(int row)
{
    return m_rooms.at(row);
}

void RoomListModel::updateRoom(QMatrixClient::Room* room,
                               QMatrixClient::Room* prev)
{
    // There are two cases when this method is called:
    // 1. (prev == nullptr) adding a new room to the room list
    // 2. (prev != nullptr) accepting/rejecting an invitation or inviting to
    //    the previously left room (in both cases prev has the previous state).
    if (prev == room)
    {
        qCritical() << "RoomListModel::updateRoom: room tried to replace itself";
        refresh(static_cast<QMatrixClient::Room*>(room));
        return;
    }
    if (prev && room->id() != prev->id())
    {
        qCritical() << "RoomListModel::updateRoom: attempt to update room"
                    << room->id() << "to" << prev->id();
        // That doesn't look right but technically we still can do it.
    }
    // Ok, we're through with pre-checks, now for the real thing.
    auto* newRoom = room;
    const auto it = std::find_if(m_rooms.begin(), m_rooms.end(),
          [=](const QMatrixClient::Room* r) { return r == prev || r == newRoom; });
    if (it != m_rooms.end())
    {
        const int row = it - m_rooms.begin();
        // There's no guarantee that prev != newRoom
        if (*it == prev && *it != newRoom)
        {
            prev->disconnect(this);
            m_rooms.replace(row, newRoom);
            connectRoomSignals(newRoom);
        }
        emit dataChanged(index(row), index(row));
        emit roomDataChangedEvent(row);
    }
    else
    {
        beginInsertRows(QModelIndex(), m_rooms.count(), m_rooms.count());
        doAddRoom(newRoom);
        endInsertRows();
    }
}

void RoomListModel::deleteRoom(QMatrixClient::Room* room)
{
    auto i = m_rooms.indexOf(room);
    if (i == -1)
        return; // Already deleted, nothing to do

    beginRemoveRows(QModelIndex(), i, i);
    m_rooms.removeAt(i);
    endRemoveRows();
}

void RoomListModel::doAddRoom(QMatrixClient::Room* r)
{
    if (auto* room = r)
    {
        m_rooms.append(room);
        connectRoomSignals(room);
    } else
    {
        qCritical() << "Attempt to add nullptr to the room list";
        Q_ASSERT(false);
    }
}

void RoomListModel::connectRoomSignals(QMatrixClient::Room* room)
{
    connect(room, &QMatrixClient::Room::callEvent,
            this, &RoomListModel::callEventChanged); 
    connect(room, &QMatrixClient::Room::displaynameChanged,
            this, [=]{ displaynameChanged(room); });
    connect(room, &QMatrixClient::Room::unreadMessagesChanged,
            this, [=]{ unreadMessagesChanged(room); });
    connect(room, &QMatrixClient::Room::notificationCountChanged,
            this, [=]{ unreadMessagesChanged(room); });
    connect(room, &QMatrixClient::Room::joinStateChanged,
            this, [=]{ refresh(room); });
    connect(room, &QMatrixClient::Room::avatarChanged,
            this, [=]{ refresh(room, { Qt::DecorationRole }); });
}

int RoomListModel::rowCount(const QModelIndex& parent) const
{
    if( parent.isValid() )
        return 0;
    return m_rooms.count();
}

QVariant RoomListModel::data(const QModelIndex& index, int role) const
{
    if( !index.isValid() )
        return QVariant();

    if( index.row() >= m_rooms.count() )
    {
        qDebug() << "UserListModel: something wrong here...";
        return QVariant();
    }
    auto room = m_rooms.at(index.row());
    switch (role)
    {
        case Qt::DisplayRole:
        {
            const auto unreadCount = room->unreadCount();
            const auto postfix = unreadCount == -1 ? QString() :
                room->readMarker() != room->timelineEdge()
                    ? QStringLiteral(" [%1]").arg(unreadCount)
                    : QStringLiteral(" [%1+]").arg(unreadCount);
            for (auto c: m_connections)
            {
                if (c == room->connection())
                    continue;
                if (c->room(room->id(), room->joinState()))
                    return tr("%1 (as %2)").arg(room->displayName(),
                                                room->connection()->userId())
                           + postfix;
            }
            return room->displayName() + postfix;
        }
        case Qt::DecorationRole:
        {
            auto mediaid = room->avatarMediaId();
            if (!mediaid.isEmpty()) {
                auto url = QUrl("image://mtx/" + mediaid);
                return url;
            }
            switch( room->joinState() )
            {
                case QMatrixClient::JoinState::Join:
                    return "./resources/icons/breeze/irc-channel-joined.svg";
                case QMatrixClient::JoinState::Invite:
                    return "./resources/icons/irc-channel-invited.svg";
                case QMatrixClient::JoinState::Leave:
                    return "./resources/icons/breeze/irc-channel-parted.svg";
            }
        }
        case RoomEventStateRole:
        {
            if (room->highlightCount() > 0) {
                return "highlight";
            } else if (room->hasUnreadMessages()) {
                return "unread";
            } else {
                return "normal";
            }
        }
        return QVariant();
    }
    return QVariant();
}

QHash<int, QByteArray> RoomListModel::roleNames() const {
    return QHash<int, QByteArray>({
                      std::make_pair(Qt::DisplayRole, QByteArray("display")),
                      std::make_pair(Qt::DecorationRole, QByteArray("roomImg")),
                      std::make_pair(RoomEventStateRole, QByteArray("roomEventState"))
          });
}

void RoomListModel::displaynameChanged(QMatrixClient::Room* room)
{
    refresh(room);
}

void RoomListModel::unreadMessagesChanged(QMatrixClient::Room* room)
{
    refresh(room);
}

void RoomListModel::refresh(QMatrixClient::Room* room, const QVector<int>& roles)
{
    int row = m_rooms.indexOf(room);
    if (row == -1)
        qCritical() << "Room" << room->id() << "not found in the room list";
    else
        emit dataChanged(index(row), index(row), roles);
        emit roomDataChangedEvent(row);
}

void RoomListModel::callEventChanged(QMatrixClient::Room* room, const QMatrixClient::RoomEvent* e)
{

    const auto& ev = *e;

    visit(ev
        , [this, room] (const CallAnswerEvent& evt) {
            qCDebug(MAIN) << evt.toJson();
            emit callEvent("answer", room, evt.toJson());
        }
        , [this, room] (const CallCandidatesEvent& evt) {
            qCDebug(MAIN) << evt.toJson();
            emit callEvent("candidates", room, evt.toJson());
        }
        , [this, room] (const CallHangupEvent& evt) {
            qCDebug(MAIN) << evt.toJson();
            emit callEvent("hangup", room, evt.toJson());
        }
        , [this, room] (const CallInviteEvent& evt) {
            qCDebug(MAIN) << evt.toJson();
            emit callEvent("invite", room, evt.toJson());
        }
    );
}
