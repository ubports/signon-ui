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

#include "debug.h"

#include <SignOn/uisessiondata_priv.h>

Request::Request(const QDBusConnection &connection,
                 const QDBusMessage &message,
                 const QVariantMap &parameters,
                 QObject *parent):
    QObject(parent),
    m_connection(connection),
    m_message(message),
    m_parameters(parameters)
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

WId Request::windowId() const
{
    return m_parameters[SSOUI_KEY_WINDOWID].toUInt();
}

bool Request::isInProgress() const
{
    return m_inProgress;
}

void Request::start()
{
    if (m_inProgress) {
        BLAME() << "Request already started!";
        return;
    }
    m_inProgress = true;

    // TODO
    Q_EMIT completed();
}

