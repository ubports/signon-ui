/*
 * This file is part of signon-ui
 *
 * Copyright (C) 2013 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ubuntu-browser-request.h"

#include "debug.h"
#include "qquick-dialog.h"
#include "errors.h"
#include "i18n.h"

#include <QDir>
#include <QQmlContext>
#include <QStandardPaths>
#include <QTimer>
#include <SignOn/uisessiondata_priv.h>

using namespace SignOnUi;
using namespace SignOnUi::QQuick;

namespace SignOnUi {

class UbuntuBrowserRequestPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(UbuntuBrowserRequest)
    Q_PROPERTY(QUrl pageComponentUrl READ pageComponentUrl CONSTANT)
    Q_PROPERTY(QUrl currentUrl READ currentUrl WRITE setCurrentUrl)
    Q_PROPERTY(QUrl startUrl READ startUrl CONSTANT)
    Q_PROPERTY(QUrl finalUrl READ finalUrl CONSTANT)

public:
    UbuntuBrowserRequestPrivate(UbuntuBrowserRequest *request);
    ~UbuntuBrowserRequestPrivate();

    void start();

    void setCurrentUrl(const QUrl &url);
    QUrl pageComponentUrl() const;
    QUrl currentUrl() const { return m_currentUrl; }
    QUrl startUrl() const { return m_startUrl; }
    QUrl finalUrl() const { return m_finalUrl; }
    QUrl responseUrl() const { return m_responseUrl; }

public Q_SLOTS:
    void cancel();
    void onLoadStarted();
    void onLoadFinished(bool ok);

private Q_SLOTS:
    void onFailTimer();
    void onFinished();

private:
    void buildDialog(const QVariantMap &params);

private:
    Dialog *m_dialog;
    QUrl m_currentUrl;
    QUrl m_startUrl;
    QUrl m_finalUrl;
    QUrl m_responseUrl;
    QTimer m_failTimer;
    mutable UbuntuBrowserRequest *q_ptr;
};

} // namespace

UbuntuBrowserRequestPrivate::UbuntuBrowserRequestPrivate(
    UbuntuBrowserRequest *request):
    QObject(request),
    m_dialog(0),
    q_ptr(request)
{
    m_failTimer.setSingleShot(true);
    m_failTimer.setInterval(3000);
    QObject::connect(&m_failTimer, SIGNAL(timeout()),
                     this, SLOT(onFailTimer()));
}

UbuntuBrowserRequestPrivate::~UbuntuBrowserRequestPrivate()
{
    delete m_dialog;
}

void UbuntuBrowserRequestPrivate::start()
{
    Q_Q(UbuntuBrowserRequest);

    const QVariantMap &params = q->parameters();
    TRACE() << params;

    QString cachePath =
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir rootDir = cachePath + QString("/id-%1").arg(q->identity());
    if (!rootDir.exists()) {
        rootDir.mkpath(".");
    }

    m_finalUrl = params.value(SSOUI_KEY_FINALURL).toString();
    m_startUrl = params.value(SSOUI_KEY_OPENURL).toString();
    buildDialog(params);

    QObject::connect(m_dialog, SIGNAL(finished(int)),
                     this, SLOT(onFinished()));

    QUrl webview("qrc:/MainWindow.qml");
    QDir qmlDir("/usr/share/signon-ui/qml");
    if (qmlDir.exists())
    {
        QFileInfo qmlFile(qmlDir.absolutePath() + "/MainWindow.qml");
        if (qmlFile.exists())
            webview.setUrl(qmlFile.absoluteFilePath());
    }

    m_dialog->rootContext()->setContextProperty("request", this);
    m_dialog->rootContext()->setContextProperty("rootDir",
                                                QUrl::fromLocalFile(rootDir.absolutePath()));
    m_dialog->setSource(webview);
}

QUrl UbuntuBrowserRequestPrivate::pageComponentUrl() const
{
    Q_Q(const UbuntuBrowserRequest);
    /* We define the X-PageComponent key to let the clients override the QML
     * component to be used to build the authentication page.
     * To prevent a malicious client to show it's own UI, we require that the
     * file path begins with "/usr/share/signon-ui/" (where Ubuntu click
     * packages cannot install files).
     */
    QUrl providedUrl = q->clientData().value("X-PageComponent").toString();
    if (providedUrl.isValid() && providedUrl.isLocalFile() &&
        providedUrl.path().startsWith("/usr/share/signon-ui/")) {
        return providedUrl;
    } else {
        return QStringLiteral("DefaultPage.qml");
    }
}

void UbuntuBrowserRequestPrivate::setCurrentUrl(const QUrl &url)
{
    TRACE() << "Url changed:" << url;
    m_failTimer.stop();

    if (url.host() == m_finalUrl.host() &&
        url.path() == m_finalUrl.path()) {
        m_responseUrl = url;
        if (!m_dialog->isVisible()) {
            /* Do not show the notification page. */
            m_dialog->accept();
        } else {
            /* Replace the web page with an information screen */
            /* TODO */
            m_dialog->accept();
        }
    }
}

void UbuntuBrowserRequestPrivate::cancel()
{
    Q_Q(UbuntuBrowserRequest);

    TRACE() << "Client requested to cancel";
    q->setCanceled();
    if (m_dialog) {
        m_dialog->close();
    }
}

void UbuntuBrowserRequestPrivate::onLoadStarted()
{
    m_failTimer.stop();
}

void UbuntuBrowserRequestPrivate::onLoadFinished(bool ok)
{
    Q_Q(const UbuntuBrowserRequest);

    TRACE() << "Load finished" << ok;

    if (!ok) {
        m_failTimer.start();
        return;
    }

    if (!m_dialog->isVisible()) {
        if (m_responseUrl.isEmpty()) {
            Dialog::ShowMode mode = (q->windowId() == 0) ? Dialog::TopLevel :
                q->embeddedUi() ? Dialog::Embedded : Dialog::Transient;
            m_dialog->show(q->windowId(), mode);
        } else {
            onFinished();
        }
    }
}

void UbuntuBrowserRequestPrivate::onFailTimer()
{
    Q_Q(UbuntuBrowserRequest);

    TRACE() << "Page loading failed";
    if (m_dialog) {
        m_dialog->close();
    }
    q->setResult(QVariantMap());
}

void UbuntuBrowserRequestPrivate::onFinished()
{
    Q_Q(UbuntuBrowserRequest);

    TRACE() << "Browser dialog closed";

    QVariantMap reply;
    QUrl url = m_responseUrl.isEmpty() ? m_currentUrl : m_responseUrl;
    reply[SSOUI_KEY_URLRESPONSE] = url.toString();

    m_dialog->close();

    q->setResult(reply);
}

void UbuntuBrowserRequestPrivate::buildDialog(const QVariantMap &params)
{
    m_dialog = new Dialog;

    QString title;
    if (params.contains(SSOUI_KEY_TITLE)) {
        title = params[SSOUI_KEY_TITLE].toString();
    } else if (params.contains(SSOUI_KEY_CAPTION)) {
        title = _("Web authentication for %1").
            arg(params[SSOUI_KEY_CAPTION].toString());
    } else {
        title = _("Web authentication");
    }

    m_dialog->setTitle(title);

    TRACE() << "Dialog was built";
}

UbuntuBrowserRequest::UbuntuBrowserRequest(const QDBusConnection &connection,
                                           const QDBusMessage &message,
                                           const QVariantMap &parameters,
                                           QObject *parent):
    Request(connection, message, parameters, parent),
    d_ptr(new UbuntuBrowserRequestPrivate(this))
{
}

UbuntuBrowserRequest::~UbuntuBrowserRequest()
{
}

void UbuntuBrowserRequest::start()
{
    Q_D(UbuntuBrowserRequest);

    Request::start();
    d->start();
}

#include "ubuntu-browser-request.moc"
