#pragma once


struct PLUGIN_INFO
{
	char ID[19]{ 0 };
	const char* previewResource;
};

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#elif defined(EMSCRIPTEN)
#include "emscripten.h"
#define EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define EXPORT
#endif

class PluginClass
{
public:
	virtual PLUGIN_INFO GetPluginInfos() = 0;
	virtual void PluginInit(void* backendData) = 0;
	virtual void PluginRender(void* backendData) = 0;
};


#define PLUGIN_EXPORTS() extern "C" EXPORT PluginClass* GetPlugin()
#define PLUGIN_EXPORT_DEFINITION(PlugClass, ID) extern "C" EXPORT PluginClass* GetPlugin(){ return new PlugClass(); } const char* _Plugin_Export_ID_Value = ID;