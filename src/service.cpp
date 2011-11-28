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

#include "debug.h"
#include "request.h"

#include <QQueue>

using namespace SignOnUi;

namespace SignOnUi {

typedef QQueue<Request*> RequestQueue;

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
    WId windowId = request->windowId();

    RequestQueue &queue = queueForWindowId(windowId);
    queue.enqueue(request);

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
}

Service::Service(QObject *parent):
    QObject(parent),
    d_ptr(new ServicePrivate(this))
{
}

Service::~Service()
{
}

QVariantMap Service::queryDialog(const QVariantMap &parameters)
{
    Q_D(Service);

    TRACE() << "Got request:" << parameters;
    Request *request = Request::newRequest(connection(),
                                           message(),
                                           parameters,
                                           this);
    d->enqueue(request);

    /* The following line tells QtDBus not to generate a reply now */
    setDelayedReply(true);
    return QVariantMap();
}

QVariantMap Service::refreshDialog(const QVariantMap &newParameters)
{
    QString requestId = Request::id(newParameters);
    // TODO find the request and update it

    /* The following line tells QtDBus not to generate a reply now */
    setDelayedReply(true);
    return QVariantMap();
}

void Service::cancelUiRequest(const QString &requestId)
{
    Q_UNUSED(requestId); // TODO
}

#include "service.moc"
