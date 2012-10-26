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

#include <QDebug>
#include <QTimer>
#include <SignOn/AuthSession>
#include <SignOn/Identity>

using namespace SignOn;

namespace SignOn {

class IdentityImpl {
    AuthSessionP createSession(const QString &method);
    friend class Identity;
    Identity *q;
    quint32 id;
};

class AuthSessionImpl: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emitResponse();

private:
    friend class AuthSession;
    AuthSession *q;
    quint32 id;
    QString method;
    SessionData sessionData;
};

} // namespace

AuthSessionP IdentityImpl::createSession(const QString &method)
{
    /* pretend to fail creating a few authsessions */
    if (id % 5 == 0) return 0;

    return new AuthSession(id, method, q);
}

Identity::Identity(quint32 id, QObject *parent):
    QObject(parent),
    impl(new IdentityImpl)
{
    impl->id = id;
    impl->q = this;
}

Identity::~Identity()
{
    delete impl;
}

Identity *Identity::existingIdentity(quint32 id, QObject *parent)
{
    /* Pretend not to find a few identities */
    if (id % 3 == 0) return 0;

    return new Identity(id, parent);
}

AuthSessionP Identity::createSession(const QString &method)
{
    return impl->createSession(method);
}

void AuthSessionImpl::emitResponse()
{
    Q_EMIT q->response(sessionData);
}

AuthSession::AuthSession(quint32 id, const QString &method,
                         QObject *parent):
    QObject(parent),
    impl(new AuthSessionImpl)
{
    qDebug() << "Created fake Authsession" << id << method;
    impl->q = this;
    impl->id = id;
    impl->method = method;
}

AuthSession::~AuthSession()
{
    delete impl;
}

void AuthSession::process(const SessionData &sessionData,
                          const QString &mechanism)
{
    QVariantMap map = sessionData.toMap();
    map["TheMechanism"] = mechanism;
    impl->sessionData = SessionData(map);
    /* delay the response with some pseudo-random wait */
    int msec = 100 + (impl->id % 4) * 5;
    QTimer::singleShot(msec, impl, SLOT(emitResponse()));
}

#include "fake-libsignon.moc"
