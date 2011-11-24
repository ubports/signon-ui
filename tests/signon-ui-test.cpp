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

#include "signon-ui-test.h"

#include <QDebug>
#include <QTest>

#include <SignOn/uisessiondata_priv.h>

static const char serviceName[] = "com.nokia.singlesignonui";
static const char objectPath[] = "/SignonUi";
static const char interfaceName[] = "com.nokia.singlesignonui";

PendingCall::PendingCall(const QDBusPendingCall &call, QObject *parent):
    QDBusPendingCallWatcher(call, parent)
{
}

PendingCall::~PendingCall()
{
}

bool PendingCall::isError() const
{
    if (!m_reply.isValid()) {
        m_reply = *this;
    }

    return m_reply.isError();
}

QDBusError PendingCall::error() const
{
    return m_reply.error();
}

QVariantMap PendingCall::variantMap() const
{
    if (!m_reply.isValid()) {
        m_reply = *this;
    }

    return m_reply.argumentAt<0>();
}

PendingCall *SignOnUiTest::queryDialog(const QVariantMap &parameters)
{
    QDBusPendingCall call = m_interface->asyncCall(QLatin1String("queryDialog"),
                                                   parameters);
    return new PendingCall(call, this);
}

void SignOnUiTest::initTestCase()
{
    m_interface = new QDBusInterface(QLatin1String(serviceName),
                                     QLatin1String(objectPath),
                                     QLatin1String(interfaceName));
}

void SignOnUiTest::cleanupTestCase()
{
    delete m_interface;
}

void SignOnUiTest::username()
{
    QVariantMap params;

    params.insert(SSOUI_KEY_QUERYUSERNAME, true);
    params.insert(SSOUI_KEY_CAPTION, QString::fromLatin1("Insert username"));
    PendingCall *call = queryDialog(params);
    call->waitForFinished();

    QVERIFY(!call->isError());
    qDebug() << call->variantMap();
}

QTEST_MAIN(SignOnUiTest)

