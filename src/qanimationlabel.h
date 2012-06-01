/*
 * Copyright (c) 2009 Nokia Corporation
 */
 
#ifndef QANIMATIONLABEL_H_
#define QANIMATIONLABEL_H_
 
#include <QVBoxLayout>
 
#include <QLabel>
#include <QMovie>
 
/**
 * QAnimationLabel
 *
 * Uses animation from the path
 * to display it in a QLabel.
 */
class QAnimationLabel : public QWidget {
	Q_OBJECT
public:
	QAnimationLabel(const QString& animationPath, QWidget* parent);
	virtual ~QAnimationLabel();
 
public Q_SLOTS:
	void start();
	void stop();
 
private:
	QPointer<QMovie> m_animation;
};
 
#endif /* QANIMATIONLABEL_H_ */
