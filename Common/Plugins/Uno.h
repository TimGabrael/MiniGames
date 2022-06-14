#pragma once
#include "PluginCommon.h"




class UnoPlugin : public PluginClass
{
	virtual PLUGIN_INFO GetPluginInfos() override;
	virtual void PluginInit(void* backendData) override;
	virtual void PluginRender(void* backendData) override;
};

PLUGIN_EXPORTS(UnoPlugin);