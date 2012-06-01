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

#ifndef ANIMATIONLABEL_H_
#define ANIMATIONLABEL_H_
 
#include <QVBoxLayout>
 
#include <QLabel>
#include <QMovie>
 
/**
 * AnimationLabel
 *
 * Uses animation from the path to display it in a QLabel.
 */
class AnimationLabel : public QWidget {
	Q_OBJECT
public:
	AnimationLabel(const QString &animationPath, QWidget *parent);
	virtual ~AnimationLabel();
 
public Q_SLOTS:
	void start();
	void stop();
 
private:
	QMovie *m_animation;
};
 
#endif /* ANIMATIONLABEL_H_ */
