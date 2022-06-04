#include "CustomButton.h"
#include <qpropertyanimation.h>
#include <qgraphicseffect.h>

#include <qevent.h>
#include "../Utilfuncs.h"
#include <qpainter.h>
#include <qstatictext.h>
#include <qpropertyanimation.h>


#include <iostream>


CustomButton::CustomButton(const QString& text, QWidget* wdt) : QPushButton(text, wdt)
{
	cols.background = 0x202020;
	cols.highlight = 0xFC6603;
	cols.border = 0x202040;
	cols.borderHighlight = 0x404080;
	cols.text = 0xD0D0D0;
	cols.textHighlight = 0xD0D0F0;
	pAnimation = new QPropertyAnimation(this, "value");

	interpolationValue = 0.0;
	pAnimation->setStartValue(0.0);
	pAnimation->setEndValue(0.4);
	pAnimation->setDuration(100);

	connect(this, &QPushButton::pressed, this, &CustomButton::onPress);
	connect(this, &QPushButton::released, this, &CustomButton::onRelease);
}
CustomButton::CustomButton(QWidget* wdt) : QPushButton(wdt)
{
	cols.background = 0x202020;
	cols.highlight = 0xFC6603;
	cols.border = 0x202040;
	cols.borderHighlight = 0x404080;
	cols.text = 0xD0D0D0;
	cols.textHighlight = 0xD0D0F0;
	pAnimation = new QPropertyAnimation(this, "value");

	pAnimation->setStartValue(0.0);
	pAnimation->setEndValue(0.4);
	pAnimation->setDuration(100);
	interpolationValue = 0.0f;

	connect(this, &QPushButton::pressed, this, &CustomButton::onPress);
	connect(this, &QPushButton::released, this, &CustomButton::onRelease);
}

CustomButton::CustomButton() : QPushButton()
{
	
}

void CustomButton::SetBackgroundColor(const QColor& c)
{
	cols.background = c;
	update();
}

void CustomButton::SetBackgroundHighlightColor(const QColor& c)
{
	cols.highlight = c;
	update();
}

void CustomButton::SetBorderColor(const QColor& c)
{
	cols.border = c;
	update();
}

void CustomButton::SetBorderHighlightColor(const QColor& c)
{
	cols.borderHighlight = c;
	update();
}

void CustomButton::SetTextColor(const QColor& c)
{
	cols.text = c;
	update();
}

void CustomButton::SetTextHighlightColor(const QColor& c)
{
	cols.textHighlight = c;
	update();
}

void CustomButton::paintEvent(QPaintEvent* e)
{
	QPainter painter(this);
	
	QRect wnd = rect();
	QString str = text();

	wnd.setRight(wnd.right() - 1);
	wnd.setBottom(wnd.bottom() - 1);


	QRect bound = fontMetrics().boundingRect(str);
	const int width = bound.right() - bound.left();

	const int height = bound.bottom() - bound.top();
	const int offsetX = ((wnd.right() - wnd.left()) - width) / 2;
	const int offsetY = ((wnd.bottom() - wnd.top()) - height) / 2;


	QColor outline = hasFocus() ? cols.borderHighlight : cols.border;

	double alpha = isDown() ? 0.2 + interpolationValue : interpolationValue;
	QColor inner = AlphaBlend(cols.highlight, cols.background, alpha);



	painter.setPen(outline);
	painter.setBrush(inner);
	painter.drawRect(wnd);
	QColor textColor = AlphaBlend(cols.textHighlight, cols.text, alpha);
	painter.setPen(textColor);
	painter.drawStaticText(wnd.left() + offsetX, wnd.top() + offsetY, QStaticText(str));
}

void CustomButton::enterEvent(QEnterEvent* e)
{
	QPushButton::enterEvent(e);
	StartAnim(QAbstractAnimation::Direction::Forward);
}
void CustomButton::leaveEvent(QEvent* e)
{
	QPushButton::leaveEvent(e);
	pressedInterpolValue = 0.0;
	StartAnim(QAbstractAnimation::Direction::Backward);
}

void CustomButton::onPress()
{
	StartAnim(QAbstractAnimation::Direction::Forward);
}
void CustomButton::onRelease()
{
	StartAnim(QAbstractAnimation::Direction::Backward);
}

void CustomButton::StartAnim(QPropertyAnimation::Direction dir)
{
	if(dir == QAbstractAnimation::Direction::Forward)
	{
		this->pressedInterpolValue = interpolationValue;
	}
	pAnimation->setStartValue(pressedInterpolValue);
	this->pAnimation->setDirection(dir);
	pAnimation->start();
}

void CustomButton::SetInterpolationValue(qreal val)
{
	interpolationValue = val;
	update();
}