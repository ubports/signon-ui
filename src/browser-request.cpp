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

#include "animation-label.h"
#include "cookie-jar-manager.h"
#include "debug.h"
#include "dialog.h"
#include "i18n.h"

#include <QDBusArgument>
#include <QDesktopServices>
#include <QLabel>
#include <QNetworkCookie>
#include <QNetworkRequest>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QRegExp>
#include <QSettings>
#include <QStackedLayout>
#include <QTimer>
#include <QVBoxLayout>
#include <QWebElement>
#include <QWebFrame>
#include <QWebView>
#include <SignOn/uisessiondata_priv.h>

using namespace SignOnUi;

namespace SignOnUi {

static const QString keyPreferredWidth = QString("PreferredWidth");
static const QString keyHorizontalScrollBar = QString("HorizontalScrollBar");
static const QString keyVerticalScrollBar = QString("VerticalScrollBar");
static const QString keyTextSizeMultiplier = QString("TextSizeMultiplier");
static const QString keyUserAgent = QString("UserAgent");
static const QString keyViewportWidth = QString("ViewportWidth");
static const QString keyViewportHeight = QString("ViewportHeight");
static const QString keyZoomFactor = QString("ZoomFactor");
static const QString keyUsernameField = QString("UsernameField");
static const QString keyPasswordField = QString("PasswordField");
static const QString keyLoginButton = QString("LoginButton");
static const QString keyInternalLinksPattern = QString("InternalLinksPattern");
static const QString keyExternalLinksPattern = QString("ExternalLinksPattern");
static const QString keyAllowedUrls = QString("AllowedUrls");
static const QString valueAlwaysOn = QString("alwaysOn");
static const QString valueAlwaysOff = QString("alwaysOff");

/* Additional session-data keys we support. */
static const QString keyCookies = QString("Cookies");
static const QString keyAllowedSchemes = QString("AllowedSchemes");

class WebPage: public QWebPage
{
    Q_OBJECT

public:
    WebPage(QObject *parent = 0): QWebPage(parent) {}
    ~WebPage() {}

    void setUserAgent(const QString &userAgent) { m_userAgent = userAgent; }

    void setExternalLinksPattern(const QString &pattern) {
        m_externalLinksPattern =
            QRegExp(pattern, Qt::CaseInsensitive, QRegExp::RegExp2);
    }

    void setInternalLinksPattern(const QString &pattern) {
        m_internalLinksPattern =
            QRegExp(pattern, Qt::CaseInsensitive, QRegExp::RegExp2);
    }

    void setAllowedSchemes(const QStringList &schemes) {
        m_allowedSchemes = schemes;
    }

    void setAllowedUrls(const QString &pattern) {
        m_allowedUrls =
            QRegExp(pattern, Qt::CaseInsensitive, QRegExp::RegExp2);
    }

    void setFinalUrl(const QUrl &url) { m_finalUrl = url; }

protected:
    // reimplemented virtual methods
    QString userAgentForUrl(const QUrl &url) const
    {
        return m_userAgent.isEmpty() ?
            QWebPage::userAgentForUrl(url) : m_userAgent;
    }

    bool acceptNavigationRequest(QWebFrame *frame,
                                 const QNetworkRequest &request,
                                 NavigationType type)
    {
        Q_UNUSED(type);

        QUrl url = request.url();
        TRACE() << url;

        /* We generally don't need to load the final URL, so skip loading it.
         * If this behaviour is not desired for some requests, then just avoid
         * calling setFinalUrl() */
        if (url.host() == m_finalUrl.host() &&
            url.path() == m_finalUrl.path()) {
            Q_EMIT finalUrlReached(url);
            return false;
        }

        /* open all new window requests (identified by "frame == 0") in the
         * external browser, as well as other links according to the
         * ExternalLinksPattern and InternalLinksPattern rules. */
        if (frame == 0 || urlIsBlocked(url)) {
            QDesktopServices::openUrl(url);
            return false;
        }
        /* Handle all other requests internally. */
        return true;
    }

Q_SIGNALS:
    void finalUrlReached(const QUrl &url);

private:
    bool urlIsBlocked(QUrl url) const;

private:
    QString m_userAgent;
    QRegExp m_externalLinksPattern;
    QRegExp m_internalLinksPattern;
    QStringList m_allowedSchemes;
    QRegExp m_allowedUrls;
    QUrl m_finalUrl;
};

bool WebPage::urlIsBlocked(QUrl url) const {
    if (!m_allowedSchemes.contains(url.scheme())) {
        TRACE() << "Scheme not allowed:" << url.scheme();
        return true;
    }

    if (!m_allowedUrls.isEmpty() &&
        !m_allowedUrls.exactMatch(url.toString())) {
        TRACE() << "URL not allowed:" << url;
        return true;
    }

    QString urlText = url.toString(QUrl::RemoveScheme |
                                   QUrl::RemoveUserInfo |
                                   QUrl::RemoveFragment |
                                   QUrl::StripTrailingSlash);
    if (urlText.startsWith("//")) {
        urlText = urlText.mid(2);
    }

    if (!m_internalLinksPattern.isEmpty()) {
        return !m_internalLinksPattern.exactMatch(urlText);
    }

    if (!m_externalLinksPattern.isEmpty()) {
        return m_externalLinksPattern.exactMatch(urlText);
    }

    return false;
}

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
    QWidget *buildLoadFailurePage();
    void buildDialog(const QVariantMap &params);
    void start();

private Q_SLOTS:
    void onUrlChanged(const QUrl &url);
    void onLoadProgress();
    void onLoadFinished(bool ok);
    void onFailTimer();
    void onFinished();
    void startProgress();
    void stopProgress();
    void onContentsChanged();

private:
    void showDialog();
    void setupViewForUrl(const QUrl &url);
    void notifyAuthCompleted();
    void notifyLoadFailed();
    QWebElement initializeField(const QString &settingsKey,
                                const QString &paramKey = QString());
    void initializeFields();
    bool tryAutoLogin();
    void addBrowserCookies(CookieJar *cookieJar);

private:
    mutable BrowserRequest *q_ptr;
    /* The dialog can be deleted by the Request class, if it's set as children
     * of an embedded widget which is then deleted. Therefore, in order to
     * avoid a double deletion, guard the pointer with a QPointer. */
    QPointer<Dialog> m_dialog;
    QStackedLayout *m_dialogLayout;
    QWidget *m_webViewPage;
    QWidget *m_successPage;
    QWidget *m_loadFailurePage;
    QStackedLayout *m_webViewLayout;
    WebView *m_webView;
    AnimationLabel *m_animationLabel;
    QUrl finalUrl;
    QUrl responseUrl;
    QString m_host;
    QSettings *m_settings;
    QWebElement m_usernameField;
    QWebElement m_passwordField;
    QWebElement m_loginButton;
    QString m_username;
    QString m_password;
    int m_loginCount;
    QTimer m_failTimer;
};

} // namespace

BrowserRequestPrivate::BrowserRequestPrivate(BrowserRequest *request):
    QObject(request),
    q_ptr(request),
    m_dialog(0),
    m_webViewLayout(0),
    m_webView(0),
    m_animationLabel(0),
    m_settings(0),
    m_loginCount(0)
{
    m_failTimer.setSingleShot(true);
    m_failTimer.setInterval(3000);
    QObject::connect(&m_failTimer, SIGNAL(timeout()),
                     this, SLOT(onFailTimer()));
}

BrowserRequestPrivate::~BrowserRequestPrivate()
{
    delete m_dialog;
}

void BrowserRequestPrivate::onUrlChanged(const QUrl &url)
{
    Q_Q(BrowserRequest);

    TRACE() << "Url changed:" << url;
    m_failTimer.stop();

    if (url.host() == finalUrl.host() &&
        url.path() == finalUrl.path()) {
        responseUrl = url;
        if (q->embeddedUi() || !m_dialog->isVisible()) {
            /* Do not show the notification page. */
            m_dialog->accept();
        } else {
            /* Replace the web page with an information screen */
            notifyAuthCompleted();
        }
    }

    setupViewForUrl(url);
}

void BrowserRequestPrivate::onLoadProgress()
{
    m_failTimer.stop();
}

void BrowserRequestPrivate::onLoadFinished(bool ok)
{
    TRACE() << "Load finished" << ok;

    if (!ok) {
        m_failTimer.start();
        return;
    }

    if (loggingLevel() > 2) {
        /* Dump the HTML */
        TRACE() << m_webView->page()->mainFrame()->toHtml();
    }

    initializeFields();

    if (!m_dialog->isVisible()) {
        if (responseUrl.isEmpty()) {
            if (!tryAutoLogin())
                showDialog();
        } else {
            onFinished();
        }
    }
}

void BrowserRequestPrivate::onFailTimer()
{
    notifyLoadFailed();
}

void BrowserRequestPrivate::addBrowserCookies(CookieJar *cookieJar)
{
    Q_Q(BrowserRequest);

    const QVariantMap &clientData = q->clientData();
    if (!clientData.contains(keyCookies)) return;

    RawCookies rawCookies;
    QDBusArgument arg = clientData[keyCookies].value<QDBusArgument>();
    if (arg.currentSignature() == "a{sv}") {
        /* The signature of the argument should be "a{ss}", not "a{sv}";
         * however, ruby-dbus is rather primitive and there seems to be no way
         * to speficy a different signature than "a{sv}" when marshalling Hash
         * into a variant.
         * Therefore, just for our functional tests, also support "a{sv}".
         */
        QVariantMap cookieMap = qdbus_cast<QVariantMap>(arg);
        QVariantMap::const_iterator i;
        for (i = cookieMap.constBegin(); i != cookieMap.constEnd(); i++) {
            rawCookies.insert(i.key(), i.value().toString());
        }
    } else {
        rawCookies = qdbus_cast<RawCookies>(arg);
    }

    QList<QNetworkCookie> cookies;
    RawCookies::const_iterator i;
    for (i = rawCookies.constBegin(); i != rawCookies.constEnd(); i++) {
        const QString &host = i.key();
        QStringList cookieList = i.value().split(";");
        foreach (const QString &cookieSpec, cookieList) {
            int semicolon = cookieSpec.indexOf("=");
            QNetworkCookie cookie(cookieSpec.left(semicolon).toUtf8(),
                                  cookieSpec.mid(semicolon + 1).toUtf8());
            cookie.setDomain(host);
            cookie.setPath("/");
            cookies.append(cookie);
        }
    }

    TRACE() << "cookies:" << cookies;
    cookieJar->setCookies(cookies);
}

void BrowserRequestPrivate::startProgress()
{
    m_animationLabel->start();
    m_webViewLayout->setCurrentWidget(m_animationLabel);
}

void BrowserRequestPrivate::stopProgress()
{
    m_animationLabel->stop();
    m_webViewLayout->setCurrentWidget(m_webView);
}

QWidget *BrowserRequestPrivate::buildWebViewPage(const QVariantMap &params)
{
    Q_Q(BrowserRequest);

    QWidget *dialogPage = new QWidget;
    m_webViewLayout = new QStackedLayout(dialogPage);

    m_webView = new WebView();
    WebPage *page = new WebPage(this);
    QObject::connect(page, SIGNAL(contentsChanged()),
                     this, SLOT(onContentsChanged()));
    m_webView->setPage(page);

    /* The following couple of lines serve to instruct the QWebPage not to load
     * the final URL, but to block it and emit the finalUrlReached() signal
     * instead.
     */
    page->setFinalUrl(finalUrl);
    QObject::connect(page, SIGNAL(finalUrlReached(const QUrl&)),
                     this, SLOT(onUrlChanged(const QUrl&)));

    /* set a per-identity cookie jar on the page */
    uint identity = q->identity();
    CookieJarManager *cookieJarManager = CookieJarManager::instance();
    CookieJar *cookieJar = cookieJarManager->cookieJarForIdentity(identity);
    addBrowserCookies(cookieJar);
    page->networkAccessManager()->setCookieJar(cookieJar);
    /* NetworkAccessManager takes ownership of the cookieJar; we don't want
     * this */
    cookieJar->setParent(cookieJarManager);

    const QVariantMap &clientData = q->clientData();
    if (clientData.contains(keyAllowedSchemes)) {
        page->setAllowedSchemes(clientData[keyAllowedSchemes].toStringList());
    } else {
        /* by default, allow only https */
        page->setAllowedSchemes(QStringList("https"));
    }

    QUrl url(params.value(SSOUI_KEY_OPENURL).toString());
    setupViewForUrl(url);
    QObject::connect(m_webView, SIGNAL(urlChanged(const QUrl&)),
                     this, SLOT(onUrlChanged(const QUrl&)));
    QObject::connect(m_webView, SIGNAL(loadProgress(int)),
                     this, SLOT(onLoadProgress()));
    QObject::connect(m_webView, SIGNAL(loadFinished(bool)),
                     this, SLOT(onLoadFinished(bool)));
    m_webViewLayout->addWidget(m_webView);

    m_animationLabel = new AnimationLabel(":/spinner-26.gif", 0);
    QObject::connect(m_webView, SIGNAL(loadStarted()),
                     this, SLOT(startProgress()));
    QObject::connect(m_webView, SIGNAL(loadFinished(bool)),
                     this, SLOT(stopProgress()));
    m_webViewLayout->addWidget(m_animationLabel);
    m_webView->setUrl(url);

    return dialogPage;
}

QWidget *BrowserRequestPrivate::buildSuccessPage()
{
    QWidget *dialogPage = new QWidget;
    dialogPage->setSizePolicy(QSizePolicy::Ignored,
                              QSizePolicy::MinimumExpanding);
    QVBoxLayout *layout = new QVBoxLayout(dialogPage);

    QLabel *label = new QLabel(_("The authentication process is complete.\n"
                                 "You may now close this dialog "
                                 "and return to the application."));
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);

    QPushButton *doneButton = new QPushButton(_("Done"));
    doneButton->setDefault(true);
    QObject::connect(doneButton, SIGNAL(clicked()),
                     m_dialog, SLOT(accept()));
    layout->addWidget(doneButton);

    return dialogPage;
}

QWidget *BrowserRequestPrivate::buildLoadFailurePage()
{
    QWidget *dialogPage = new QWidget;
    dialogPage->setSizePolicy(QSizePolicy::Ignored,
                              QSizePolicy::MinimumExpanding);
    QVBoxLayout *layout = new QVBoxLayout(dialogPage);

    QLabel *label = new QLabel(_("An error occurred while loading "
                                 "the authentication page."));
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);

    return dialogPage;
}

void BrowserRequestPrivate::buildDialog(const QVariantMap &params)
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

    m_dialog->setWindowTitle(title);

    m_dialogLayout = new QStackedLayout(m_dialog);

    m_webViewPage = buildWebViewPage(params);
    m_dialogLayout->addWidget(m_webViewPage);

    m_successPage = buildSuccessPage();
    m_dialogLayout->addWidget(m_successPage);

    m_loadFailurePage = buildLoadFailurePage();
    m_dialogLayout->addWidget(m_loadFailurePage);

    TRACE() << "Dialog was built";
}

void BrowserRequestPrivate::start()
{
    Q_Q(BrowserRequest);

    finalUrl = QUrl(q->parameters().value(SSOUI_KEY_FINALURL).toString());
    buildDialog(q->parameters());

    QObject::connect(m_dialog, SIGNAL(finished(int)),
                     this, SLOT(onFinished()));

    if (q->embeddedUi()) {
        showDialog();
    }
}

void BrowserRequestPrivate::onFinished()
{
    Q_Q(BrowserRequest);

    TRACE() << "Browser dialog closed";

    QObject::disconnect(m_webView, 0, this, 0);

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

static Qt::ScrollBarPolicy scrollPolicyFromValue(const QVariant &value)
{
    QString stringValue = value.toString();
    if (stringValue == valueAlwaysOn) {
        return Qt::ScrollBarAlwaysOn;
    } else if (stringValue == valueAlwaysOff) {
        return Qt::ScrollBarAlwaysOff;
    } else {
        return Qt::ScrollBarAsNeeded;
    }
}

void BrowserRequestPrivate::setupViewForUrl(const QUrl &url)
{
    QString host = url.host();
    if (host == m_host) return;

    m_host = host;

    /* Load the host-specific configuration file */
    delete m_settings;
    m_settings = new QSettings("signon-ui/webkit-options.d/" + host, QString(), this);

    WebPage *page = qobject_cast<WebPage *>(m_webView->page());

    if (m_settings->contains(keyViewportWidth) &&
        m_settings->contains(keyViewportHeight)) {
        QSize viewportSize(m_settings->value(keyViewportWidth).toInt(),
                           m_settings->value(keyViewportHeight).toInt());
        m_webView->setPreferredSize(viewportSize);
    }

    if (m_settings->contains(keyPreferredWidth)) {
        QSize preferredSize(m_settings->value(keyPreferredWidth).toInt(), 300);
        page->setPreferredContentsSize(preferredSize);
    }

    if (m_settings->contains(keyTextSizeMultiplier)) {
        m_webView->setTextSizeMultiplier(m_settings->value(keyTextSizeMultiplier).
                                         toReal());
    }

    if (m_settings->contains(keyUserAgent)) {
        page->setUserAgent(m_settings->value(keyUserAgent).toString());
    }

    if (m_settings->contains(keyZoomFactor)) {
        m_webView->setZoomFactor(m_settings->value(keyZoomFactor).toReal());
    }

    if (m_settings->contains(keyHorizontalScrollBar)) {
        Qt::ScrollBarPolicy policy =
            scrollPolicyFromValue(m_settings->value(keyHorizontalScrollBar));
        page->mainFrame()->setScrollBarPolicy(Qt::Horizontal, policy);
    }

    if (m_settings->contains(keyVerticalScrollBar)) {
        Qt::ScrollBarPolicy policy =
            scrollPolicyFromValue(m_settings->value(keyVerticalScrollBar));
        page->mainFrame()->setScrollBarPolicy(Qt::Vertical, policy);
    }

    page->setExternalLinksPattern(m_settings->value(keyExternalLinksPattern).
                                  toString());
    page->setInternalLinksPattern(m_settings->value(keyInternalLinksPattern).
                                  toString());
    page->setAllowedUrls(m_settings->value(keyAllowedUrls).toString());
}

void BrowserRequestPrivate::notifyAuthCompleted()
{
    /* Ignore any webview signals from now on.
     * This is needed because QWebView might still emit loadFinished(false)
     * (which we would interpret as an error) on the final URL, which we don't
     * care about anymore. */
    QObject::disconnect(m_webView, 0, this, 0);

    m_dialogLayout->setCurrentWidget(m_successPage);
}

void BrowserRequestPrivate::notifyLoadFailed()
{
    m_dialogLayout->setCurrentWidget(m_loadFailurePage);
    showDialog();
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
        if (!paramKey.isEmpty() && params.contains(paramKey)) {
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
    m_loginButton = initializeField(keyLoginButton);
}

bool BrowserRequestPrivate::tryAutoLogin()
{
    if (m_loginButton.isNull()) return false;

    if (m_usernameField.isNull() ||
        m_usernameField.evaluateJavaScript("this.value").isNull())
        return false;

    if (m_passwordField.isNull() ||
        m_passwordField.evaluateJavaScript("this.value").isNull())
        return false;

    /* Avoid falling in a failed login loop */
    m_loginCount++;
    if (m_loginCount > 1)
        return false;

    m_loginButton.evaluateJavaScript("this.click()");
    return true;
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
