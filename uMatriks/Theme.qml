/*
This file is part of uMatriks.

uMatriks is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

uMatriks is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with uMatriks.  If not, see <https://www.gnu.org/licenses/>.
*/

pragma Singleton

import QtQuick 2.0
import Qt.labs.settings 1.0
import Ubuntu.Components 1.3
import Matrix 1.0

Settings {

    category: "Theme"

    property string textFont: "Ubuntumono"
    property string nickFont: "Consolas"
    property int textSize: units.gu(2) // Need to change to gu(something)
    property int timeLabelSize: units.gu(0.5) // Need to change to gu(something)
    property int listTextSize: units.gu(4)
    property string roomListBg: "#6a1b9a"
    property string chatBg: "#fdf6e3"
    property string roomListSelectedBg: "#9c27b0"
    property string unreadRoomFg: "white"
    property string highlightRoomFg: "white"
    property string normalRoomFg: "#dddddd"
    property QColor prueba: "white"

}
