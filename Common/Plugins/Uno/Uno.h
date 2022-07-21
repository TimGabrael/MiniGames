#pragma once
#include "../PluginCommon.h"
#include "Graphics/Camera.h"
#include "Graphics/Simple3DRendering.h"
#include "Card.h"
#include "../InputStates.h"
#include "Animator.h"
#include "Pointer.h"

struct PlayerInfo
{
	std::string name;
	CardHand hand;
};

struct UnoGlobals
{
	Camera playerCam;
	MovementComponent moveComp;
	SingleFBO reflectFBO;
	GLuint skybox;
	S3DCombinedBuffer platform;
	CardHand* localPlayer;
	std::vector<CardHand> hands;
	MouseState ms;
	Pointer p;
	CardStack stack;
	CardsInAnimation anims;
	CardDeck deck;
	ColorPicker picker;

	PScene UnoScene;
	SceneObject* basePlatform;
	CardSceneObject* cardRenderObject;
	int offscreenX;
	int offscreenY;
};


class UnoPlugin : public PluginClass
{
public:
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

	UnoGlobals* g_objs;
};

UnoPlugin* GetInstance();

PLUGIN_EXPORTS();