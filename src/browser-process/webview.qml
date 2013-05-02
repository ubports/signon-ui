import QtQuick 2.0
import QtWebKit 3.0

Item {
    width: 400
    height: 300

    WebView {
        id: webView
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: osk.top
        }

        Component.onCompleted: url = request.startUrl

        onLoadingChanged: {
            console.log("Loading changed")
            if (loadRequest.status === WebView.LoadSucceededStatus) {
                request.onLoadFinished(true)
            }
        }
        onUrlChanged: request.currentUrl = url
    }

    KeyboardRectangle {
        id: osk
    }
}
