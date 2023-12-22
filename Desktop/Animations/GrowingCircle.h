#pragma once

#include <qwidget.h>

class GrowingCircleAnim : public QObject
{
	Q_OBJECT
	Q_PROPERTY(qreal val WRITE SetInterpolationValue)
public:
	GrowingCircleAnim(QWidget* w);
	~GrowingCircleAnim();
	void SetDuration(int ms);

	void Start();
	void Reset();

	void Draw(QPainter* p);

	QColor startPenCol;
	QColor startBrushCol;
	QColor endPenCol;
	QColor endBrushCol;
private:
	void SetInterpolationValue(qreal val);
	QWidget* drawnWdgt;
	class QPropertyAnimation* anim;
	qreal interpolationValue;
	QPoint startMP;
};
