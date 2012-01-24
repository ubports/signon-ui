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

#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QStackedLayout>
#include <QVBoxLayout>
#include <QWebElement>
#include <QWebFrame>
#include <QWebView>
#include <SignOn/uisessiondata_priv.h>

using namespace SignOnUi;

namespace SignOnUi {

static const QString keyPreferredWidth = QString("PreferredWidth");
static const QString keyTextSizeMultiplier = QString("TextSizeMultiplier");
static const QString keyUserAgent = QString("UserAgent");
static const QString keyViewportWidth = QString("ViewportWidth");
static const QString keyViewportHeight = QString("ViewportHeight");
static const QString keyZoomFactor = QString("ZoomFactor");
static const QString keyUsernameField = QString("UsernameField");
static const QString keyPasswordField = QString("PasswordField");

class WebPage: public QWebPage
{
    Q_OBJECT

public:
    WebPage(QObject *parent = 0): QWebPage(parent) {}
    ~WebPage() {}

    void setUserAgent(const QString &userAgent) { m_userAgent = userAgent; }

protected:
    // reimplemented virtual methods
    QString userAgentForUrl(const QUrl &url) const
    {
        return m_userAgent.isEmpty() ?
            QWebPage::userAgentForUrl(url) : m_userAgent;
    }

private:
    QString m_userAgent;
};

class WebView: public QWebView
{
    Q_OBJECT

public:
    WebView(QWidget *parent = 0):
        QWebView(parent)
    {
        setSizePolicy(QSizePolicy::MinimumExpanding,
                      QSizePolicy::MinimumExpanding);
        setAttribute(Qt::WA_OpaquePaintEvent, true);
    }
    ~WebView() {};

    void setPreferredSize(const QSize &size) {
        m_preferredSize = size;
        updateGeometry();
    }

protected:
    QSize sizeHint() const {
        if (m_preferredSize.isValid()) {
            return m_preferredSize;
        } else {
            return QSize(400, 300);
        }
    }

    void paintEvent(QPaintEvent *event) {
        QPainter painter(this);
        painter.fillRect(rect(), palette().window());
        QWebView::paintEvent(event);
    }

private:
    QSize m_preferredSize;
};

class BrowserRequestPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(BrowserRequest)

public:
    BrowserRequestPrivate(BrowserRequest *request);
    ~BrowserRequestPrivate();

    QWidget *buildWebViewPage(const QVariantMap &params);
    QWidget *buildSuccessPage();
    void buildDialog(const QVariantMap &params);
    void start();

private Q_SLOTS:
    void onUrlChanged(const QUrl &url);
    void onLoadFinished(bool ok);
    void onFinished();
    void onContentsChanged();

private:
    void showDialog();
    void setupViewForUrl(const QUrl &url);
    void notifyAuthCompleted();
    QWebElement initializeField(const QString &settingsKey,
                                const QString &paramKey);
    void initializeFields();

private:
    mutable BrowserRequest *q_ptr;
    Dialog *m_dialog;
    QStackedLayout *m_dialogLayout;
    QWidget *m_webViewPage;
    QWidget *m_successPage;
    WebView *m_webView;
    QProgressBar *m_progressBar;
    QUrl finalUrl;
    QUrl responseUrl;
    QString m_host;
    QSettings *m_settings;
    QWebElement m_usernameField;
    QWebElement m_passwordField;
    QString m_username;
    QString m_password;
};

} // namespace

BrowserRequestPrivate::BrowserRequestPrivate(BrowserRequest *request):
    QObject(request),
    q_ptr(request),
    m_dialog(0),
    m_webView(0),
    m_progressBar(0),
    m_settings(0)
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
        if (m_dialog->isVisible()) {
            /* Replace the web page with an information screen */
            notifyAuthCompleted();
        }
    }

    setupViewForUrl(url);
}

void BrowserRequestPrivate::onLoadFinished(bool ok)
{
    TRACE() << "Load finished" << ok;

    initializeFields();

    if (!m_dialog->isVisible()) {
        if (responseUrl.isEmpty()) {
            showDialog();
        } else {
            onFinished();
        }
    }
}

QWidget *BrowserRequestPrivate::buildWebViewPage(const QVariantMap &params)
{
    QWidget *dialogPage = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(dialogPage);

    m_webView = new WebView();
    WebPage *page = new WebPage(this);
    page->setNetworkAccessManager(NetworkAccessManager::instance());
    QObject::connect(page, SIGNAL(contentsChanged()),
                     this, SLOT(onContentsChanged()));
    m_webView->setPage(page);

    QUrl url(params.value(SSOUI_KEY_OPENURL).toString());
    setupViewForUrl(url);
    m_webView->setUrl(url);
    QObject::connect(m_webView, SIGNAL(urlChanged(const QUrl&)),
                     this, SLOT(onUrlChanged(const QUrl&)));
    QObject::connect(m_webView, SIGNAL(loadFinished(bool)),
                     this, SLOT(onLoadFinished(bool)));
    layout->addWidget(m_webView);

    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    QObject::connect(m_webView, SIGNAL(loadProgress(int)),
                     m_progressBar, SLOT(setValue(int)));
    QObject::connect(m_webView, SIGNAL(loadStarted()),
                     m_progressBar, SLOT(show()));
    QObject::connect(m_webView, SIGNAL(loadFinished(bool)),
                     m_progressBar, SLOT(hide()));
    layout->addWidget(m_progressBar);

    return dialogPage;
}

QWidget *BrowserRequestPrivate::buildSuccessPage()
{
    QWidget *dialogPage = new QWidget;
    dialogPage->setSizePolicy(QSizePolicy::Ignored,
                              QSizePolicy::MinimumExpanding);
    QVBoxLayout *layout = new QVBoxLayout(dialogPage);

    QLabel *label = new QLabel(tr("The authentication process is complete.\n"
                                  "You may now close this dialog "
                                  "and return to the application."));
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);

    QPushButton *doneButton = new QPushButton(tr("Done"));
    doneButton->setDefault(true);
    QObject::connect(doneButton, SIGNAL(clicked()),
                     m_dialog, SLOT(accept()));
    layout->addWidget(doneButton);

    return dialogPage;
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

    m_dialogLayout = new QStackedLayout(m_dialog);

    m_webViewPage = buildWebViewPage(params);
    m_dialogLayout->addWidget(m_webViewPage);

    m_successPage = buildSuccessPage();
    m_dialogLayout->addWidget(m_successPage);

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

    if (!m_username.isEmpty())
        reply[SSOUI_KEY_USERNAME] = m_username;
    if (!m_password.isEmpty())
        reply[SSOUI_KEY_PASSWORD] = m_password;

    q->setResult(reply);
}

void BrowserRequestPrivate::onContentsChanged()
{
    /* See https://bugs.webkit.org/show_bug.cgi?id=32865 for the reason why
     * we are not simply calling m_usernameField.attribute("value")
     */
    if (!m_usernameField.isNull()) {
        m_username =
            m_usernameField.evaluateJavaScript("this.value").toString();
    }
    if (!m_passwordField.isNull()) {
        m_password =
            m_passwordField.evaluateJavaScript("this.value").toString();
    }
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
    delete m_settings;
    m_settings = new QSettings("signon-ui/webkit-options.d/" + host, QString(), this);

    if (m_settings->contains(keyViewportWidth) &&
        m_settings->contains(keyViewportHeight)) {
        QSize viewportSize(m_settings->value(keyViewportWidth).toInt(),
                           m_settings->value(keyViewportHeight).toInt());
        m_webView->setPreferredSize(viewportSize);
    }

    if (m_settings->contains(keyPreferredWidth)) {
        QSize preferredSize(m_settings->value(keyPreferredWidth).toInt(), 300);
        m_webView->page()->setPreferredContentsSize(preferredSize);
    }

    if (m_settings->contains(keyTextSizeMultiplier)) {
        m_webView->setTextSizeMultiplier(m_settings->value(keyTextSizeMultiplier).
                                         toReal());
    }

    if (m_settings->contains(keyUserAgent)) {
        WebPage *page = qobject_cast<WebPage *>(m_webView->page());
        if (page != 0)
            page->setUserAgent(m_settings->value(keyUserAgent).toString());
    }

    if (m_settings->contains(keyZoomFactor)) {
        m_webView->setZoomFactor(m_settings->value(keyZoomFactor).toReal());
    }
}

void BrowserRequestPrivate::notifyAuthCompleted()
{
    m_dialogLayout->setCurrentWidget(m_successPage);
}

QWebElement BrowserRequestPrivate::initializeField(const QString &settingsKey,
                                                   const QString &paramKey)
{
    Q_Q(BrowserRequest);

    QWebElement element;

    if (!m_settings->contains(settingsKey)) return element;

    QString selector = m_settings->value(settingsKey).toString();
    if (selector.isEmpty()) return element;

    QWebFrame *frame = m_webView->page()->mainFrame();
    element = frame->findFirstElement(selector);
    if (!element.isNull()) {
        const QVariantMap &params = q->parameters();
        if (params.contains(paramKey)) {
            QString value = params.value(paramKey).toString();
            element.setAttribute("value", value);
        }
    } else {
        BLAME() << "Couldn't find element:" << selector;
    }

    return element;
}

void BrowserRequestPrivate::initializeFields()
{
    /* If the configuration file contains a "UsernameField" or a
     * "PasswordField" key whose value is set to a valid CSS selector, we get
     * the QWebElement to these fields.
     * Also, if the username or password are present in the input parameters,
     * we prefill the respective fields.
     */
    m_usernameField = initializeField(keyUsernameField, SSOUI_KEY_USERNAME);
    m_passwordField = initializeField(keyPasswordField, SSOUI_KEY_PASSWORD);
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
