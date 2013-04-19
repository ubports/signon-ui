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

#include "remote-request-interface.h"

#include "debug.h"

#include <QByteArray>
#include <QDataStream>
#include <QFile>
#include <QSocketNotifier>

using namespace SignOnUi;

namespace SignOnUi {

class IpcHandler: public QObject
{
    Q_OBJECT

public:
    enum Code {
        Start = 1,
        Cancel,
        SetResult,
        SetCanceled,
    };

    IpcHandler();
    ~IpcHandler();

    void setChannels(QIODevice *readChannel, QIODevice *writeChannel);
    void write(const QByteArray &data);

Q_SIGNALS:
    void dataReady(const QByteArray &data);

private Q_SLOTS:
    void onReadyRead();

private:
    QIODevice *m_readChannel;
    QIODevice *m_writeChannel;
    int m_expectedLength;
    QByteArray m_readBuffer;
};

} // namespace

IpcHandler::IpcHandler():
    QObject(),
    m_readChannel(0),
    m_writeChannel(0),
    m_expectedLength(0)
{
}

IpcHandler::~IpcHandler()
{
}

void IpcHandler::setChannels(QIODevice *readChannel, QIODevice *writeChannel)
{
    m_readChannel = readChannel;
    m_writeChannel = writeChannel;
    QObject::connect(m_readChannel, SIGNAL(readyRead()),
                     this, SLOT(onReadyRead()));
    /* QFile need special handling */
    QFile *file = qobject_cast<QFile*>(m_readChannel);
    if (file != 0) {
        QSocketNotifier *notifier = new QSocketNotifier(file->handle(),
                                                        QSocketNotifier::Read,
                                                        this);
        QObject::connect(notifier, SIGNAL(activated(int)),
                         this, SLOT(onReadyRead()));
    }
    onReadyRead();
}

void IpcHandler::write(const QByteArray &data)
{
    int length = data.count();
    m_writeChannel->write((char *)&length, sizeof(length));
    m_writeChannel->write(data);
}

void IpcHandler::onReadyRead()
{
    while (true) {
        if (m_expectedLength == 0) {
            /* We are beginning a new read */
            int length;
            int bytesRead = m_readChannel->read((char *)&length,
                                                sizeof(length));
            if (bytesRead < int(sizeof(length))) break;
            m_expectedLength = length;
            m_readBuffer.clear();
        }

        int neededBytes = m_expectedLength - m_readBuffer.length();
        QByteArray buffer = m_readChannel->read(neededBytes);
        m_readBuffer += buffer;
        if (buffer.length() < neededBytes) break;
        if (m_readBuffer.length() == m_expectedLength) {
            Q_EMIT dataReady(m_readBuffer);
            m_expectedLength = 0;
        }
    }
}

namespace SignOnUi {
class RemoteRequestClientPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(RemoteRequestClient)

public:
    RemoteRequestClientPrivate(RemoteRequestClient *client);
    ~RemoteRequestClientPrivate() {};

private Q_SLOTS:
    void onDataReady(const QByteArray &data);

private:
    IpcHandler m_handler;
    mutable RemoteRequestClient *q_ptr;
};
} // namespace

RemoteRequestClientPrivate::RemoteRequestClientPrivate(RemoteRequestClient *client):
    QObject(client),
    q_ptr(client)
{
    QObject::connect(&m_handler, SIGNAL(dataReady(const QByteArray&)),
                     this, SLOT(onDataReady(const QByteArray&)));
}

void RemoteRequestClientPrivate::onDataReady(const QByteArray &data)
{
    Q_Q(RemoteRequestClient);

    QDataStream dataStream(data);

    // All messages start with the operation code
    int code;
    dataStream >> code;
    if (code == IpcHandler::SetResult) {
        QVariantMap result;
        dataStream >> result;
        Q_EMIT q->result(result);
    } else if (code == IpcHandler::SetCanceled) {
        Q_EMIT q->canceled();
    } else {
        qWarning() << "Unsupported opcode" << code;
    }
}

RemoteRequestClient::RemoteRequestClient(QObject *parent):
    QObject(parent),
    d_ptr(new RemoteRequestClientPrivate(this))
{
}

RemoteRequestClient::~RemoteRequestClient()
{
}

void RemoteRequestClient::setChannels(QIODevice *readChannel, QIODevice *writeChannel)
{
    Q_D(RemoteRequestClient);
    d->m_handler.setChannels(readChannel, writeChannel);
}

void RemoteRequestClient::start(const QVariantMap &parameters)
{
    Q_D(RemoteRequestClient);

    QByteArray data;
    QDataStream dataStream(&data, QIODevice::WriteOnly);
    dataStream << int(IpcHandler::Start);
    dataStream << parameters;
    d->m_handler.write(data);
}

void RemoteRequestClient::cancel()
{
    Q_D(RemoteRequestClient);

    QByteArray data;
    QDataStream dataStream(&data, QIODevice::WriteOnly);
    dataStream << int(IpcHandler::Cancel);
    d->m_handler.write(data);
}

namespace SignOnUi {
class RemoteRequestServerPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(RemoteRequestServer)

public:
    RemoteRequestServerPrivate(RemoteRequestServer *server);
    ~RemoteRequestServerPrivate() {};

private Q_SLOTS:
    void onDataReady(const QByteArray &data);

private:
    IpcHandler m_handler;
    mutable RemoteRequestServer *q_ptr;
};
} // namespace

RemoteRequestServerPrivate::RemoteRequestServerPrivate(RemoteRequestServer *server):
    QObject(server),
    q_ptr(server)
{
    QObject::connect(&m_handler, SIGNAL(dataReady(const QByteArray&)),
                     this, SLOT(onDataReady(const QByteArray&)));
}

void RemoteRequestServerPrivate::onDataReady(const QByteArray &data)
{
    Q_Q(RemoteRequestServer);

    QDataStream dataStream(data);

    // All messages start with the operation code
    int code;
    dataStream >> code;
    if (code == IpcHandler::Start) {
        QVariantMap parameters;
        dataStream >> parameters;
        Q_EMIT q->started(parameters);
    } else if (code == IpcHandler::Cancel) {
        Q_EMIT q->canceled();
    } else {
        qWarning() << "Unsupported opcode" << code;
    }
}

RemoteRequestServer::RemoteRequestServer(QObject *parent):
    QObject(parent),
    d_ptr(new RemoteRequestServerPrivate(this))
{
}

RemoteRequestServer::~RemoteRequestServer()
{
}

void RemoteRequestServer::setChannels(QIODevice *readChannel, QIODevice *writeChannel)
{
    Q_D(RemoteRequestServer);
    d->m_handler.setChannels(readChannel, writeChannel);
}

void RemoteRequestServer::setResult(const QVariantMap &result)
{
    Q_D(RemoteRequestServer);

    QByteArray data;
    QDataStream dataStream(&data, QIODevice::WriteOnly);
    dataStream << int(IpcHandler::SetResult);
    dataStream << result;
    d->m_handler.write(data);
}

void RemoteRequestServer::setCanceled()
{
    Q_D(RemoteRequestServer);

    QByteArray data;
    QDataStream dataStream(&data, QIODevice::WriteOnly);
    dataStream << int(IpcHandler::SetCanceled);
    d->m_handler.write(data);
}

#include "remote-request-interface.moc"
