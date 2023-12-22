#pragma once

#include <qscrollbar.h>

class CustomScrollBar : public QScrollBar
{
public:
	CustomScrollBar(QWidget* parent);
    virtual ~CustomScrollBar();

protected:
	virtual void paintEvent(QPaintEvent* e) override;

};
