#include "CustomTextInput.h"
#include <qpainter.h>
#include <qbrush.h>
#include <qstyle.h>
#include <iostream>
#include <qstyleoption.h>
#include <qstylefactory.h>
#include <QtWidgets/qstyleoption.h>

CustomTextInput::CustomTextInput(QWidget* parent) : QLineEdit(parent)
{
	setAlignment(Qt::AlignmentFlag::AlignLeft);

	QPalette pal = palette();
	pal.setBrush(QPalette::Base, Qt::transparent);
	pal.setBrush(QPalette::Text, Qt::white);
	QBrush br(0xA0A0A0);
	pal.setBrush(QPalette::ColorRole::PlaceholderText, br);
	this->setPalette(pal);

	this->setFrame(false);
	setTextMargins(2, 1, 2, 1);
	
}
CustomTextInput::CustomTextInput()
{

}

void CustomTextInput::paintEvent(QPaintEvent* e)
{
	QPainter painter(this);
	
	QRect wnd = rect();
	wnd.setRight(wnd.right() - 1);
	wnd.setBottom(wnd.bottom() - 1);
	
	QColor rectColOutline = hasFocus() ? 0x4040F0 : 0x204080;
	QColor rectCol = 0x202020;
	painter.setPen(rectColOutline);
	painter.setBrush(rectCol);
	painter.drawRoundedRect(wnd, 6.0f, 6.0f);



	QLineEdit::paintEvent(e);
	
}