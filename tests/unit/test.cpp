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
#include "fake-libnotify.h"
#include "indicator-service.h"
#include "test.h"
#include "reauthenticator.h"
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
    account->setValue("CredentialsId", 0xdeadbeef);
    account->setEnabled(true);
    account->syncAndBlock();
    /* now create the "good" account */
    account = manager->createAccount(0);
    account->setValue("CredentialsId", signonId);
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

static void prepareAuthData(AuthData &authData, int identity)
{
    QVariantMap sessionData;
    sessionData["Int"] = identity;

    authData.identity = identity;
    authData.sessionData = sessionData;
    authData.method = QString::fromLatin1("method%1").arg(identity);
    authData.mechanism = QString::fromLatin1("mechanism%1").arg(identity);
}

void SignOnUiTest::testReauthenticator()
{
    QList<AuthData> failuresData;

    for (int i = 1; i < 8; i++) {
        AuthData authData;
        prepareAuthData(authData, i);

        failuresData.append(authData);
    }

    QVariantMap extraParameters;
    extraParameters["Greeting"] = QString::fromLatin1("Hello!");

    Reauthenticator *reauthenticator =
        new Reauthenticator(failuresData, extraParameters);
    QSignalSpy finished(reauthenticator, SIGNAL(finished(bool)));
    reauthenticator->start();

    QTest::qWait(200);

    QCOMPARE(finished.count(), 1);
    QList<QVariant> arguments = finished.takeFirst(); // first signal

    /* The reauthentication failed because of some invalid identities
     * created by the fake libsignon.*/
    QCOMPARE(arguments.at(0).toBool(), false);

    delete reauthenticator;

    /* Now create the AuthData for the identities which are known to work */
    failuresData.clear();
    AuthData authData;
    prepareAuthData(authData, 1);
    failuresData.append(authData);
    prepareAuthData(authData, 2);
    failuresData.append(authData);
    prepareAuthData(authData, 4);
    failuresData.append(authData);

    reauthenticator =
        new Reauthenticator(failuresData, extraParameters);
    QSignalSpy finished2(reauthenticator, SIGNAL(finished(bool)));
    reauthenticator->start();

    QTest::qWait(200);

    QCOMPARE(finished2.count(), 1);
    arguments = finished2.takeFirst(); // first signal

    QCOMPARE(arguments.at(0).toBool(), true);

    delete reauthenticator;
}

void SignOnUiTest::testIndicatorService()
{
    const uint firstFailure = 413;

    QVERIFY(IndicatorService::instance() == 0);

    IndicatorService *service = new IndicatorService();
    QVERIFY(service != 0);
    QCOMPARE(IndicatorService::instance(), service);
    QVERIFY(service->serviceObject() != 0);

    // Check initial status
    QVERIFY(service->failures().isEmpty());
    QCOMPARE(service->errorStatus(), false);

    // Report the first failure
    service->reportFailure(firstFailure, QVariantMap());
    QCOMPARE(FakeLibNotify::notificationCount(), 1);
    QCOMPARE(service->errorStatus(), true);
    QCOMPARE(service->failures().count(), 1);
    QVERIFY(service->failures().contains(firstFailure));

    // Report more failures
    QList<uint> moreFailures;
    moreFailures << 89 << 412 << 1 << 4 << 144;
    foreach (uint id, moreFailures) {
        service->reportFailure(id, QVariantMap());
    }
    QCOMPARE(FakeLibNotify::notificationCount(), 1);
    QCOMPARE(service->errorStatus(), true);
    QCOMPARE(service->failures().count(), 1 + moreFailures.count());

    // Remove some failures
    QSet<uint> removedFailures;
    removedFailures << 4 << firstFailure << 89;
    service->removeFailures(removedFailures);
    QSet<uint> remainingFailures;
    remainingFailures << 412 << 1 << 144;
    QCOMPARE(FakeLibNotify::notificationCount(), 1);
    QCOMPARE(service->errorStatus(), true);
    QCOMPARE(service->failures(), remainingFailures);

    // Clear the error status
    service->clearErrorStatus();
    QCOMPARE(service->errorStatus(), false);
    QCOMPARE(service->failures(), remainingFailures);

    // Send one more failure
    service->reportFailure(3, QVariantMap());
    QCOMPARE(FakeLibNotify::notificationCount(), 2);
    QCOMPARE(service->errorStatus(), true);

    delete service;
    QVERIFY(IndicatorService::instance() == 0);
}

QTEST_MAIN(SignOnUiTest);
