#pragma once
#include "Network/NetworkBase.h"

struct SERVER_PLUGIN_INFO
{
	char ID[19]{ 0 };
	float updateTimer;
	bool allowLateJoin;
};


struct ServerPlugin
{
	virtual ~ServerPlugin() {};
	virtual void Initialize(NetServerInterface* s) = 0;
	virtual void Update(float dt) = 0;
	virtual SERVER_PLUGIN_INFO GetInfo() const = 0;
	
};

#define SERVER_PLUGIN_EXPORTS() extern "C" EXPORT ServerPlugin* GetPlugin()
#define SERVER_PLUGIN_EXPORT_DEFINITION(PlugClass, _ID, dt, allow) extern "C" EXPORT ServerPlugin* GetPlugin(){ return new ServerPlugin(); }