#pragma once
#include <qpushbutton>
#include <qpropertyanimation.h>

class CustomButton : public  QPushButton
{
	Q_OBJECT
	Q_PROPERTY(qreal value WRITE SetInterpolationValue)
public:
	CustomButton(const QString& text, QWidget* wdt);
	CustomButton(QWidget* wdt);
	CustomButton();




	void SetBackgroundColor(const QColor& c);
	void SetBackgroundHighlightColor(const QColor& c);
	void SetBorderColor(const QColor& c);
	void SetBorderHighlightColor(const QColor& c);
	void SetTextColor(const QColor& c);
	void SetTextHighlightColor(const QColor& c);
protected:
	virtual void paintEvent(QPaintEvent* e) override;

	virtual void enterEvent(QEnterEvent* e) override;
	virtual void leaveEvent(QEvent* e) override;

	void onPress();
	void onRelease();

	void StartAnim(QPropertyAnimation::Direction dir);

	void SetInterpolationValue(qreal value);
	qreal interpolationValue;
	qreal pressedInterpolValue;
	class QPropertyAnimation* pAnimation;


	struct CustomColors
	{
		QColor background;
		QColor highlight;
		QColor border;
		QColor borderHighlight;

		QColor text;
		QColor textHighlight;
	};
	CustomColors cols;
};
