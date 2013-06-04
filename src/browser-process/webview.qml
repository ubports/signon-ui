import QtQuick 2.0
import QtQuick.Window 2.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0

FocusScope {
    id: browser
    width: 400
    height: 300
    focus: true
    property string qtwebkitdpr: "1.0"

    QtObject {
        // clumsy way of defining an enum in QML
        id: formFactor
        readonly property int desktop: 0
        readonly property int phone: 1
        readonly property int tablet: 2
    }
    // FIXME: this is a quick hack that will become increasingly unreliable
    // as we support more devices, so we need a better solution for this
    // FIXME: only handling phone and tablet for now
    property int formFactor: (Screen.width >= units.gu(60)) ? formFactor.tablet : formFactor.phone

    onQtwebkitdprChanged: {
        // Do not make this patch to QtWebKit a hard requirement.
        if (webview.experimental.hasOwnProperty('devicePixelRatio')) {
            webview.experimental.devicePixelRatio = qtwebkitdpr
        }
    }

    WebView {
        id: webView
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: osk.top
        }
        focus: true
        experimental.userAgent: {
            // FIXME: using iOS 5.0's iPhone/iPad user-agent strings
            // (source: http://stackoverflow.com/questions/7825873/what-is-the-ios-5-0-user-agent-string),
            // this should be changed to a more neutral user-agent in the
            // future as we don't want websites to recommend installing
            // their iPhone/iPad apps.
            if (browser.formFactor === formFactor.phone) {
                return "Mozilla/5.0 (iPhone; CPU iPhone OS 5_0 like Mac OS X) AppleWebKit/534.46 (KHTML, like Gecko) Version/5.1 Mobile/9A334 Safari/7534.48.3"
            } else if (browser.formFactor === formFactor.tablet) {
                return "Mozilla/5.0 (iPad; CPU OS 5_0 like Mac OS X) AppleWebKit/534.46 (KHTML, like Gecko) Version/5.1 Mobile/9A334 Safari/7534.48.3"
            }
        }

        experimental.preferences.developerExtrasEnabled: false
        experimental.preferences.navigatorQtObjectEnabled: true


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
