#pragma once
#include "../Network/Networking.h"

struct SERVER_PLUGIN_INFO
{
	char ID[19]{ 0 };
	float updateTimer;
	bool allowLateJoin;
};

struct ServerPlugin
{
	virtual ~ServerPlugin() {};
	virtual void Initialize() = 0;
	virtual void Update(float dt) = 0;
	virtual SERVER_PLUGIN_INFO GetInfo() const = 0;
	UDPServerSocket* socket;
};

#define SERVER_PLUGIN_EXPORTS() extern "C" EXPORT ServerPlugin* GetPlugin()
#define SERVER_PLUGIN_EXPORT_DEFINITION(PlugClass, _ID, dt, allow) extern "C" EXPORT ServerPlugin* GetPlugin(){ return new ServerPlugin(); }