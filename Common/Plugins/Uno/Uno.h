#pragma once
#include "../PluginCommon.h"




class UnoPlugin : public PluginClass
{
	virtual PLUGIN_INFO GetPluginInfos() override;
	virtual void Init(ApplicationData* data) override;
	virtual void Resize(ApplicationData* data) override;
	virtual void Render(ApplicationData* data) override;

	virtual void MouseCallback(const PB_MouseData* mData) override;
	virtual void KeyDownCallback(Key k, bool isRepeat) override;
	virtual void KeyUpCallback(Key k, bool isRepeat) override;

	virtual void TouchDownCallback(int x, int y, int touchID) override;
	virtual void TouchUpCallback(int x, int y, int touchID) override;
	virtual void TouchMoveCallback(int x, int y, int dx, int dy, int touchID) override;

	virtual void CleanUp() override;

	ApplicationData* backendData;	// AAssetManager* for android, else nullptr
	bool initialized = false;
};

PLUGIN_EXPORTS();