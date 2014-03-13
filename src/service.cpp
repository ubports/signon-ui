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

#include "service.h"

#include "cookie-jar-manager.h"
#include "debug.h"
#include "request.h"

#include <QDBusArgument>
#include <QQueue>

using namespace SignOnUi;

namespace SignOnUi {

typedef QQueue<Request*> RequestQueue;

static QVariant dbusValueToVariant(const QDBusArgument &argument)
{
    QVariant ret;

    /* Note: this function should operate recursively, but it doesn't. */
    if (argument.currentType() == QDBusArgument::MapType) {
        /* Assume that all maps are a{sv} */
        ret = qdbus_cast<QVariantMap>(argument);
    } else {
        /* We don't know how to handle other types */
        ret = argument.asVariant();
    }
    return ret;
}

static QVariantMap expandDBusArguments(const QVariantMap &dbusMap)
{
    QVariantMap map;
    QMapIterator<QString, QVariant> it(dbusMap);
    while (it.hasNext()) {
        it.next();
        if (qstrcmp(it.value().typeName(), "QDBusArgument") == 0) {
            QDBusArgument dbusValue = it.value().value<QDBusArgument>();
            map.insert(it.key(), dbusValueToVariant(dbusValue));
        } else {
            map.insert(it.key(), it.value());
        }
    }
    return map;
}

class ServicePrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Service)

public:
    ServicePrivate(Service *service);
    ~ServicePrivate();

    RequestQueue &queueForWindowId(WId windowId);
    void enqueue(Request *request);
    void runQueue(RequestQueue &queue);
    void cancelUiRequest(const QString &requestId);
    void removeIdentityData(quint32 id);

private Q_SLOTS:
    void onRequestCompleted();

private:
    mutable Service *q_ptr;
    /* each window Id has a different queue */
    QMap<WId,RequestQueue> m_requests;
};

} // namespace

ServicePrivate::ServicePrivate(Service *service):
    QObject(service),
    q_ptr(service)
{
}

ServicePrivate::~ServicePrivate()
{
}

RequestQueue &ServicePrivate::queueForWindowId(WId windowId)
{
    if (!m_requests.contains(windowId)) {
        RequestQueue queue;
        m_requests.insert(windowId, queue);
    }
    return m_requests[windowId];
}

void ServicePrivate::enqueue(Request *request)
{
    Q_Q(Service);
    bool wasIdle = q->isIdle();

    WId windowId = request->windowId();

    RequestQueue &queue = queueForWindowId(windowId);
    queue.enqueue(request);

    if (wasIdle) {
        Q_EMIT q->isIdleChanged();
    }

    runQueue(queue);
}

void ServicePrivate::runQueue(RequestQueue &queue)
{
    Request *request = queue.head();
    TRACE() << "Head:" << request;

    if (request->isInProgress()) {
        TRACE() << "Already in progress";
        return; // Nothing to do
    }

    QObject::connect(request, SIGNAL(completed()),
                     this, SLOT(onRequestCompleted()));
    request->start();
}

void ServicePrivate::onRequestCompleted()
{
    Q_Q(Service);

    Request *request = qobject_cast<Request*>(sender());
    WId windowId = request->windowId();

    RequestQueue &queue = queueForWindowId(windowId);
    if (request != queue.head()) {
        BLAME() << "Completed request is not first in queue!";
        return;
    }

    queue.dequeue();
    request->deleteLater();

    if (queue.isEmpty()) {
        m_requests.remove(windowId);
    } else {
        /* start the next request */
        runQueue(queue);
    }

    if (q->isIdle()) {
        Q_EMIT q->isIdleChanged();
    }
}

void ServicePrivate::cancelUiRequest(const QString &requestId)
{
    Request *request = 0;

    /* Find the request; we don't know in which queue it is, so we must search
     * all queues. */
    foreach (RequestQueue queue, m_requests) {
        foreach (Request *r, queue) {
            if (r->id() == requestId) {
                request = r;
                break;
            }
        }
    }

    TRACE() << "Cancelling request" << request;
    if (request != 0) {
        request->cancel();
    }
}

void ServicePrivate::removeIdentityData(quint32 id)
{
    /* Remove any data associated with the given identity. */

    /* The BrowserRequest class uses CookieJarManager to store the cookies */
    CookieJarManager::instance()->removeForIdentity(id);
}

Service::Service(QObject *parent):
    QObject(parent),
    d_ptr(new ServicePrivate(this))
{
}

Service::~Service()
{
}

bool Service::isIdle() const
{
    Q_D(const Service);
    return d->m_requests.isEmpty();
}

QVariantMap Service::queryDialog(const QVariantMap &parameters)
{
    Q_D(Service);

    QVariantMap cleanParameters = expandDBusArguments(parameters);
    TRACE() << "Got request:" << cleanParameters;
    Request *request = Request::newRequest(connection(),
                                           message(),
                                           cleanParameters,
                                           this);
    d->enqueue(request);

    /* The following line tells QtDBus not to generate a reply now */
    setDelayedReply(true);
    return QVariantMap();
}

QVariantMap Service::refreshDialog(const QVariantMap &newParameters)
{
    QVariantMap cleanParameters = expandDBusArguments(newParameters);
    QString requestId = Request::id(cleanParameters);
    // TODO find the request and update it

    /* The following line tells QtDBus not to generate a reply now */
    setDelayedReply(true);
    return QVariantMap();
}

void Service::cancelUiRequest(const QString &requestId)
{
    Q_D(Service);
    d->cancelUiRequest(requestId);
}

void Service::removeIdentityData(quint32 id)
{
    Q_D(Service);
    d->removeIdentityData(id);
}

#include "service.moc"
