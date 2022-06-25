#pragma once
#include "PluginCommon.h"




class UnoPlugin : public PluginClass
{
	virtual PLUGIN_INFO GetPluginInfos() override;
	virtual void Init(void* backendData) override;
	virtual void Resize(void* backendData) override;
	virtual void Render(void* backendData) override;

	virtual void MouseCallback(const PB_MouseData* mData) override;
	virtual void KeyDownCallback(Key k, bool isRepeat) override;
	virtual void KeyUpCallback(Key k, bool isRepeat) override;

	void* backendData;	// AAssetManager* for android, else nullptr
	bool initialized = false;
};

PLUGIN_EXPORTS();