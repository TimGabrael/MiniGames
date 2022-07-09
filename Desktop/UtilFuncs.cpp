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
#include "MiniGames.h"


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

MainWindow* GetMainWindow()
{
	MainApplication* app = MainApplication::GetInstance();
	return app->mainWindow;
}

MainWindow* GetMainWindowOfWidget(QWidget* wdgt)
{
	QWidget* main = (QWidget*)wdgt->parentWidget();
	if (!main) return (MainWindow*)wdgt;
	QWidget* mainparent = main;
	while (mainparent)
	{
		main = mainparent;
		mainparent = main->parentWidget();
	}
	return (MainWindow*)main;
}
