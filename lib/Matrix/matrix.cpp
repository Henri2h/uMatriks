#include <QtQml>
#include <QtQml/QQmlContext>
#include "matrix.h"

#include "user.h"
#include "csapi/joining.h"
#include "models/messageeventmodel.h"
#include "models/roomlistmodel.h"
#include "settings.h"
#include "matrixhelper.h"

Q_DECLARE_METATYPE(SyncJob*)
Q_DECLARE_METATYPE(Room*)

void MatrixPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QLatin1String("Matrix"));

    qmlRegisterType<SyncJob>(); qRegisterMetaType<SyncJob*> ("SyncJob*");
    qmlRegisterType<JoinRoomJob>(); qRegisterMetaType<JoinRoomJob*> ("JoinRoomJob*");
    // qmlRegisterType<LeaveRoomJob>(); qRegisterMetaType<LeaveRoomJob*> ("LeaveRoomJob*");
    qmlRegisterType<Room>(); qRegisterMetaType<Room*> ("Room*");
    qmlRegisterType<User>(); qRegisterMetaType<User*> ("User*");

    qmlRegisterType<Connection> ("Matrix", 1, 0, "Connection");
    qmlRegisterType<MessageEventModel> ("Matrix", 1, 0, "MessageEventModel");
    qmlRegisterType<RoomListModel> ("Matrix", 1, 0, "RoomListModel");
}

void MatrixPlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    QQmlExtensionPlugin::initializeEngine(engine, uri);

    // qDebug() << "uri: " << uri;
    Connection* conn = new Connection();
    // TODO we need so set somewhere the connection or base_url is empty
    img = new ImageProvider(conn);
    engine->addImageProvider("mtx", img);
    auto matrixHelper = new MatrixHelper();
    engine->rootContext()->setContextProperty("matrixHelper", matrixHelper);
    connect(matrixHelper, &MatrixHelper::setImageProviderConnection, this, &MatrixPlugin::setImageProviderConnection);
}

void MatrixPlugin::setImageProviderConnection(Connection* connection)
{
    qDebug() << "connection: " << connection;
    img->setConnection(connection);
}
