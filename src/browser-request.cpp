/*
 * This file is part of signon-ui
 *
 * Copyright (C) 2011 Canonical Ltd.
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

#include "browser-request.h"

#include "debug.h"
#include "dialog.h"
#include "network-access-manager.h"

#include <QSettings>
#include <QVBoxLayout>
#include <QWebView>
#include <SignOn/uisessiondata_priv.h>

using namespace SignOnUi;

namespace SignOnUi {

static const QString keyPreferredWidth = QString("PreferredWidth");
static const QString keyTextSizeMultiplier = QString("TextSizeMultiplier");
static const QString keyViewportWidth = QString("ViewportWidth");
static const QString keyViewportHeight = QString("ViewportHeight");
static const QString keyZoomFactor = QString("ZoomFactor");

class BrowserRequestPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(BrowserRequest)

public:
    BrowserRequestPrivate(BrowserRequest *request);
    ~BrowserRequestPrivate();

    void buildDialog(const QVariantMap &params);
    void start();

private Q_SLOTS:
    void onUrlChanged(const QUrl &url);
    void onLoadFinished(bool ok);
    void onFinished();

private:
    void showDialog();
    void setupViewForUrl(const QUrl &url);

private:
    mutable BrowserRequest *q_ptr;
    Dialog *m_dialog;
    QWebView *m_webView;
    QUrl finalUrl;
    QUrl responseUrl;
    QString m_host;
};

} // namespace

BrowserRequestPrivate::BrowserRequestPrivate(BrowserRequest *request):
    QObject(request),
    q_ptr(request),
    m_dialog(0),
    m_webView(0)
{
}

BrowserRequestPrivate::~BrowserRequestPrivate()
{
    delete m_dialog;
}

void BrowserRequestPrivate::onUrlChanged(const QUrl &url)
{
    TRACE() << "Url changed:" << url;

    if (url.host() == finalUrl.host() &&
        url.path() == finalUrl.path()) {
        responseUrl = url;
    }

    setupViewForUrl(url);
}

void BrowserRequestPrivate::onLoadFinished(bool ok)
{
    TRACE() << "Load finished" << ok;

    if (!m_dialog->isVisible()) {
        if (responseUrl.isEmpty()) {
            showDialog();
        } else {
            onFinished();
        }
    }
}

void BrowserRequestPrivate::buildDialog(const QVariantMap &params)
{
    m_dialog = new Dialog;

    QString title;
    if (params.contains(SSOUI_KEY_TITLE)) {
        title = params[SSOUI_KEY_TITLE].toString();
    } else if (params.contains(SSOUI_KEY_CAPTION)) {
        title = tr("Web authentication for %1").
            arg(params[SSOUI_KEY_CAPTION].toString());
    } else {
        title = tr("Web authentication");
    }

    m_dialog->setWindowTitle(title);

    QVBoxLayout *layout = new QVBoxLayout(m_dialog);

    m_webView = new QWebView();
    m_webView->page()->setNetworkAccessManager(NetworkAccessManager::instance());

    QUrl url(params.value(SSOUI_KEY_OPENURL).toString());
    setupViewForUrl(url);
    m_webView->setUrl(url);
    QObject::connect(m_webView, SIGNAL(urlChanged(const QUrl&)),
                     this, SLOT(onUrlChanged(const QUrl&)));
    QObject::connect(m_webView, SIGNAL(loadFinished(bool)),
                     this, SLOT(onLoadFinished(bool)));
    layout->addWidget(m_webView);

    TRACE() << "Dialog was built";
}

void BrowserRequestPrivate::start()
{
    Q_Q(BrowserRequest);

    finalUrl = QUrl(q->parameters().value(SSOUI_KEY_FINALURL).toString());
    buildDialog(q->parameters());

    QObject::connect(m_dialog, SIGNAL(finished(int)),
                     this, SLOT(onFinished()));
}

void BrowserRequestPrivate::onFinished()
{
    Q_Q(BrowserRequest);

    TRACE() << "Browser dialog closed";

    QVariantMap reply;
    QUrl url = responseUrl.isEmpty() ? m_webView->url() : responseUrl;
    reply[SSOUI_KEY_URLRESPONSE] = url.toString();

    q->setResult(reply);
}

void BrowserRequestPrivate::showDialog()
{
    Q_Q(BrowserRequest);

    q->setWidget(m_dialog);
}

void BrowserRequestPrivate::setupViewForUrl(const QUrl &url)
{
    QString host = url.host();
    if (host == m_host) return;

    m_host = host;

    /* Load the host-specific configuration file */
    QSettings settings("signon-ui/webkit-options.d/" + host);

    if (settings.contains(keyViewportWidth) &&
        settings.contains(keyViewportHeight)) {
        QSize viewportSize(settings.value(keyViewportWidth).toInt(),
                           settings.value(keyViewportHeight).toInt());
        m_webView->page()->setViewportSize(viewportSize);
        m_webView->setFixedSize(viewportSize);
    }

    if (settings.contains(keyPreferredWidth)) {
        QSize preferredSize(settings.value(keyPreferredWidth).toInt(), 300);
        m_webView->page()->setPreferredContentsSize(preferredSize);
    }

    if (settings.contains(keyTextSizeMultiplier)) {
        m_webView->setTextSizeMultiplier(settings.value(keyTextSizeMultiplier).
                                         toReal());
    }

    if (settings.contains(keyZoomFactor)) {
        m_webView->setZoomFactor(settings.value(keyZoomFactor).toReal());
    }
}

BrowserRequest::BrowserRequest(const QDBusConnection &connection,
                             const QDBusMessage &message,
                             const QVariantMap &parameters,
                             QObject *parent):
    Request(connection, message, parameters, parent),
    d_ptr(new BrowserRequestPrivate(this))
{
}

BrowserRequest::~BrowserRequest()
{
}

void BrowserRequest::start()
{
    Q_D(BrowserRequest);

    Request::start();
    d->start();
}

#include "browser-request.moc"
