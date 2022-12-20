#include "UtilFuncs.h"
#include <qfile.h>
#include <QtSvg/qsvgrenderer.h>
#include <qpainter.h>
#include <qmainwindow.h>
#include <qlayout.h>
#include "Application.h"
#include <qthread.h>
#include <qtimer.h>
#include "MiniGames.h"
#include  "CustomWidgets/InfoPopup.h"


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

void SafeAsyncUI(void(*uiFunction)(MainWindow* wnd))
{
	MainApplication* app = MainApplication::GetInstance();
	MainWindow* main = app->mainWindow;
	if (main->thread() == QThread::currentThread())
	{
		uiFunction(main);
	}
	else
	{
		QTimer::singleShot(0, main, std::bind(uiFunction, main));
	}
}

bool TryConnectToServer(const std::string& name)
{
	MainApplication* app = MainApplication::GetInstance();
	if (app->client->state != NetClient::Disconnected) return true;
	MainWindow* main = app->mainWindow;
	
	size_t idx = app->input.ip.find(":");
	NetResult err;
	err.reason = "Failed to Parse IP-Port";
	err.success = false;
	if (idx != -1)
	{
		uint32_t port = atoi(&app->input.ip.at(idx + 1));
		if (port != 0)
		{
			LOG("PORT: %d\n", port);
			err = app->client->Connect(app->input.ip.c_str(), port, name);
		}
	}
	if(!err.success)
	{
		QTimer::singleShot(0, main, [&]() {
			MainApplication* app = MainApplication::GetInstance();
			auto rect = main->geometry();
			//InfoPopup* popUp = new InfoPopup(main, "FAILED TO CONNECT TO SERVER", QPoint(rect.width() / 2, rect.height() - 100), 20, 0xFFFF0000, 3000);
			InfoPopup* popUp = new InfoPopup(main, err.reason.c_str(), QPoint(rect.width() / 2, rect.height() - 100), 20, 0xFFFF0000, 3000);
		});
		return false;
	}

	return true;
}

void ClearLayout(QLayout* lay)
{
	QLayoutItem* item;
	while ((item = lay->takeAt(0)) != nullptr)
	{
		delete item->widget();
		delete item;
	}
}