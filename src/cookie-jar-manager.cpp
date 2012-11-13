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

#define QT_DISABLE_DEPRECATED_BEFORE QT_VERSION_CHECK(4, 0, 0)

#include "cookie-jar-manager.h"

#include "debug.h"

#include <QCoreApplication>
#include <QDBusMetaType>
#include <QDataStream>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QNetworkCookie>

using namespace SignOnUi;

static CookieJarManager *m_instance = 0;
static const unsigned int JAR_VERSION = 1;

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
    queueSave();
    return QNetworkCookieJar::setCookiesFromUrl(cookieList, url);
}

class CookieJarManagerPrivate
{
    Q_DECLARE_PUBLIC(CookieJarManager)

private:
    mutable CookieJarManager *q_ptr;
    QHash<quint32, CookieJar*> cookieJars;
    QDir cookieDir;
};

} // namespace

QDataStream &operator<<(QDataStream &stream,
                        const QList<QNetworkCookie> &list)
{
    stream << JAR_VERSION;
    stream << quint32(list.size());
    foreach (const QNetworkCookie &cookie, list) {
        stream << cookie.toRawForm();
    }
    return stream;
}

QDataStream &operator>>(QDataStream &stream,
                        QList<QNetworkCookie> &list)
{
    list.clear();

    quint32 version;
    stream >> version;

    if (version != JAR_VERSION)
        return stream;

    quint32 count;
    stream >> count;
    for (quint32 i = 0; i < count; i++)
    {
        QByteArray value;
        stream >> value;
        QList<QNetworkCookie> newCookies = QNetworkCookie::parseCookies(value);
        if (newCookies.count() == 0 && value.length() != 0) {
            qWarning() << "CookieJar: Unable to parse saved cookie:" << value;
        }
        foreach (const QNetworkCookie &cookie, newCookies) {
            list.append(cookie);
        }
        if (stream.atEnd())
            break;
    }
    return stream;
}

CookieJar::CookieJar(QString cookiePath, QObject *parent):
    QNetworkCookieJar(parent),
    m_cookiePath(cookiePath)
{
    // Prepare the auto-save timer
    m_saveTimer.setInterval(10 * 1000);
    m_saveTimer.setSingleShot(true);
    QObject::connect(&m_saveTimer, SIGNAL(timeout()),
                     this, SLOT(save()));

    // Load any saved cookies
    QFile file(m_cookiePath);
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);

    QList<QNetworkCookie> cookies;
    in >> cookies;
    setAllCookies(cookies);
}

void CookieJar::save()
{
    TRACE() << "saving to" << m_cookiePath;
    QFile file(m_cookiePath);
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);

    out << allCookies();

    /* clear any running timer */
    m_saveTimer.stop();
}

void CookieJar::queueSave()
{
    m_saveTimer.start();
}

CookieJarManager::CookieJarManager(QObject *parent):
    QObject(parent),
    d_ptr(new CookieJarManagerPrivate)
{
    Q_D(CookieJarManager);

    qDBusRegisterMetaType<RawCookies>();
    qRegisterMetaTypeStreamOperators<QList<QNetworkCookie> >("QList<QNetworkCookie>");

    d->cookieDir =
        QDesktopServices::storageLocation(QDesktopServices::CacheLocation) +
        QDir::separator() + "cookies";
    if (!d->cookieDir.exists()) {
        d->cookieDir.mkpath(".");
    }

    QObject::connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()),
                     this, SLOT(saveAll()));
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
        QString fileName = QString::fromLatin1("%1.jar").arg(id);
        CookieJar *cookieJar =
            new CookieJar(d->cookieDir.absoluteFilePath(fileName), this);
        d->cookieJars.insert(id, cookieJar);
        return cookieJar;
    }
}

void CookieJarManager::saveAll()
{
    Q_D(CookieJarManager);
    foreach (CookieJar *jar, d->cookieJars) {
        jar->save();
    }
}
