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

#ifndef SIGNON_UI_DIALOG_H
#define SIGNON_UI_DIALOG_H

#include <QDialog>
#include <QObject>

namespace SignOnUi {

class Dialog: public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0, Qt::WindowFlags f = 0);
    ~Dialog();

protected:
    // reimplemented virtual methods
    void closeEvent(QCloseEvent *e);
};

} // namespace

#endif // SIGNON_UI_DIALOG_H

