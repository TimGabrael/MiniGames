#include "CustomCheckBox.h"

#include <qpainter.h>
#include <qpropertyanimation.h>
#include "../UtilFuncs.h"
#include <qevent.h>

CustomCheckBox::CustomCheckBox(QWidget* parent) : QCheckBox(parent)
{
	anim = new QPropertyAnimation(this, "val");
	anim->setStartValue(0.0);
	anim->setEndValue(0.4);
	anim->setDuration(100);
	anim->setDirection(QAbstractAnimation::Direction::Forward);

	interpolationValue = 0.0;

	cols.bg = 0x202020;
	cols.bgHov = 0xFC6603;
	cols.bgAct = 0x20B020;
	cols.border = 0x606060;
	cols.borderHover = 0x6060A0;
	cols.borderActive = 0x8080F0;
	cols.borderFocus = 0x8090FF;
	topPadding = 0;
}

void CustomCheckBox::SetBackgroundColor(const QColor& c)
{
	cols.bg = c; update();
}
void CustomCheckBox::SetBackgroundHoveredColor(const QColor& c)
{
	cols.bgHov = c; update();
}
void CustomCheckBox::SetBackgroundActiveColor(const QColor& c)
{
	cols.bgAct = c; update();
}
void CustomCheckBox::SetBorderColor(const QColor& c)
{
	cols.border = c; update();
}
void CustomCheckBox::SetBorderHoverColor(const QColor& c)
{
	cols.borderHover = c; update();
}
void CustomCheckBox::SetBorderFocusColor(const QColor& c)
{
	cols.borderFocus = c; update();
}
void CustomCheckBox::SetBorderActiveColor(const QColor& c)
{
	cols.borderActive = c; update();
}
void CustomCheckBox::SetTopPadding(int pad)
{
	topPadding = pad;
}

void CustomCheckBox::paintEvent(QPaintEvent* e)
{
	QPainter painter(this);

	QRect wnd = rect();
	wnd.setTop(wnd.top() + topPadding);
	wnd.setRight(wnd.right() - 1);
	wnd.setBottom(wnd.bottom() - 1);

	QColor bgCol = isChecked() ? cols.bgAct : cols.bg;
	QColor borderCol = isChecked() ? cols.borderActive : cols.border;
	if (hasFocus()) borderCol = cols.borderFocus;

	bgCol = AlphaBlend(cols.bgHov, bgCol, interpolationValue);
	borderCol = AlphaBlend(cols.borderHover, borderCol, interpolationValue);


	QBrush brush(bgCol);
	QPen pen(borderCol, 1.0);

	painter.setPen(pen);
	
	painter.setBrush(brush);
	painter.drawRect(wnd);

}

void CustomCheckBox::enterEvent(QEnterEvent* e)
{
	anim->setDirection(QAbstractAnimation::Direction::Forward);
	anim->start();
	mouseInside = true;
}
void CustomCheckBox::leaveEvent(QEvent* e)
{
	anim->setDirection(QAbstractAnimation::Direction::Backward);
	anim->start();
	mouseInside = false;
}
void CustomCheckBox::mousePressEvent(QMouseEvent* e)
{
	// QCheckBox::mousePressEvent(e);
	pressed();
}
void CustomCheckBox::mouseReleaseEvent(QMouseEvent* e)
{
	// QCheckBox::mouseReleaseEvent(e);
	released();

	const QPointF& p = e->position();
	const QSize& sz = size();
	int x = p.x(); int y = p.y();
	if (x >= 0 && y >= 0 && x <= sz.width() && y < sz.height())
	{
		click();
	}
}

void CustomCheckBox::SetInterpolationValue(qreal val)
{
	interpolationValue = val;
	update();
}
