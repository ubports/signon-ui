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

#include "browser-process.h"

#include "debug.h"
#include "dialog.h"
#include "i18n.h"
#include "remote-request-interface.h"

#include <QQmlContext>
#include <SignOn/uisessiondata_priv.h>
#include <stdio.h>
#include <QDir>
#include <QFile>
#include <QTimer>

using namespace SignOnUi;

namespace SignOnUi {

class BrowserProcessPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(BrowserProcess)
    Q_PROPERTY(QUrl pageComponentUrl READ pageComponentUrl CONSTANT)
    Q_PROPERTY(QUrl currentUrl READ currentUrl WRITE setCurrentUrl)
    Q_PROPERTY(QUrl startUrl READ startUrl CONSTANT)
    Q_PROPERTY(QUrl finalUrl READ finalUrl CONSTANT)

public:
    BrowserProcessPrivate(BrowserProcess *request);
    ~BrowserProcessPrivate();

    void processClientRequest();

    void setCurrentUrl(const QUrl &url);
    QUrl pageComponentUrl() const;
    QUrl currentUrl() const { return m_currentUrl; }
    QUrl startUrl() const { return m_startUrl; }
    QUrl finalUrl() const { return m_finalUrl; }
    QUrl responseUrl() const { return m_responseUrl; }
    WId windowId() const {
        return m_clientData[SSOUI_KEY_WINDOWID].toUInt();
    }
    bool embeddedUi() const {
        return m_clientData[SSOUI_KEY_EMBEDDED].toBool();
    }


public Q_SLOTS:
    void onLoadStarted();
    void onLoadFinished(bool ok);
    void cancel();

private Q_SLOTS:
    void start(const QVariantMap &params);
    void onFailTimer();
    void onFinished();

private:
    void buildDialog(const QVariantMap &params);

private:
    Dialog *m_dialog;
    QVariantMap m_clientData;
    QUrl m_currentUrl;
    QUrl m_startUrl;
    QUrl m_finalUrl;
    QUrl m_responseUrl;
    QString m_host;
    QFile m_input;
    QFile m_output;
    RemoteRequestServer m_server;
    QTimer m_failTimer;
    mutable BrowserProcess *q_ptr;
};

} // namespace

BrowserProcessPrivate::BrowserProcessPrivate(BrowserProcess *process):
    QObject(process),
    m_dialog(0),
    q_ptr(process)
{
    m_failTimer.setSingleShot(true);
    m_failTimer.setInterval(3000);
    QObject::connect(&m_failTimer, SIGNAL(timeout()),
                     this, SLOT(onFailTimer()));
}

BrowserProcessPrivate::~BrowserProcessPrivate()
{
    delete m_dialog;
}

QUrl BrowserProcessPrivate::pageComponentUrl() const
{
    /* We define the X-PageComponent key to let the clients override the QML
     * component to be used to build the authentication page.
     * To prevent a malicious client to show it's own UI, we require that the
     * file patth begins with "/usr/share/signon-ui/" (where Ubuntu click
     * packages cannot install files).
     */
    QUrl providedUrl = m_clientData.value("X-PageComponent").toString();
    if (providedUrl.isValid() && providedUrl.isLocalFile() &&
        providedUrl.path().startsWith("/usr/share/signon-ui/")) {
        return providedUrl;
    } else {
        return QStringLiteral("DefaultPage.qml");
    }
}

void BrowserProcessPrivate::processClientRequest()
{
    TRACE();
    m_input.open(stdin, QIODevice::ReadOnly);
    m_output.open(stdout, QIODevice::WriteOnly);

    QObject::connect(&m_server, SIGNAL(started(const QVariantMap&)),
                     this, SLOT(start(const QVariantMap&)));
    QObject::connect(&m_server, SIGNAL(canceled()),
                     this, SLOT(cancel()));

    /* This effectively starts the communication with the client */
    m_server.setChannels(&m_input, &m_output);
}

void BrowserProcessPrivate::setCurrentUrl(const QUrl &url)
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

void BrowserProcessPrivate::onLoadStarted()
{
    m_failTimer.stop();
}

void BrowserProcessPrivate::onLoadFinished(bool ok)
{
    TRACE() << "Load finished" << ok;

    if (!ok) {
        m_failTimer.start();
        return;
    }

    if (!m_dialog->isVisible()) {
        if (m_responseUrl.isEmpty()) {
            Dialog::ShowMode mode = (windowId() == 0) ? Dialog::TopLevel :
                embeddedUi() ? Dialog::Embedded : Dialog::Transient;
            m_dialog->show(windowId(), mode);
        } else {
            onFinished();
        }
    }
}

void BrowserProcessPrivate::start(const QVariantMap &params)
{
    TRACE() << params;
    if (params.contains(SSOUI_KEY_CLIENT_DATA)) {
        m_clientData = params[SSOUI_KEY_CLIENT_DATA].toMap();
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
    m_dialog->setSource(webview);
}

void BrowserProcessPrivate::cancel()
{
    Q_Q(BrowserProcess);

    TRACE() << "Client requested to cancel";
    m_server.setCanceled();
    if (m_dialog) {
        m_dialog->close();
    }
    Q_EMIT q->finished();
}

void BrowserProcessPrivate::onFailTimer()
{
    Q_Q(BrowserProcess);

    TRACE() << "Page loading failed";
    m_server.setResult(QVariantMap());
    if (m_dialog) {
        m_dialog->close();
    }
    Q_EMIT q->finished();
}

void BrowserProcessPrivate::onFinished()
{
    Q_Q(BrowserProcess);

    TRACE() << "Browser dialog closed";

    QVariantMap reply;
    QUrl url = m_responseUrl.isEmpty() ? m_currentUrl : m_responseUrl;
    reply[SSOUI_KEY_URLRESPONSE] = url.toString();

    m_server.setResult(reply);
    m_dialog->close();

    Q_EMIT q->finished();
}

void BrowserProcessPrivate::buildDialog(const QVariantMap &params)
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

BrowserProcess::BrowserProcess(QObject *parent):
    QObject(parent),
    d_ptr(new BrowserProcessPrivate(this))
{
}

BrowserProcess::~BrowserProcess()
{
}

void BrowserProcess::processClientRequest()
{
    Q_D(BrowserProcess);
    d->processClientRequest();
}

#include "browser-process.moc"
