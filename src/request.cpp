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

#include <QX11Info>
#include <SignOn/uisessiondata.h>
#include <SignOn/uisessiondata_priv.h>
#include <X11/Xlib.h>

using namespace SignOnUi;

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
    m_connection(connection),
    m_message(message),
    m_parameters(parameters),
    m_inProgress(false)
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
    return Request::id(m_parameters);
}

void Request::setWidget(QWidget *widget) const
{
    widget->setWindowModality(Qt::WindowModal);
    widget->show();
    if (windowId() != 0) {
        TRACE() << "Setting" << widget->effectiveWinId() << "transient for" << windowId();
        XSetTransientForHint(QX11Info::display(),
                             widget->effectiveWinId(),
                             windowId());
    }
}

WId Request::windowId() const
{
    return m_parameters[SSOUI_KEY_WINDOWID].toUInt();
}

bool Request::isInProgress() const
{
    return m_inProgress;
}

const QVariantMap &Request::parameters() const
{
    return m_parameters;
}

void Request::start()
{
    if (m_inProgress) {
        BLAME() << "Request already started!";
        return;
    }
    m_inProgress = true;
}

void Request::fail(const QString &name, const QString &message)
{
    QDBusMessage reply = m_message.createErrorReply(name, message);
    m_connection.send(reply);

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
    QDBusMessage reply = m_message.createReply(result);
    m_connection.send(reply);

    Q_EMIT completed();
}

