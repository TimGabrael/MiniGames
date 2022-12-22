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
	virtual void Initialize(NetServerInfo* s) = 0;
	virtual void Update(float dt) = 0;
	virtual SERVER_PLUGIN_INFO GetInfo() const = 0;
	
	virtual void CleanUp() = 0;

};

#define SERVER_PLUGIN_EXPORTS() extern "C" EXPORT ServerPlugin* GetServerPlugin()
#define SERVER_PLUGIN_EXPORT_DEFINITION(PlugClass, _ID, dt, allow) extern "C" EXPORT ServerPlugin* GetServerPlugin(){ return new ServerPlugin(); }