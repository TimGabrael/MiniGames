#pragma once
#include "../PluginCommon.h"
#include "Graphics/Camera.h"
#include "Graphics/Simple3DRendering.h"
#include "Graphics/Renderer.h"
#include "Card.h"
#include "../InputStates.h"
#include "Animator.h"
#include "Pointer.h"

struct PlayerInfo
{
	std::string name;
	uint32_t id = 0;
	CardHand* hand = nullptr;
};

enum GAME_STATE : uint32_t
{
	STATE_PENDING,
	STATE_PLAYING,
};

struct UnoGlobals
{
	Camera playerCam;
	OrthographicCamera shadowCam;
	glm::vec3 lightDir;
	MovementComponent moveComp;
	SingleFBO reflectFBO;
	SceneRenderData rendererData;
	GLuint skybox;
	GLuint refGroundTexture;
	S3DCombinedBuffer platform;
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
	float cardHandlingTimer = 0.0f;
	uint32_t localPlayerIndex = 0;
};
struct GameStateData
{
	std::vector<PlayerInfo> players;
	std::vector<CardHand>* hands;
	int playerInTurn;
	GAME_STATE state = STATE_PENDING;
	bool isChoosingColor = false;
	bool isClockwise = true;
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
	UnoGlobals* g_objs;
	bool initialized = false;

};


UnoPlugin* GetInstance();
GameStateData* GetGameState();
void SendNetworkData(uint32_t packetID, uint32_t group, uint16_t additionalFlags, uint16_t clientID, size_t size, const void* data);
void SendNetworkData(uint32_t packetID, uint32_t group, uint16_t additionalFlags, uint16_t clientID, const std::string& str);


PLUGIN_EXPORTS();