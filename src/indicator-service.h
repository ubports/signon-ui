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

#ifndef SIGNON_UI_INDICATOR_SERVICE_H
#define SIGNON_UI_INDICATOR_SERVICE_H

#include <QObject>
#include <QSet>
#include <QVariantMap>

namespace SignOnUi {

#define WEBCREDENTIALS_OBJECT_PATH "/com/canonical/indicators/webcredentials"
#define WEBCREDENTIALS_INTERFACE "com.canonical.indicators.webcredentials"
#define WEBCREDENTIALS_BUS_NAME WEBCREDENTIALS_INTERFACE

class IndicatorServicePrivate;

class IndicatorService: public QObject
{
    Q_OBJECT

public:

    explicit IndicatorService(QObject *parent = 0);
    ~IndicatorService();

    static IndicatorService *instance();

    QObject *serviceObject() const;

    void reportFailure(uint accountId, const QVariantMap &notification);

private:
    IndicatorServicePrivate *d_ptr;
    Q_DECLARE_PRIVATE(IndicatorService)
};

} // namespace

Q_DECLARE_METATYPE(QSet<uint>)

#endif // SIGNON_UI_INDICATOR_SERVICE_H
