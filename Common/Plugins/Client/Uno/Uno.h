#pragma once
#include "../PluginCommon.h"
#include "Graphics/Camera.h"
#include "Graphics/Helper.h"
#include "Graphics/Simple3DRendering.h"
#include "Graphics/Renderer.h"
#include "Card.h"
#include "../InputStates.h"
#include "Animator.h"
#include "Pointer.h"




struct PlayerInfo
{
	std::string name;
	uint16_t id = 0;
	CardHand* hand = nullptr;
};

enum GAME_STATE : uint8_t
{
	STATE_PENDING,
	STATE_PLAYING,
};

struct GameStateData
{
	GameStateData() : topCard(CARD_UNKNOWN, CARD_COLOR_UNKOWN) {}
	ColorPicker picker;
	std::vector<PlayerInfo> players;
	std::vector<CardHand>* hands;
	CardData topCard;
	uint16_t playerInTurn = 0xFFFF;
	GAME_STATE state = STATE_PENDING;
	bool isChoosingColor = false;
	bool isClockwise = true;

	PlayerInfo* GetPlayerInfo(uint16_t id) { for (int i = 0; i < players.size(); i++) { if (players.at(i).id == id) return &players.at(i); } return nullptr; }
	PlayerInfo* GetPlayerInfoForcefully(uint16_t id);
	CardHand* GetHand(uint16_t id) { for (int i = 0; i < hands->size(); i++) { if (hands->at(i).handID == id) return &hands->at(i); } return nullptr; }
	CardHand* GetHandForcefully(uint16_t id);

	PlayerInfo* GetLocalPlayer();
	CardHand* GetLocalPlayerHand();
};


struct UnoGlobals
{
	GameStateData game;

	Camera playerCam;
	OrthographicCamera shadowCam;
	glm::vec3 lightDir;
	MovementComponent moveComp;
	SingleFBO reflectFBO;
	SceneRenderData rendererData;
	EnvironmentMaps environment;
	GLuint refGroundTexture = 0;
	S3DCombinedBuffer platform;
	std::vector<CardHand> hands;
	MouseState ms;
	Pointer p;
	CardStack stack;
	CardsInAnimation anims;
	CardDeck deck;
	ColorPicker picker;

	PScene UnoScene = nullptr;
	SceneObject* basePlatform = nullptr;
	CardSceneObject* cardRenderObject = nullptr;
	int offscreenX = 0;
	int offscreenY = 0;
	float cardHandlingTimer = 0.0f;
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

	ApplicationData* backendData;
	UnoGlobals* g_objs;
	bool initialized = false;

};

UnoPlugin* GetInstance();
GameStateData* GetGameState();

PLUGIN_EXPORTS();
