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


	virtual void FetchSyncData(std::string& str) override;
	virtual void HandleAddClient(const ClientData* added) override;
	virtual void HandleRemovedClient(const ClientData* removed) override;

	virtual void HandleNetworkMessage(Packet* packet) override;
	virtual void HandleSync(const std::string& syncData) override;


	static PluginClass* activePlugin;
private:
};