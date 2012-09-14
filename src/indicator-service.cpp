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

#include "indicator-service.h"

#include "debug.h"
#include "i18n.h"
#include "webcredentials_adaptor.h"

using namespace SignOnUi;

QDBusArgument &operator<<(QDBusArgument &argument, const QSet<uint> &set)
{
    argument.beginArray(qMetaTypeId<uint>());
    foreach (uint id, set) {
        argument << id;
    }
    argument.endArray();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument,
                                QSet<uint> &set)
{
    argument.beginArray();
    while (!argument.atEnd()) {
        uint id;
        argument >> id;
        set.insert(id);
    }
    argument.endArray();
    return argument;
}

namespace SignOnUi {

static IndicatorService *m_instance = 0;

class IndicatorServicePrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(IndicatorService)

public:
    Q_PROPERTY(QSet<uint> Failures READ failures)
    Q_PROPERTY(bool ErrorStatus READ errorStatus)

    IndicatorServicePrivate(IndicatorService *service);
    ~IndicatorServicePrivate() {};

    QSet<uint> failures() const { return m_failures; }
    bool errorStatus() const { return m_errorStatus; }

public Q_SLOTS:
    void ClearErrorStatus();
    void RemoveFailures(const QSet<uint> &accountIds);
    void ReportFailure(uint accountId, const QVariantMap &notification);

private:
    void setErrorStatus();
    void notifyPropertyChanged(const char *propertyName);

private:
    mutable IndicatorService *q_ptr;
    WebcredentialsAdaptor *m_adaptor;
    QSet<uint> m_failures;
    bool m_errorStatus;
};

} // namespace

IndicatorServicePrivate::IndicatorServicePrivate(IndicatorService *service):
    QObject(service),
    q_ptr(service),
    m_adaptor(new WebcredentialsAdaptor(this)),
    m_errorStatus(false)
{
    qDBusRegisterMetaType< QSet<uint> >();
}

void IndicatorServicePrivate::ClearErrorStatus()
{
    if (m_errorStatus) {
        m_errorStatus = false;
        notifyPropertyChanged("ErrorStatus");
    }
}

void IndicatorServicePrivate::RemoveFailures(const QSet<uint> &accountIds)
{
    m_failures.subtract(accountIds);
    notifyPropertyChanged("Failures");
}

void IndicatorServicePrivate::ReportFailure(uint accountId,
                                            const QVariantMap &notification)
{
    Q_UNUSED(notification);

    m_failures.insert(accountId);
    notifyPropertyChanged("Failures");

    setErrorStatus();
}

void IndicatorServicePrivate::setErrorStatus()
{
    /* Don't show more than one notification, until the error status is
     * cleared */
    if (m_errorStatus) return;

    m_errorStatus = true;
    notifyPropertyChanged("ErrorStatus");
}

void IndicatorServicePrivate::notifyPropertyChanged(const char *propertyName)
{
    QDBusMessage signal =
        QDBusMessage::createSignal(WEBCREDENTIALS_OBJECT_PATH,
                                   "org.freedesktop.DBus.Properties",
                                   "PropertiesChanged");
    signal << WEBCREDENTIALS_INTERFACE;
    QVariantMap changedProps;
    changedProps.insert(QString::fromLatin1(propertyName),
                        property(propertyName));
    signal << changedProps;
    signal << QStringList();
    QDBusConnection::sessionBus().send(signal);
}

IndicatorService::IndicatorService(QObject *parent):
    QObject(parent),
    d_ptr(new IndicatorServicePrivate(this))
{
    if (m_instance == 0) {
        m_instance = this;
    } else {
        BLAME() << "Instantiating a second IndicatorService!";
    }
}

IndicatorService::~IndicatorService()
{
    m_instance = 0;
    delete d_ptr;
}

IndicatorService *IndicatorService::instance()
{
    return m_instance;
}

QObject *IndicatorService::serviceObject() const
{
    return d_ptr;
}

void IndicatorService::clearErrorStatus()
{
    Q_D(IndicatorService);
    d->ClearErrorStatus();
}

void IndicatorService::removeFailures(const QSet<uint> &accountIds)
{
    Q_D(IndicatorService);
    d->RemoveFailures(accountIds);
}

void IndicatorService::reportFailure(uint accountId,
                                     const QVariantMap &notification)
{
    Q_D(IndicatorService);
    d->ReportFailure(accountId, notification);
}

QSet<uint> IndicatorService::failures() const
{
    Q_D(const IndicatorService);
    return d->m_failures;
}

bool IndicatorService::errorStatus() const
{
    Q_D(const IndicatorService);
    return d->m_errorStatus;
}

#include "indicator-service.moc"
