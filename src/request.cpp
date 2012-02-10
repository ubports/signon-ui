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

#include "request.h"

#include "browser-request.h"
#include "debug.h"
#include "dialog-request.h"
#include "errors.h"

#include <QDBusArgument>
#include <QX11EmbedWidget>
#include <QX11Info>
#include <SignOn/uisessiondata.h>
#include <SignOn/uisessiondata_priv.h>
#include <X11/Xlib.h>

using namespace SignOnUi;

namespace SignOnUi {

class RequestPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Request)

public:
    RequestPrivate(const QDBusConnection &connection,
                   const QDBusMessage &message,
                   const QVariantMap &parameters,
                   Request *request);
    ~RequestPrivate();

    WId windowId() const {
        return m_clientData[SSOUI_KEY_WINDOWID].toUInt();
    }

    bool embeddedUi() const {
        return m_clientData[SSOUI_KEY_EMBEDDED].toBool();
    }

private Q_SLOTS:
    void onEmbedError();

private:
    void setWidget(QWidget *widget) const;

private:
    mutable Request *q_ptr;
    QDBusConnection m_connection;
    QDBusMessage m_message;
    QVariantMap m_parameters;
    QVariantMap m_clientData;
    bool m_inProgress;
};

} // namespace

RequestPrivate::RequestPrivate(const QDBusConnection &connection,
                               const QDBusMessage &message,
                               const QVariantMap &parameters,
                               Request *request):
    QObject(request),
    q_ptr(request),
    m_connection(connection),
    m_message(message),
    m_parameters(parameters),
    m_inProgress(false)
{
    if (parameters.contains(SSOUI_KEY_CLIENT_DATA)) {
        QVariant variant = parameters[SSOUI_KEY_CLIENT_DATA];
        m_clientData = qdbus_cast<QVariantMap>(variant.value<QDBusArgument>());
    }
}

RequestPrivate::~RequestPrivate()
{
}

void RequestPrivate::setWidget(QWidget *widget) const
{
    if (embeddedUi() && windowId() != 0) {
        TRACE() << "Requesting widget embedding";
        QX11EmbedWidget *embed = new QX11EmbedWidget;
        QObject::connect(embed, SIGNAL(error(QX11EmbedWidget::Error)),
                         this, SLOT(onEmbedError()));
        QObject::connect(embed, SIGNAL(containerClosed()),
                         widget, SLOT(close()));
        QObject::connect(embed, SIGNAL(containerClosed()),
                         embed, SLOT(deleteLater()));
        embed->embedInto(windowId());
        widget->setParent(embed);
        widget->show();
        embed->show();
        return;
    }

    widget->setWindowModality(Qt::WindowModal);
    widget->show();
    if (windowId() != 0) {
        TRACE() << "Setting" << widget->effectiveWinId() << "transient for" << windowId();
        XSetTransientForHint(QX11Info::display(),
                             widget->effectiveWinId(),
                             windowId());
    }
}

void RequestPrivate::onEmbedError()
{
    Q_Q(Request);

    QX11EmbedWidget *embed = qobject_cast<QX11EmbedWidget*>(sender());
    TRACE() << "Embed error:" << embed->error();

    q->fail(SIGNON_UI_ERROR_EMBEDDING_FAILED,
            QString("Embedding signon UI failed: %1").arg(embed->error()));
}

Request *Request::newRequest(const QDBusConnection &connection,
                             const QDBusMessage &message,
                             const QVariantMap &parameters,
                             QObject *parent)
{
    if (parameters.contains(SSOUI_KEY_OPENURL)) {
        return new BrowserRequest(connection, message, parameters, parent);
    } else {
        return new DialogRequest(connection, message, parameters, parent);
    }
}

Request::Request(const QDBusConnection &connection,
                 const QDBusMessage &message,
                 const QVariantMap &parameters,
                 QObject *parent):
    QObject(parent),
    d_ptr(new RequestPrivate(connection, message, parameters, this))
{
}

Request::~Request()
{
}

QString Request::id(const QVariantMap &parameters)
{
    return parameters[SSOUI_KEY_REQUESTID].toString();
}

QString Request::id() const
{
    Q_D(const Request);
    return Request::id(d->m_parameters);
}

void Request::setWidget(QWidget *widget) const
{
    Q_D(const Request);
    d->setWidget(widget);
}

WId Request::windowId() const
{
    Q_D(const Request);
    return d->windowId();
}

bool Request::embeddedUi() const
{
    Q_D(const Request);
    return d->embeddedUi();
}

bool Request::isInProgress() const
{
    Q_D(const Request);
    return d->m_inProgress;
}

const QVariantMap &Request::parameters() const
{
    Q_D(const Request);
    return d->m_parameters;
}

const QVariantMap &Request::clientData() const
{
    Q_D(const Request);
    return d->m_clientData;
}

void Request::start()
{
    Q_D(Request);
    if (d->m_inProgress) {
        BLAME() << "Request already started!";
        return;
    }
    d->m_inProgress = true;
}

void Request::fail(const QString &name, const QString &message)
{
    Q_D(Request);
    QDBusMessage reply = d->m_message.createErrorReply(name, message);
    d->m_connection.send(reply);

    Q_EMIT completed();
}

void Request::setCanceled()
{
    QVariantMap result;
    result[SSOUI_KEY_ERROR] = SignOn::QUERY_ERROR_CANCELED;

    setResult(result);
}

void Request::setResult(const QVariantMap &result)
{
    Q_D(Request);
    QDBusMessage reply = d->m_message.createReply(result);
    d->m_connection.send(reply);

    Q_EMIT completed();
}

#include "request.moc"
