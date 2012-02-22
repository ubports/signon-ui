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

#ifndef SIGNON_UI_WEBCREDENTIALS_INTERFACE_H
#define SIGNON_UI_WEBCREDENTIALS_INTERFACE_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

/*
 * Fake proxy class for interface com.canonical.indicators.webcredentials,
 * for unit testing.
 */
class ComCanonicalIndicatorsWebcredentialsInterface:
    public QDBusAbstractInterface
{
    Q_OBJECT

public:
    static inline const char *staticInterfaceName()
    { return "com.canonical.indicators.webcredentials"; }

public:
    ComCanonicalIndicatorsWebcredentialsInterface(const QString &service,
                                                  const QString &path,
                                                  const QDBusConnection &connection,
                                                  QObject *parent = 0);

    ~ComCanonicalIndicatorsWebcredentialsInterface();

public Q_SLOTS: // METHODS
    QDBusPendingReply<> RemoveFailures()
    {
        return m_reply;
    }

    QDBusPendingReply<> ReportFailure(uint account_id,
                                      const QVariantMap &notification)
    {
        qDebug() << "Report failure called";
        m_reportFailureCalled = true;
        m_account_id = account_id;
        m_notification = notification;
        return m_reply;
    }

private:
    void setReply(const QDBusPendingCall &reply)
    {
        m_reply = static_cast<QDBusPendingReply<> >(reply);
    }

    static ComCanonicalIndicatorsWebcredentialsInterface *instance();

    friend class SignOnUiTest;
    QDBusPendingReply<> m_reply;
    bool m_reportFailureCalled;
    uint m_account_id;
    QVariantMap m_notification;
};

namespace com {
  namespace canonical {
    namespace indicators {
      typedef ::ComCanonicalIndicatorsWebcredentialsInterface webcredentials;
    }
  }
}
#endif // SIGNON_UI_WEBCREDENTIALS_INTERFACE_H
