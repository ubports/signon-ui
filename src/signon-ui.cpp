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

#include "signon-ui.h"
#include "signon_ui_adaptor.h"

class SignOnUiPrivate
{
    Q_DECLARE_PUBLIC(SignOnUi)

public:
    SignOnUiPrivate(SignOnUi *signOnUi);
    ~SignOnUiPrivate();

private:
    mutable SignOnUi *q_ptr;
};

SignOnUiPrivate::SignOnUiPrivate(SignOnUi *signOnUi):
    q_ptr(signOnUi)
{
}

SignOnUiPrivate::~SignOnUiPrivate()
{
}

SignOnUi::SignOnUi(QObject *parent):
    QObject(parent),
    d_ptr(new SignOnUiPrivate(this))
{
}

SignOnUi::~SignOnUi()
{
    delete d_ptr;
}

