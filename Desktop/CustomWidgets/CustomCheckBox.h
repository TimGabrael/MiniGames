#pragma once

#include <qcheckbox>

class CustomCheckBox : public QCheckBox
{
	Q_OBJECT
public:
	CustomCheckBox(QWidget* parent);


	// color precedence: focus -> active -> hover -> standard		 [NOTE:  hover gets alpha blended ontop of the others]

	void SetBackgroundColor(const QColor& c);
	void SetBackgroundHoveredColor(const QColor& c);
	void SetBackgroundActiveColor(const QColor& c);
	void SetBorderColor(const QColor& c);
	void SetBorderHoverColor(const QColor& c);
	void SetBorderFocusColor(const QColor& c);
	void SetBorderActiveColor(const QColor& c);

	// used to align items better
	void SetTopPadding(int pad);

private:

	virtual void paintEvent(QPaintEvent* e) override;

	virtual void enterEvent(QEnterEvent* e) override;
	virtual void leaveEvent(QEvent* e) override;

	virtual void mousePressEvent(QMouseEvent* e) override;
	virtual void mouseReleaseEvent(QMouseEvent* e) override;


	void SetInterpolationValue(qreal val);

	struct CustomColor
	{
		QColor bg;
		QColor bgHov;
		QColor bgAct;

		QColor border;
		QColor borderHover;
		QColor borderFocus;
		QColor borderActive;
	};
	class QPropertyAnimation* anim;
	qreal interpolationValue;
	CustomColor cols;
	int topPadding;
	bool mouseInside = false;

};

