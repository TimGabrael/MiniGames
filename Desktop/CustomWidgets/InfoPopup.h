#pragma once
#include <qwidget.h>
#include <qmainwindow.h>
#include <qtimer.h>

// ONLY EXISTS ONE TIME, MULTIPLE INSTANCES RESET THE POPUP
class InfoPopup : public QWidget
{
public:
	InfoPopup(QMainWindow* wnd, const QString& content, const QPoint& position, int fontSize, const QColor& color, int ms);


	static bool IsActive();
	static void EndPopup();

private:
	void OnTimerFinish();

	virtual void paintEvent(QPaintEvent* event) override;
	QPointF m_middle;
	QTimer m_timer;
	QString m_msg;
	QColor m_col;

	static InfoPopup* instance;

};


