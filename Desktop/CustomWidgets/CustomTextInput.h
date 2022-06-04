#pragma once
#include <qlineedit.h>


class CustomTextInput : public QLineEdit
{
public:
	CustomTextInput(QWidget* parent);
	CustomTextInput();


protected:

	virtual void paintEvent(QPaintEvent* e) override;

};