#pragma once
#include "Network/NetworkBase.h"
#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#elif defined(EMSCRIPTEN)
#include "emscripten.h"
#define EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define EXPORT 
#endif

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
#define SERVER_PLUGIN_EXPORT_DEFINITION(PlugClass) extern "C" EXPORT ServerPlugin* GetServerPlugin(){ return new PlugClass(); }