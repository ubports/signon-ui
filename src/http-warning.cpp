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

#include "http-warning.h"

#include "debug.h"
#include "i18n.h"

#include <QFile>
#include <QLabel>
#include <QPixmap>
#include <QHBoxLayout>
#include <QVBoxLayout>

using namespace SignOnUi;

HttpWarning::HttpWarning(QWidget *parent):
    QWidget(parent)
{
    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(vLayout);

    QFrame *separator = new QFrame;
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Raised);
    separator->setContentsMargins(0, 0, 0, 0);
    vLayout->addWidget(separator);

    QWidget *labels = new QWidget;
    vLayout->addWidget(labels);

    QHBoxLayout *labelsLayout = new QHBoxLayout;
    labelsLayout->setContentsMargins(0, 0, 0, 0);
    labels->setLayout(labelsLayout);

    QLabel *label = new QLabel;
    label->setPixmap(QPixmap(":security-low.png"));
    labelsLayout->addWidget(label);

    label = new QLabel;
    label->setTextFormat(Qt::RichText);
    QString text = _("This site uses an insecure connection.");
#ifdef HTTP_WARNING_HELP
    text.append(QString(" <a href=\"" HTTP_WARNING_HELP "\">%1</a>").
                arg(_("What does this mean?")));
#endif
    label->setText(text);
    label->setOpenExternalLinks(true);
    labelsLayout->addWidget(label);

    labelsLayout->addStretch();
}

HttpWarning::~HttpWarning()
{
}
