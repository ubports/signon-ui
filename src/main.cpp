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

#include "debug.h"
#include "i18n.h"
#include "inactivity-timer.h"
#include "indicator-service.h"
#include "my-network-proxy-factory.h"
#include "service.h"

#include <QApplication>
#include <QDBusConnection>
#include <QProcessEnvironment>
#include <QSettings>

using namespace SignOnUi;

/* This is where signond expects to find us */
static const char serviceName[] = "com.nokia.singlesignonui";
static const char objectPath[] = "/SignonUi";

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName("signon-ui");
    app.setQuitOnLastWindowClosed(false);

    /* read environment variables */
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    if (environment.contains(QLatin1String("SSOUI_LOGGING_LEVEL"))) {
        bool isOk;
        int value = environment.value(
            QLatin1String("SSOUI_LOGGING_LEVEL")).toInt(&isOk);
        if (isOk)
            setLoggingLevel(value);
    }

    /* default daemonTimeout to 30 */
    int daemonTimeout = 30;

    /* override daemonTimeout if SSOUI_DAEMON_TIMEOUT is set */
    if (environment.contains(QLatin1String("SSOUI_DAEMON_TIMEOUT"))) {
        bool isOk;
        int value = environment.value(
            QLatin1String("SSOUI_DAEMON_TIMEOUT")).toInt(&isOk);
        if (isOk)
            daemonTimeout = value;
    }

    QSettings::setPath(QSettings::NativeFormat, QSettings::SystemScope,
                       QLatin1String("/etc"));

    initTr(I18N_DOMAIN, NULL);

    /* Use a libproxy-based proxy factory; this code will no longer be
     * needed when https://bugreports.qt-project.org/browse/QTBUG-26295
     * is fixed. */
    MyNetworkProxyFactory *proxyFactory = new MyNetworkProxyFactory();
    QNetworkProxyFactory::setApplicationProxyFactory(proxyFactory);

    Service *service = new Service();
    QDBusConnection connection = QDBusConnection::sessionBus();
    connection.registerService(QLatin1String(serviceName));
    connection.registerObject(QLatin1String(objectPath),
                              service,
                              QDBusConnection::ExportAllContents);

    IndicatorService *indicatorService = new IndicatorService();
    connection.registerService(QLatin1String(WEBCREDENTIALS_BUS_NAME));
    connection.registerObject(QLatin1String(WEBCREDENTIALS_OBJECT_PATH),
                              indicatorService->serviceObject());

    InactivityTimer *inactivityTimer = 0;
    if (daemonTimeout > 0) {
        inactivityTimer = new InactivityTimer(daemonTimeout * 1000);
        inactivityTimer->watchObject(service);
        inactivityTimer->watchObject(indicatorService);
        QObject::connect(inactivityTimer, SIGNAL(timeout()),
                         &app, SLOT(quit()));
    }

    int ret = app.exec();

    connection.unregisterService(QLatin1String(WEBCREDENTIALS_BUS_NAME));
    connection.unregisterObject(QLatin1String(WEBCREDENTIALS_OBJECT_PATH));
    delete indicatorService;

    connection.unregisterService(QLatin1String(serviceName));
    connection.unregisterObject(QLatin1String(objectPath));
    delete service;

    delete inactivityTimer;

    return ret;
}

