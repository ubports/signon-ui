import QtQuick 2.0
import Ubuntu.Components 0.1

Page {

    Loader {
        id: loader
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: osk.top
        }
        focus: true
        sourceComponent: browserComponent
    }

    KeyboardRectangle {
        id: osk
    }
}
