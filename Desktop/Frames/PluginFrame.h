#pragma once
#include <qwidget.h>
#include <qmainwindow.h>
#include "../util/PluginLoader.h"

#include "StateFrame.h"


class PluginFrame : public StateFrame
{
public:
	PluginFrame(QMainWindow* parent);
	~PluginFrame();


	

	static PluginClass* activePlugin;
	static uint16_t activePluginID;
private:
};