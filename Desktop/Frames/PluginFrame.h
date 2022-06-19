#pragma once
#include <qwidget.h>
#include <qmainwindow.h>
#include "../util/PluginLoader.h"



class PluginFrame : public QWidget
{
public:
	PluginFrame(QMainWindow* parent, PluginClass* plClass);
	~PluginFrame();


private:
};