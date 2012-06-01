/*
 * Copyright (c) 2009 Nokia Corporation
 */
 
#include "qanimationlabel.h"
 
QAnimationLabel::QAnimationLabel(const QString& animationPath, QWidget* parent):
    QWidget(parent)
{
    m_animation = new QMovie(animationPath, QByteArray(), this);
    // We need a container for the QMovie.
    QLabel *container = new QLabel(this);
    container->setMovie(m_animation);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->addWidget(container);
    setLayout(layout);
}
 
QAnimationLabel::~QAnimationLabel()
{
}
 
void QAnimationLabel::start()
{
    // Let's check if the movie can be started.
    if (!m_animation.isNull() &&
        (m_animation->state() == QMovie::NotRunning ||
         m_animation->state() == QMovie::Paused)) {
        // It can so start the animation.
        m_animation->start();
    }
}
 
void QAnimationLabel::stop()
{
    // Check if the animation can be stopped.
    if (!m_animation.isNull()) {
        if (m_animation->state() == QMovie::Running) {
            // It can so stop the animation.
            m_animation->stop();
        }
    }
}
