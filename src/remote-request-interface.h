/*
 * This file is part of signon-ui
 *
 * Copyright (C) 2013 Canonical Ltd.
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

#ifndef SIGNON_UI_REMOTE_REQUEST_INTERFACE_H
#define SIGNON_UI_REMOTE_REQUEST_INTERFACE_H

#include <QIODevice>
#include <QVariantMap>

namespace SignOnUi {

class RemoteRequestClientPrivate;
class RemoteRequestClient: public QObject
{
    Q_OBJECT

public:
    RemoteRequestClient(QObject *parent = 0);
    ~RemoteRequestClient();

    void setChannels(QIODevice *readChannel, QIODevice *writeChannel);

    void start(const QVariantMap &parameters);
    void cancel();

Q_SIGNALS:
    void result(const QVariantMap &result);
    void canceled();

private:
    RemoteRequestClientPrivate *d_ptr;
    Q_DECLARE_PRIVATE(RemoteRequestClient)
};

class RemoteRequestServerPrivate;
class RemoteRequestServer: public QObject
{
    Q_OBJECT

public:
    RemoteRequestServer(QObject *parent = 0);
    ~RemoteRequestServer();

    void setChannels(QIODevice *readChannel, QIODevice *writeChannel);

    void setResult(const QVariantMap &result);
    void setCanceled();

Q_SIGNALS:
    void started(const QVariantMap &parameters);
    void canceled();

private:
    RemoteRequestServerPrivate *d_ptr;
    Q_DECLARE_PRIVATE(RemoteRequestServer)
};

} // namespace

#endif // SIGNON_UI_REMOTE_REQUEST_INTERFACE_H
