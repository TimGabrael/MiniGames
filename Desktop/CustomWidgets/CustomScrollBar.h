#pragma once

#include <qscrollbar.h>

class CustomScrollBar : public QScrollBar
{
public:
	CustomScrollBar(QWidget* parent);

protected:
	virtual void paintEvent(QPaintEvent* e) override;

};