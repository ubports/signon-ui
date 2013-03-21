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

#include "fake-libnotify.h"

#undef signals
#include <libnotify/notification.h>
#include <libnotify/notify.h>

static int m_notificationCount = 0;
static NotifyNotification *fake_notification = 0;

int FakeLibNotify::notificationCount()
{
    return m_notificationCount;
}

void FakeLibNotify::clearNotificationCount()
{
    m_notificationCount = 0;
}

gboolean notify_init(const char *app_name)
{
    g_return_val_if_fail(app_name != NULL, FALSE);
    return TRUE;
}

NotifyNotification *notify_notification_new(const char *,
                                            const char *,
                                            const char *)
{
    fake_notification = (NotifyNotification *)g_object_new (G_TYPE_OBJECT, NULL);
    return fake_notification;
}

gboolean notify_notification_show(NotifyNotification *notification,
                                  GError **error)
{
    g_return_val_if_fail(notification == fake_notification, FALSE);
    *error = NULL;
    m_notificationCount++;
    return TRUE;
}

