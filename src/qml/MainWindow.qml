import QtQuick 2.0
import Ubuntu.Components 0.1

MainView {
    id: root
    width: units.gu(60)
    height: units.gu(90)

    Loader {
        id: loader

        property Component webView: browserComponent
        property Item osk: osk

        anchors.fill: parent
        focus: true
        source: signonRequest.pageComponentUrl
    }

    KeyboardRectangle {
        id: osk
    }

    Component {
        id: browserComponent
        WebView {
        }
    }
}
