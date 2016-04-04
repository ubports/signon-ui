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

#define HAS_FOREIGN_QWINDOW (QT_VERSION >= QT_VERSION_CHECK(5, 1, 0) || \
                             defined(FORCE_FOREIGN_QWINDOW))

#include "qquick-dialog.h"

#include "debug.h"

#include <QEvent>

using namespace SignOnUi::QQuick;

Dialog::Dialog(QWindow *parent):
    QQuickView(parent)
{
    setResizeMode(QQuickView::SizeRootObjectToView);
    //setWindowState(Qt::WindowFullScreen);
}

Dialog::~Dialog()
{
}

void Dialog::show(WId parent, ShowMode mode)
{
    create();
#if HAS_FOREIGN_QWINDOW
    if (mode != TopLevel) {
        QWindow *parentWindow = QWindow::fromWinId(parent);
        if (mode == Transient) {
            setTransientParent(parentWindow);
        } else if (mode == Embedded) {
            setParent(parentWindow);
        }
    }
#endif
    QQuickView::show();
}

void Dialog::accept()
{
    done(Dialog::Accepted);
}

void Dialog::reject()
{
    done(Dialog::Rejected);
}

void Dialog::done(int result)
{
    setVisible(false);
    Q_EMIT finished(result);
}

bool Dialog::event(QEvent *e)
{
    if (e->type() == QEvent::Close) {
        reject();
    }
    return QQuickView::event(e);
}
