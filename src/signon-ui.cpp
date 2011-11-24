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

#include "signon-ui.h"

#include "debug.h"
#include "request.h"

#include <QQueue>

typedef QQueue<Request*> RequestQueue;

class SignOnUiPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(SignOnUi)

public:
    SignOnUiPrivate(SignOnUi *signOnUi);
    ~SignOnUiPrivate();

    RequestQueue &queueForWindowId(WId windowId);
    void enqueue(Request *request);
    void runQueue(RequestQueue &queue);

private Q_SLOTS:
    void onRequestCompleted();

private:
    mutable SignOnUi *q_ptr;
    /* each window Id has a different queue */
    QMap<WId,RequestQueue> m_requests;
};

SignOnUiPrivate::SignOnUiPrivate(SignOnUi *signOnUi):
    QObject(signOnUi),
    q_ptr(signOnUi)
{
}

SignOnUiPrivate::~SignOnUiPrivate()
{
}

RequestQueue &SignOnUiPrivate::queueForWindowId(WId windowId)
{
    if (!m_requests.contains(windowId)) {
        RequestQueue queue;
        m_requests.insert(windowId, queue);
    }
    return m_requests[windowId];
}

void SignOnUiPrivate::enqueue(Request *request)
{
    WId windowId = request->windowId();

    RequestQueue &queue = queueForWindowId(windowId);
    queue.enqueue(request);

    runQueue(queue);
}

void SignOnUiPrivate::runQueue(RequestQueue &queue)
{
    Request *request = queue.head();

    if (request->isInProgress()) {
        return; // Nothing to do
    }

    QObject::connect(request, SIGNAL(completed()),
                     this, SLOT(onRequestCompleted()));
    request->start();
}

void SignOnUiPrivate::onRequestCompleted()
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

SignOnUi::SignOnUi(QObject *parent):
    QObject(parent),
    d_ptr(new SignOnUiPrivate(this))
{
}

SignOnUi::~SignOnUi()
{
}

QVariantMap SignOnUi::queryDialog(const QVariantMap &parameters)
{
    Q_D(SignOnUi);

    TRACE() << "Got request:" << parameters;
    Request *request = new Request(connection(),
                                   message(),
                                   parameters,
                                   this);
    d->enqueue(request);

    /* The following line tells QtDBus not to generate a reply now */
    setDelayedReply(true);
    return QVariantMap();
}

QVariantMap SignOnUi::refreshDialog(const QVariantMap &newParameters)
{
    QString requestId = Request::id(newParameters);
    // TODO find the request and update it

    /* The following line tells QtDBus not to generate a reply now */
    setDelayedReply(true);
    return QVariantMap();
}

void SignOnUi::cancelUiRequest(const QString &requestId)
{
    Q_UNUSED(requestId); // TODO
}

#include "signon-ui.moc"
