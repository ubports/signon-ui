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

#include "remote-request.h"

#include "debug.h"
#include "dialog.h"
#include "errors.h"
#include "i18n.h"
#include "remote-request-interface.h"

#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>

using namespace SignOnUi;

namespace SignOnUi {

static const QLatin1String remoteProcessPath(REMOTE_PROCESS_PATH);

class RemoteRequestPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(RemoteRequest)

public:
    RemoteRequestPrivate(const QString &processName, RemoteRequest *request);
    ~RemoteRequestPrivate();

    void start();

private Q_SLOTS:
    void onStarted();
    void handleInternalError(QProcess::ProcessError error);
    void readStandardError();

private:
    QString m_processName;
    QProcess m_process;
    RemoteRequestClient m_client;
    mutable RemoteRequest *q_ptr;
};

} // namespace

RemoteRequestPrivate::RemoteRequestPrivate(const QString &processName,
                                           RemoteRequest *request):
    QObject(request),
    m_processName(processName),
    q_ptr(request)
{
    m_process.setReadChannel(QProcess::StandardOutput);
    m_client.setChannels(&m_process, &m_process);

    QObject::connect(&m_process, SIGNAL(started()),
                     this, SLOT(onStarted()));
    QObject::connect(&m_process, SIGNAL(error(QProcess::ProcessError)),
                     this, SLOT(handleInternalError(QProcess::ProcessError)));
    QObject::connect(&m_process, SIGNAL(readyReadStandardError()),
                     this, SLOT(readStandardError()));

    QObject::connect(&m_client, SIGNAL(result(const QVariantMap&)),
                     request, SLOT(setResult(const QVariantMap&)));
    QObject::connect(&m_client, SIGNAL(canceled()),
                     request, SLOT(setCanceled()));
}

RemoteRequestPrivate::~RemoteRequestPrivate()
{
    if (m_process.state() == QProcess::Running) {
        m_client.cancel();
        m_process.disconnect(this);
    }
}

void RemoteRequestPrivate::start()
{
    Q_Q(RemoteRequest);

    static bool pathAdded = false;

    if (!pathAdded) {
        // For development, also add the browser-process subdirectory
        QStringList paths;
        paths << QCoreApplication::applicationDirPath() + "/browser-process";
        paths << remoteProcessPath;
        paths << qgetenv("PATH");
        qputenv("PATH", paths.join(":").toLocal8Bit());
        pathAdded = true;
    }
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString cachePath =
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir homeDir = cachePath + QString("/id-%1").arg(q->identity());
    if (!homeDir.exists()) {
        homeDir.mkpath(".");
    }
    env.insert("HOME", homeDir.absolutePath());
    m_process.setProcessEnvironment(env);
    /* FIXME the second parameter is a temporary workaround */
    m_process.start(m_processName, QStringList("--desktop-file-hint=/usr/share/applications/signon-ui-browser-process.desktop"));
}

void RemoteRequestPrivate::onStarted()
{
    Q_Q(RemoteRequest);
    TRACE();
    m_client.start(q->parameters());
}

void RemoteRequestPrivate::handleInternalError(QProcess::ProcessError error)
{
    Q_Q(RemoteRequest);
    TRACE() << error;
    q->fail(SIGNON_UI_ERROR_INTERNAL,
            "Error communicating with remote process");
}

void RemoteRequestPrivate::readStandardError()
{
    TRACE() << m_process.readAllStandardError();
}

RemoteRequest::RemoteRequest(const QString &processName,
                             const QDBusConnection &connection,
                             const QDBusMessage &message,
                             const QVariantMap &parameters,
                             QObject *parent):
    Request(connection, message, parameters, parent),
    d_ptr(new RemoteRequestPrivate(processName, this))
{
}

RemoteRequest::~RemoteRequest()
{
}

void RemoteRequest::start()
{
    Q_D(RemoteRequest);

    Request::start();
    d->start();
}

#include "remote-request.moc"
