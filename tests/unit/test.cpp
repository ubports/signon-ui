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
#include "test.h"
#include "request.h"
#include "fake-webcredentials-interface.h"

#include <Accounts/Manager>
#include <QDebug>
#include <QDir>
#include <QSignalSpy>
#include <SignOn/uisessiondata.h>
#include <SignOn/uisessiondata_priv.h>

using namespace SignOnUi;

SignOnUiTest::SignOnUiTest():
    m_dbusConnection("test")
{
    setLoggingLevel(2);
}

void SignOnUiTest::initTestCase()
{
    /* Use a temporary DB for accounts */
    setenv("ACCOUNTS", "/tmp/", false);
    QDir dbroot("/tmp");
    dbroot.remove("accounts.db");

}

void SignOnUiTest::testRequestObjects()
{
    const WId windowId = 23141;
    const QString requestId = QLatin1String("request id 12342");
    const QString title = QLatin1String("Title for the signon-ui window");

    QVariantMap clientData;
    clientData[SSOUI_KEY_WINDOWID] = (uint)windowId;
    clientData[SSOUI_KEY_EMBEDDED] = true;

    QVariantMap parameters;
    parameters[SSOUI_KEY_REQUESTID] = requestId;
    parameters[SSOUI_KEY_TITLE] = title;
    parameters[SSOUI_KEY_QUERYPASSWORD] = true;
    parameters[SSOUI_KEY_CLIENT_DATA] = clientData;

    Request *request = Request::newRequest(m_dbusConnection,
                                           m_dbusMessage,
                                           parameters,
                                           this);
    QVERIFY(request != 0);
    QCOMPARE(request->parent(), this);
    QCOMPARE(request->id(), requestId);
    QCOMPARE(request->windowId(), windowId);
    QCOMPARE(request->embeddedUi(), true);
    QCOMPARE(request->parameters(), parameters);
    QCOMPARE(request->clientData(), clientData);
    QCOMPARE(request->isInProgress(), false);
    /* make sure that the request is of type DialogRequest */
    QCOMPARE(request->metaObject()->className(), "SignOnUi::DialogRequest");
    delete request;

    /* Now slightly modify the request parameters, so that a BrowserRequest
     * will be created instead */
    parameters[SSOUI_KEY_OPENURL] = "http://localhost:9999/page404.html";
    request = Request::newRequest(m_dbusConnection,
                                  m_dbusMessage,
                                  parameters,
                                  this);
    QVERIFY(request != 0);
    QCOMPARE(request->metaObject()->className(), "SignOnUi::BrowserRequest");
    delete request;
}

void SignOnUiTest::testRequestWithIndicator()
{
    const uint signonId = 1234;
    const QString displayName = QLatin1String("Beautiful account");

    /* create an account claiming to use our fake signon-id */
    Accounts::Manager *manager = new Accounts::Manager(this);
    /* first, create a couple of dummy accounts */
    Accounts::Account *account = manager->createAccount(0);
    account->setEnabled(true);
    account->syncAndBlock();
    account = manager->createAccount(0);
    account->setValue("signon-id", 0xdeadbeef);
    account->setEnabled(true);
    account->syncAndBlock();
    /* now create the "good" account */
    account = manager->createAccount(0);
    account->setValue("signon-id", signonId);
    account->setEnabled(true);
    account->setDisplayName(displayName);
    account->syncAndBlock();

    /* now create a request */
    QVariantMap parameters;
    parameters[SSOUI_KEY_QUERYPASSWORD] = true;
    parameters[SSOUI_KEY_IDENTITY] = signonId;

    Request *request = Request::newRequest(m_dbusConnection,
                                           m_dbusMessage,
                                           parameters,
                                           this);
    QVERIFY(request != 0);
    QCOMPARE(request->isInProgress(), false);

    QSignalSpy requestCompleted(request, SIGNAL(completed()));

    request->start();
    QCOMPARE(requestCompleted.count(), 1);

    ComCanonicalIndicatorsWebcredentialsInterface *indicator =
        ComCanonicalIndicatorsWebcredentialsInterface::instance();
    QVERIFY(indicator != 0);
    QCOMPARE(indicator->m_reportFailureCalled, true);
    QCOMPARE(indicator->m_account_id, account->id());
    QCOMPARE(indicator->m_notification["DisplayName"].toString(), displayName);

    delete request;
    delete manager;
}

QTEST_MAIN(SignOnUiTest);
