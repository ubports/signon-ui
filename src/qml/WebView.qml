import QtQuick 2.0
import Ubuntu.Components 1.3
import Ubuntu.Web 0.2

WebView {
    Component.onCompleted: url = signonRequest.startUrl

    onLoadingStateChanged: {
        console.log("Loading changed")
        if (loading) {
            signonRequest.onLoadStarted()
        } else if (lastLoadSucceeded) {
            signonRequest.onLoadFinished(true)
        } else {
            signonRequest.onLoadFinished(false)
        }
    }
    onUrlChanged: signonRequest.currentUrl = url

    context: WebContext {
        dataPath: rootDir
    }
}
