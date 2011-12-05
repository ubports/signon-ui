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

#include "network-access-manager.h"

#include "debug.h"

using namespace SignOnUi;

static NetworkAccessManager *m_instance = 0;

/* At the moment the only reason for using this class is reusing the NAM across
 * browser requests, in order to reuse the cookie jar.
 * We might want to add here proxy settings, or specialized cookie jars
 * integrated with the user's desktop browser.
 */
NetworkAccessManager::NetworkAccessManager(QObject *parent):
    QNetworkAccessManager(parent)
{
}

NetworkAccessManager::~NetworkAccessManager()
{
}

NetworkAccessManager *NetworkAccessManager::instance()
{
    if (m_instance == 0) {
        m_instance = new NetworkAccessManager();
    }

    return m_instance;
}

