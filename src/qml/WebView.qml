import QtQuick 2.0
import Ubuntu.Components 1.3
import Ubuntu.Web 0.2

WebView {
    id: root

    Component.onCompleted: url = signonRequest.startUrl

    onLoadingStateChanged: {
        console.log("Loading changed")
        if (loading && !lastLoadFailed) {
            signonRequest.onLoadStarted()
        } else if (lastLoadSucceeded) {
            signonRequest.onLoadFinished(true)
        } else if (lastLoadFailed) {
            signonRequest.onLoadFinished(false)
        }
    }
    onUrlChanged: signonRequest.currentUrl = url

    context: WebContext {
        dataPath: rootDir
    }

    /* Taken from webbrowser-app */
    ProgressBar {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: units.dp(3)
        showProgressPercentage: false
        visible: root.loading
        value: root.loadProgress / 100
    }

}
