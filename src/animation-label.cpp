/*
 * This file is part of signon-ui
 *
 * Copyright (c) 2009 Nokia Corporation
 * Copyright (C) 2012 Canonical Ltd.
 *
 * Contact: David King <david.king@canonical.com>
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

#include "animation-label.h"
 
AnimationLabel::AnimationLabel(const QString &animationPath, QWidget *parent):
    QWidget(parent)
{
    m_animation = new QMovie(animationPath, QByteArray(), this);
    // We need a container for the QMovie.
    QLabel *container = new QLabel(this);
    container->setMovie(m_animation);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->addWidget(container);
    setLayout(layout);
}
 
AnimationLabel::~AnimationLabel()
{
}
 
void AnimationLabel::start()
{
    // Let's check if the movie can be started.
    if (m_animation->state() == QMovie::NotRunning ||
        m_animation->state() == QMovie::Paused) {
        // It can so start the animation.
        m_animation->start();
    }
}
 
void AnimationLabel::stop()
{
    // Check if the animation can be stopped.
    if (m_animation->state() == QMovie::Running) {
        // It can so stop the animation.
        m_animation->stop();
    }
}
