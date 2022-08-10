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
	CardHand hand;
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
};

enum UNO_MESSAGES : uint32_t
{
	UNO_PULL_CARD_REQUEST = (uint32_t)PacketID::NUM_PACKETS,
	UNO_PULL_CARD_RESPONSE,
	UNO_PLAY_CARD_REQUEST,
	UNO_PLAY_CARD_RESPONSE
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

	virtual void NetworkCallback(Packet* packet) override;
	virtual void FetchSyncData(std::string& str) override;
	virtual void HandleAddClient(const ClientData* added) override;
	virtual void HandleRemovedClient(const ClientData* removed) override;
	virtual void HandleSync(const std::string& syncData) override;

	virtual void CleanUp() override;

	ApplicationData* backendData;	// AAssetManager* for android, else nullptr
	bool initialized = false;

	UnoGlobals* g_objs;
};


UnoPlugin* GetInstance();
void SendNetworkData(uint32_t packetID, uint32_t group, uint16_t additionalFlags, uint16_t clientID, size_t size, const void* data);
void SendNetworkData(uint32_t packetID, uint32_t group, uint16_t additionalFlags, uint16_t clientID, const std::string& str);


PLUGIN_EXPORTS();