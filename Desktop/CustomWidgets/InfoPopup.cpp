#include "InfoPopup.h"
#include <qpainter.h>
#include "logging.h"
#include "../UtilFuncs.h"
#include <qlayout.h>

InfoPopup::InfoPopup(QMainWindow* wnd, const QString& content, const QPoint& position, int fontSize, const QColor& color, int duration) : QWidget(wnd)
{
	this->setAttribute(Qt::WA_TransparentForMouseEvents);
	const QSize wndSize = wnd->size();
	this->setMinimumSize(-1, -1);
	this->setFont(wnd->font());
	this->m_timer.setInterval(duration);
	this->m_msg = content;
	this->m_col = color;
	QFont font;
	font.setPixelSize(fontSize);
	this->setFont(font);

	m_middle = position;
	m_middle.setX(m_middle.x() / (float)wndSize.width());
	m_middle.setY(m_middle.y() / (float)wndSize.height());

	connect(&m_timer, &QTimer::timeout, this, &InfoPopup::OnTimerFinish);
	m_timer.start();
	this->show();
}



void InfoPopup::OnTimerFinish()
{
	this->deleteLater();
}

void InfoPopup::paintEvent(QPaintEvent* event)
{
	const QSize wndSize = parentWidget()->size();
	this->setMinimumSize(wndSize);
	
	QFontMetrics mets(font());
	QRect boundRect = mets.boundingRect(m_msg);
	
	QPoint drawMid = QPoint(m_middle.x() * wndSize.width() - boundRect.width() / 2, m_middle.y() * wndSize.height());

	QPainter painter(this);
	painter.setPen(m_col);
	painter.drawText(drawMid, m_msg);

}