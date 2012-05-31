/*
 * This file is part of signon-ui
 *
 * Copyright (C) 2012 Canonical Ltd.
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

#include "cookie-jar-manager.h"

#include "debug.h"

#include <QDBusMetaType>
#include <QHash>

using namespace SignOnUi;

static CookieJarManager *m_instance = 0;

namespace SignOnUi {

QList<QNetworkCookie> CookieJar::cookiesForUrl(const QUrl &url) const
{
    return QNetworkCookieJar::cookiesForUrl(url);
}

bool CookieJar::setCookiesFromUrl(const QList<QNetworkCookie> &cookieList,
                                  const QUrl &url)
{
    TRACE() << "Setting cookies for url:" << url;
    TRACE() << cookieList;
    return QNetworkCookieJar::setCookiesFromUrl(cookieList, url);
}

class CookieJarManagerPrivate
{
    Q_DECLARE_PUBLIC(CookieJarManager)

private:
    mutable CookieJarManager *q_ptr;
    QHash<quint32, CookieJar*> cookieJars;
};

} // namespace

CookieJarManager::CookieJarManager(QObject *parent):
    QObject(parent),
    d_ptr(new CookieJarManagerPrivate)
{
    qDBusRegisterMetaType<RawCookies>();
}

CookieJarManager::~CookieJarManager()
{
    delete d_ptr;
}

CookieJarManager *CookieJarManager::instance()
{
    if (m_instance == 0) {
        m_instance = new CookieJarManager();
    }

    return m_instance;
}

CookieJar *CookieJarManager::cookieJarForIdentity(uint id)
{
    Q_D(CookieJarManager);

    if (d->cookieJars.contains(id)) {
        return d->cookieJars[id];
    } else {
        CookieJar *cookieJar = new CookieJar(this);
        d->cookieJars.insert(id, cookieJar);
        return cookieJar;
    }
}

