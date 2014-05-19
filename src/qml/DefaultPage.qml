import QtQuick 2.0
import Ubuntu.Components 0.1

Page {
    Loader {
        id: loader
        anchors {
            fill: parent
            bottomMargin: osk.height
        }
        focus: true
        sourceComponent: browserComponent
    }
}
