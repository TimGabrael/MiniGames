#pragma once
#include "KeyboardKeys.h"
#include <string>
#include <vector>

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

class TCPSocket;
enum PLATFORM_ID
{
	PLATFORM_ID_WINDOWS = 1,
	PLATFORM_ID_LINUX = 1 << 1,
	PLATFORM_ID_OSX = 1 << 2,

	PLATFORM_ID_ANDROID = 1 << 3,
	PLATFORM_ID_IOS = 1 << 4,

	PLATFORM_ID_EMSCRIPTEN = 1 << 5,	// the black sheep of platforms

	
	PLATFORM_MASK_DESKTOP = PLATFORM_ID_WINDOWS | PLATFORM_ID_LINUX | PLATFORM_ID_OSX,
	PLATFORM_MASK_MOBILE = PLATFORM_ID_ANDROID | PLATFORM_ID_IOS,
};

struct ClientData
{
	std::string name;
	uint32_t groupMask;
};

struct ApplicationData
{
	PLATFORM_ID platform;
	std::vector<ClientData> players;
	ClientData localPlayer;
	int addedClientIdx = -1;
	ClientData* removedClient = nullptr;
	uint8_t localPlayerID[16];
	std::string roomName;
	std::string tempSyncDataStorage;
	void* assetManager;
	TCPSocket* socket;
};


struct PB_MouseData
{
	int xPos;
	int yPos;
	int dx;
	int dy;

	bool lDown = false;
	bool rDown = false;
	bool mDown = false;
	bool lPressed = false;
	bool rPressed = false;
	bool mPressed = false;
	bool lUp = false;
	bool rUp = false;
	bool mUp = false;
};



class PluginClass
{
public:
	virtual PLUGIN_INFO GetPluginInfos() = 0;
	virtual void Init(ApplicationData* data) = 0;
	virtual void Resize(ApplicationData* data) = 0;
	virtual void Render(ApplicationData* data) = 0;

	virtual void MouseCallback(const PB_MouseData* mData) = 0;

	virtual void KeyDownCallback(Key k, bool isRepeat) = 0;
	virtual void KeyUpCallback(Key k, bool isRepeat) = 0;

	virtual void TouchDownCallback(int x, int y, int touchID) = 0;
	virtual void TouchUpCallback(int x, int y, int touchID) = 0;
	virtual void TouchMoveCallback(int x, int y, int dx, int dy, int touchID) = 0;

	virtual void CleanUp() = 0;

	int sizeX, sizeY;
	int framebufferX, framebufferY;
};


#define PLUGIN_EXPORTS() extern "C" EXPORT PluginClass* GetPlugin()
#define PLUGIN_EXPORT_DEFINITION(PlugClass, ID) extern "C" EXPORT PluginClass* GetPlugin(){ return new PlugClass(); } const char* _Plugin_Export_ID_Value = ID;