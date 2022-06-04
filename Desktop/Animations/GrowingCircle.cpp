#include "GrowingCircle.h"

#include <qpropertyanimation.h>
#include <qpainter.h>
#include <iostream>
#include "../UtilFuncs.h"

GrowingCircleAnim::GrowingCircleAnim(QWidget* w) : drawnWdgt(w)
{
	this->setParent(w);
	this->anim = new QPropertyAnimation(this, "val");
	startBrushCol = QColor(255,0,255,60);
	startPenCol = QColor(255,0,0,255);
	endBrushCol = QColor(255, 0, 255, 0);
	endPenCol = QColor(0, 0, 255, 0);
	anim->setDuration(100);
	anim->setStartValue(0.0);
	anim->setEndValue(1.0);
	anim->setDirection(QAbstractAnimation::Forward);
}
void GrowingCircleAnim::SetDuration(int ms)
{
	anim->setDuration(ms);
}

void GrowingCircleAnim::Draw(QPainter* p)
{
	if (interpolationValue == 0.0) return;
	
	QRect rec = drawnWdgt->rect();
	QRect geom = drawnWdgt->geometry();
	int max = sqrt(rec.width() * rec.width() + rec.height() * rec.height()) * 2;

	int radius = interpolationValue * max;


	QColor brCol = AlphaBlend(endBrushCol, startBrushCol, interpolationValue);
	QColor penCol = AlphaBlend(endPenCol, startPenCol, interpolationValue);
	

	p->setBrush(brCol);
	p->setPen(penCol);
	QRect ellipse(startMP.x() - radius / 2, startMP.y() - radius / 2, radius, radius);
	p->drawEllipse(ellipse);
}

void GrowingCircleAnim::Start()
{
	anim->stop();
	interpolationValue = 0.0;
	startMP = drawnWdgt->mapFromGlobal(QCursor::pos());
	anim->start();
}
void GrowingCircleAnim::Reset()
{
	interpolationValue = 0.0;
}

void GrowingCircleAnim::SetInterpolationValue(qreal val)
{
	interpolationValue = val;
	drawnWdgt->update();
}