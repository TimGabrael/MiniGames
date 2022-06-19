#include "UtilFuncs.h"
#include <qfile.h>
#include <QtSvg/qsvgrenderer.h>
#include <qpainter.h>
#include <qmainwindow.h>
#include <qlayout.h>
#include "Application.h"
#include <iostream>
#include <qopengl.h>
#include <qopenglcontext.h>
#include <qopenglcontext_platform.h>
#include <qopenglfunctions.h>


QColor AlphaBlend(const QColor& top, const QColor& bottom, float topalpha)
{
	return QColor(top.red() * topalpha + bottom.red() * (1.0f - topalpha), top.green() * topalpha + bottom.green() * (1.0f - topalpha), top.blue() * topalpha + bottom.blue() * (1.0f - topalpha), top.alpha() * topalpha + bottom.alpha() * (1.0f - topalpha));
}

QIcon LoadSvgWithColor(const QString& path, const QColor& c, const QSize& sz)
{
	QFile file(path);
	file.open(QIODevice::ReadOnly);
	QByteArray baData = file.readAll();
	std::string str = baData.toStdString();
	size_t idx = str.find("fill=");
	if (idx != -1)
	{
		size_t endIdx = idx + 6;
		for (; endIdx < str.size(); endIdx++)
		{
			if (str.at(endIdx) == '\"')
				break;
		}
		str.replace(str.begin() + idx + 6, str.begin() + endIdx, c.name().toStdString().c_str());
	}


	QSvgRenderer svgRenderer(QByteArray(str.data()));
	QPixmap pix(sz);
	pix.fill(Qt::transparent);
	QPainter pixPainter(&pix);

	svgRenderer.render(&pixPainter);

	return QIcon(pix);
}

QMainWindow* GetMainWindow(QWidget* wdgt)
{
	QWidget* main = (QWidget*)wdgt->parentWidget();
	if (!main) return (QMainWindow*)wdgt;
	QWidget* mainparent = main;
	while (mainparent)
	{
		main = mainparent;
		mainparent = main->parentWidget();
	}
	return (QMainWindow*)main;
}
QMainWindow* OverdrawMainWindow(QWidget* wdgt)
{
	QMainWindow* main = GetMainWindow(wdgt);
	for (int i = 0; i < main->layout()->count(); i++)
	{
		QLayoutItem* childWidget = main->layout()->takeAt(0);
		
		if (childWidget->widget())
		{
			MainApplication::GetInstance()->backgroundWidget = childWidget->widget();
			MainApplication::GetInstance()->backgroundWidget->setVisible(false);
			break;
		}
		
	}
	return main;
}
QMainWindow* DeleteAndShowBehind(QWidget* wdgt)
{
	QMainWindow* main = GetMainWindow(wdgt);
	QWidget* bg = MainApplication::GetInstance()->backgroundWidget;
	if (bg)
	{
		bg->setVisible(true);
		for (int i = 0; i < main->layout()->count(); i++)
		{
			QLayoutItem* item = main->layout()->itemAt(i);
			if (item && item->widget())
			{
				if (item->widget() == bg) {
					return main;
				}
			}
		}
		main->setCentralWidget(bg);
	}
	return main;
}
QMainWindow* BlankMainWindow(QWidget* wdgt)
{
	QMainWindow* main = GetMainWindow(wdgt);
	QLayoutItem* item = nullptr;
	QWidget* bg = MainApplication::GetInstance()->backgroundWidget;
	if (main->layout())
	{
		while ((item = main->layout()->takeAt(0)) != nullptr)
		{
			QWidget* cur = item->widget();
			if (cur)
			{
				if (cur == bg) MainApplication::GetInstance()->backgroundWidget = nullptr;
				main->layout()->removeWidget(cur);
				cur->deleteLater();
				delete item;
			}
		}
	}
	return main;
}