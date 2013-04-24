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

#ifndef SIGNON_UI_COOKIE_JAR_MANAGER_H
#define SIGNON_UI_COOKIE_JAR_MANAGER_H

#include <QMap>
#include <QNetworkCookieJar>
#include <QObject>
#include <QString>
#include <QTimer>

namespace SignOnUi {

typedef QMap<QString,QString> RawCookies;

class CookieJar: public QNetworkCookieJar
{
    Q_OBJECT

public:
    CookieJar(QString cookiePath, QObject *parent = 0);
    ~CookieJar() {}

    QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const;
    bool setCookiesFromUrl(const QList<QNetworkCookie> &cookieList,
                           const QUrl &url);
    void setCookies(const QList<QNetworkCookie> &cookieList) {
        queueSave();
        setAllCookies(cookieList);
    }

public Q_SLOTS:
    void save();

private:
    void queueSave();

private:
    QString m_cookiePath;
    QTimer m_saveTimer;
};

class CookieJarManagerPrivate;

class CookieJarManager: public QObject
{
    Q_OBJECT

public:
    ~CookieJarManager();

    static CookieJarManager *instance();

    CookieJar *cookieJarForIdentity(uint id);
    void removeForIdentity(uint id);

public Q_SLOTS:
    void saveAll();

protected:
    explicit CookieJarManager(QObject *parent = 0);

private:
    CookieJarManagerPrivate *d_ptr;
    Q_DECLARE_PRIVATE(CookieJarManager)
};

} // namespace

Q_DECLARE_METATYPE(SignOnUi::RawCookies)

#endif // SIGNON_UI_COOKIE_JAR_MANAGER_H

