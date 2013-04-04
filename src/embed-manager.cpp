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

#include "debug.h"
#include "embed-manager.h"

#include <QApplication>
#include <QPointer>
#include <QX11Info>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

using namespace SignOnUi;

static EmbedManager *staticInstance = 0;

namespace SignOnUi {

class EmbedManagerPrivate
{
public:
    EmbedManagerPrivate();
    ~EmbedManagerPrivate();

private:
    friend class EmbedManager;
    QMap<WId,QPointer<QX11EmbedWidget> > m_embedWidgets;
};

} // namespace

/* Workaround for https://bugreports.qt-project.org/browse/QTBUG-3617
 * Send XEMBED_REQUEST_FOCUS manually.
 */
#define XEMBED_REQUEST_FOCUS 3

// Sends an XEmbed message.
static void sendXEmbedMessage(WId window, Display *display, long message,
                  long detail = 0, long data1 = 0, long data2 = 0)
{
    XClientMessageEvent c;
    memset(&c, 0, sizeof(c));
    c.type = ClientMessage;
    c.message_type = XInternAtom(display, "_XEMBED", false);
    c.format = 32;
    c.display = display;
    c.window = window;

    c.data.l[0] = QX11Info::appTime();
    c.data.l[1] = message;
    c.data.l[2] = detail;
    c.data.l[3] = data1;
    c.data.l[4] = data2;

    XSendEvent(display, window, false, NoEventMask, (XEvent *) &c);
}

static bool x11EventFilter(void *message, long *)
{
    XEvent *event = reinterpret_cast<XEvent*>(message);
    if (event->type == ButtonPress)
    {
        QWidget *w = QWidget::find(event->xbutton.window);
        if (w && w->window()->objectName() == "request-widget") {
            QX11EmbedWidget *embed = static_cast<QX11EmbedWidget*>(w->window());
            QApplication::setActiveWindow(w->window());
            sendXEmbedMessage(embed->containerWinId(),
                              w->x11Info().display(),
                              XEMBED_REQUEST_FOCUS);
        }
    }
    return false;
}

EmbedManagerPrivate::EmbedManagerPrivate()
{
}

EmbedManagerPrivate::~EmbedManagerPrivate()
{
}

EmbedManager *EmbedManager::instance()
{
    if (staticInstance == 0) {
        staticInstance = new EmbedManager();
    }

    return staticInstance;
}

EmbedManager::EmbedManager(QObject *parent):
    QObject(parent),
    d_ptr(new EmbedManagerPrivate)
{
    QCoreApplication::instance()->setEventFilter(x11EventFilter);
}

EmbedManager::~EmbedManager()
{
    delete d_ptr;
}

QX11EmbedWidget *EmbedManager::widgetFor(WId windowId)
{
    Q_D(EmbedManager);

    QX11EmbedWidget *embed = d->m_embedWidgets.value(windowId, 0);
    if (embed == 0) {
        /* Create a new embed widget */
        embed = new QX11EmbedWidget;
        QObject::connect(embed, SIGNAL(containerClosed()),
                         embed, SLOT(deleteLater()));
        embed->embedInto(windowId);
        embed->setObjectName("request-widget");
        d->m_embedWidgets[windowId] = embed;
    }

    return embed;
}
