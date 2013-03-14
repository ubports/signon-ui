/*
 * This file is part of signon-ui
 *
 * Copyright (C) 2013 Canonical Ltd.
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
#include "inactivity-timer.h"

#include <QDebug>
#include <QObject>
#include <QSignalSpy>
#include <QTest>

using namespace SignOnUi;

class InactivityTest: public QObject
{
    Q_OBJECT

public:
    InactivityTest() {};

private Q_SLOTS:
    void testAlwaysIdle();
    void testIdleThenBusy();
    void testStartBusy();
};

class TestObject: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isIdle READ isIdle NOTIFY isIdleChanged)

public:
    TestObject(): QObject(0), m_isIdle(false) {}
    ~TestObject() {}

    bool isIdle() const { return m_isIdle; }
    void setIdle(bool idle) {
        if (idle == m_isIdle) return;
        m_isIdle = idle;
        Q_EMIT isIdleChanged();
    }

Q_SIGNALS:
    void isIdleChanged();

private:
    bool m_isIdle;
};

void InactivityTest::testAlwaysIdle()
{
    InactivityTimer inactivityTimer(100);
    QSignalSpy timeout(&inactivityTimer, SIGNAL(timeout()));

    TestObject object1;
    object1.setIdle(true);
    inactivityTimer.watchObject(&object1);

    TestObject object2;
    object2.setIdle(true);
    inactivityTimer.watchObject(&object2);

    QCOMPARE(timeout.count(), 0);
    QTest::qWait(150);
    QCOMPARE(timeout.count(), 1);
}

void InactivityTest::testIdleThenBusy()
{
    InactivityTimer inactivityTimer(100);
    QSignalSpy timeout(&inactivityTimer, SIGNAL(timeout()));

    TestObject object1;
    object1.setIdle(true);
    inactivityTimer.watchObject(&object1);

    TestObject object2;
    object2.setIdle(true);
    inactivityTimer.watchObject(&object2);

    QCOMPARE(timeout.count(), 0);

    QTest::qWait(30);
    QCOMPARE(timeout.count(), 0);
    object2.setIdle(false);

    QTest::qWait(100);
    QCOMPARE(timeout.count(), 0);
    object2.setIdle(true);
    QTest::qWait(30);
    QCOMPARE(timeout.count(), 0);

    QTest::qWait(100);
    QCOMPARE(timeout.count(), 1);
}

void InactivityTest::testStartBusy()
{
    InactivityTimer inactivityTimer(100);
    QSignalSpy timeout(&inactivityTimer, SIGNAL(timeout()));

    TestObject object1;
    inactivityTimer.watchObject(&object1);

    TestObject object2;
    inactivityTimer.watchObject(&object2);

    QCOMPARE(timeout.count(), 0);

    QTest::qWait(130);
    QCOMPARE(timeout.count(), 0);
    object2.setIdle(true);

    QTest::qWait(130);
    QCOMPARE(timeout.count(), 0);
    object1.setIdle(true);

    QTest::qWait(30);
    QCOMPARE(timeout.count(), 0);

    QTest::qWait(100);
    QCOMPARE(timeout.count(), 1);
}

QTEST_MAIN(InactivityTest);
#include "tst_inactivity_timer.moc"
