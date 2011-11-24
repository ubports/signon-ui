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
#ifndef SIGNON_UI_REQUEST_H
#define SIGNON_UI_REQUEST_H

#include <QDBusConnection>
#include <QDBusMessage>
#include <QObject>
#include <QVariantMap>
#include <QWidget>

class Request: public QObject
{
    Q_OBJECT

public:
    explicit Request(const QDBusConnection &connection,
                     const QDBusMessage &message,
                     const QVariantMap &parameters,
                     QObject *parent = 0);
    ~Request();

    static QString id(const QVariantMap &parameters);
    QString id() const;

    WId windowId() const;

    bool isInProgress() const;

public Q_SLOTS:
    void start();

Q_SIGNALS:
    void completed();

private:
    const QDBusConnection &m_connection;
    const QDBusMessage &m_message;
    QVariantMap m_parameters;
    bool m_inProgress;
};

#endif // SIGNON_UI_REQUEST_H

