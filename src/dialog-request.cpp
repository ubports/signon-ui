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

#include "dialog-request.h"

#include "debug.h"
#include "dialog.h"
#include "network-access-manager.h"

#include <QDialogButtonBox>
#include <QEventLoop>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <SignOn/UiSessionData>
#include <SignOn/uisessiondata_priv.h>

using namespace SignOnUi;
using namespace SignOn;

namespace SignOnUi {

class DialogRequestPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(DialogRequest)

public:
    DialogRequestPrivate(DialogRequest *request);
    ~DialogRequestPrivate();

    void buildDialog(const QVariantMap &params);
    void start();

private Q_SLOTS:
    void onAccepted();
    void onRejected();
    void onCaptchaRetrieved(QNetworkReply *reply);

private:
    QString messageFromId(int id);
    void requestCaptcha(const QUrl &url);

private:
    mutable DialogRequest *q_ptr;
    Dialog *m_dialog;
    bool m_queryUsername;
    bool m_queryPassword;
    QLineEdit *m_wUsername;
    QLineEdit *m_wPassword;
    QLineEdit *m_wCaptchaText;
    QLabel *m_wCaptcha;
    QNetworkAccessManager *m_networkAccessManager;
};

} // namespace

DialogRequestPrivate::DialogRequestPrivate(DialogRequest *request):
    QObject(request),
    q_ptr(request),
    m_dialog(0),
    m_queryUsername(false),
    m_queryPassword(false),
    m_wUsername(0),
    m_wPassword(0),
    m_wCaptchaText(0),
    m_wCaptcha(0),
    m_networkAccessManager(0)
{
}

DialogRequestPrivate::~DialogRequestPrivate()
{
    delete m_dialog;
}

QString DialogRequestPrivate::messageFromId(int id)
{
    static QString error = QLatin1String("<font color='red'>%1</font>");
    static QString msg = QLatin1String("<i>%1</i>");

    switch (id) {
    case QUERY_MESSAGE_LOGIN:
        return msg.arg(tr("Enter your credentials to login"));
    case QUERY_MESSAGE_NOT_AUTHORIZED:
        return error.arg(tr("Previous authentication attempt failed. Please try again."));
    case QUERY_MESSAGE_EMPTY:
    default:
        return QString();
    }
}

void DialogRequestPrivate::requestCaptcha(const QUrl &url)
{
    TRACE() << url;

    if (m_networkAccessManager == 0) {
        m_networkAccessManager = NetworkAccessManager::instance();
    }

    QNetworkRequest request = QNetworkRequest(url);
    QNetworkReply *reply = m_networkAccessManager->get(request);
    if (reply->isFinished()) {
        onCaptchaRetrieved(reply);
    } else {
        // FIXME: handle download asynchronously
        QEventLoop loop;
        QObject::connect(reply, SIGNAL(finished()),
                         &loop, SLOT(quit()));
        loop.exec();
        onCaptchaRetrieved(reply);
    }
}

void DialogRequestPrivate::buildDialog(const QVariantMap &params)
{
    m_dialog = new Dialog;
    m_dialog->setMinimumWidth(400);

    QString title = params.value(SSOUI_KEY_TITLE,
                                 tr("Enter your credentials")).toString();
    m_dialog->setWindowTitle(title);

    QFormLayout *formLayout = new QFormLayout(m_dialog);

    QString message = params.value(SSOUI_KEY_MESSAGE).toString();
    if (message.isEmpty()) {
        // Check whether a predefined message id is set
        if (params.contains(SSOUI_KEY_MESSAGEID)) {
            message = messageFromId(params.value(SSOUI_KEY_MESSAGEID).toInt());
        }
    }
    if (!message.isEmpty()) {
        QLabel *wMessage = new QLabel(message);
        formLayout->addRow(wMessage);
    }

    m_queryUsername = params.value(SSOUI_KEY_QUERYUSERNAME, false).toBool();
    bool showUsername = m_queryUsername || params.contains(SSOUI_KEY_USERNAME);
    if (showUsername) {
        m_wUsername = new QLineEdit;
        m_wUsername->setEnabled(m_queryUsername);
        m_wUsername->setAccessibleName("username");
        m_wUsername->setText(params.value(SSOUI_KEY_USERNAME).toString());
        formLayout->addRow(tr("Username:"), m_wUsername);
    }

    m_queryPassword = params.value(SSOUI_KEY_QUERYPASSWORD, false).toBool();
    bool showPassword = m_queryPassword || params.contains(SSOUI_KEY_PASSWORD);
    if (showPassword) {
        m_wPassword = new QLineEdit;
        m_wPassword->setEnabled(m_queryPassword);
        m_wPassword->setEchoMode(QLineEdit::Password);
        m_wPassword->setText(params.value(SSOUI_KEY_PASSWORD).toString());
        formLayout->addRow(tr("Password:"), m_wPassword);
    }

    QString captchaUrl = params.value(SSOUI_KEY_CAPTCHAURL).toString();
    if (!captchaUrl.isEmpty()) {
        m_wCaptcha = new QLabel;
        formLayout->addRow(m_wCaptcha);
        m_wCaptchaText = new QLineEdit;
        formLayout->addRow(m_wCaptchaText);

        requestCaptcha(QUrl::fromEncoded(captchaUrl.toLatin1()));
    }

    QDialogButtonBox *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), m_dialog, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), m_dialog, SLOT(reject()));
    formLayout->addRow(buttonBox);

    TRACE() << "Dialog was built";
}

void DialogRequestPrivate::start()
{
    Q_Q(DialogRequest);

    buildDialog(q->parameters());
    q->setWidget(m_dialog);

    QObject::connect(m_dialog, SIGNAL(accepted()),
                     this, SLOT(onAccepted()));
    QObject::connect(m_dialog, SIGNAL(rejected()),
                     this, SLOT(onRejected()));
}

void DialogRequestPrivate::onAccepted()
{
    Q_Q(DialogRequest);

    TRACE() << "Dialog is accepted";

    QVariantMap reply;

    if (m_queryUsername) {
        Q_ASSERT(m_wUsername != 0);
        reply[SSOUI_KEY_USERNAME] = m_wUsername->text();
    }
    if (m_queryPassword) {
        Q_ASSERT(m_wPassword != 0);
        reply[SSOUI_KEY_PASSWORD] = m_wPassword->text();
    }
    if (m_wCaptchaText != 0) {
        reply[SSOUI_KEY_CAPTCHARESP] = m_wCaptchaText->text();
    }

    q->setResult(reply);
}

void DialogRequestPrivate::onRejected()
{
    Q_Q(DialogRequest);

    TRACE() << "Dialog is rejected";
    q->setCanceled();
}

void DialogRequestPrivate::onCaptchaRetrieved(QNetworkReply *reply)
{
    TRACE() << "Got captcha";

    reply->deleteLater();

    if (reply->error()) {
        // TODO handle error
        TRACE() << "Got error:" << reply->errorString();
        return;
    }

    QUrl newUrl =
        reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (newUrl.isEmpty()) {
        QByteArray captchaData = reply->readAll();
        QPixmap pixmap;
        pixmap.loadFromData(captchaData);
        m_wCaptcha->setPixmap(pixmap);
    } else {
        QUrl url = reply->url().resolved(newUrl);
        requestCaptcha(url);
    }
}

DialogRequest::DialogRequest(const QDBusConnection &connection,
                             const QDBusMessage &message,
                             const QVariantMap &parameters,
                             QObject *parent):
    Request(connection, message, parameters, parent),
    d_ptr(new DialogRequestPrivate(this))
{
}

DialogRequest::~DialogRequest()
{
}

void DialogRequest::start()
{
    Q_D(DialogRequest);

    Request::start();
    d->start();
}

#include "dialog-request.moc"
