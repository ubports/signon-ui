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

Q_LOGGING_CATEGORY(DBG_SIGNON_UI, "signon", QtWarningMsg)

int appLoggingLevel = 1; // criticals

void setLoggingLevel(int level)
{
    if (level >= 1) {
        QLoggingCategory::setFilterRules("signon.warning=true");
        if (level >= 2) {
            QLoggingCategory::setFilterRules("signon.debug=true");
        }
    }
    appLoggingLevel = level;
}

