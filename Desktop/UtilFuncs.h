#pragma once
#include <QMainWindow>
#include "MiniGames.h"

QColor AlphaBlend(const QColor& top, const QColor& bottom, float topalpha);


QIcon LoadSvgWithColor(const QString& path, const QColor& c, const QSize& sz);

MainWindow* GetMainWindow();

void SafeAsyncUI(void(*uiFunction)(MainWindow* wnd));

bool TryConnectToServer();

void ClearLayout(QLayout* lay);