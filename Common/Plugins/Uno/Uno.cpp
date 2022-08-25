#include "Uno.h"
#include <string>
#include "Graphics/Helper.h"
#include "Graphics/Camera.h"
#include "logging.h" // REMINDER USE THIS !! USE THIS NOT <iostream> !!
#include "Graphics/PbrRendering.h"
#include "Graphics/UiRendering.h"
#include <chrono>
#include <algorithm>
#include "Graphics/Renderer.h"
#include "Graphics/ReflectiveSurfaceRendering.h"
#include "Graphics/BloomRendering.h"
#include "Network/Networking.h"
#include "imgui.h"
#include "Messages/UnoMessages.pb.h"
#include "Network/Messages/sync.pb.h"

#define _USE_MATH_DEFINES
#include <math.h>

PLUGIN_EXPORT_DEFINITION(UnoPlugin, "a3fV-6giK-10Eb-2rdT", "Assets/Uno.png");
#define SHADOW_TEXTURE_SIZE 2048 * 4




void PrintGLMMatrix(const glm::mat4& m)
{
	LOG("%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n",
		m[0][0], m[1][0], m[2][0], m[3][0],
		m[0][1], m[1][1], m[2][1], m[3][1],
		m[0][2], m[1][2], m[2][2], m[3][2],
		m[0][3], m[1][3], m[2][3], m[3][3]);
}

UnoPlugin* instance = nullptr;
static GameStateData game;

UnoPlugin* GetInstance()
{
	return instance;
}
GameStateData* GetGameState()
{
	return &game;
}
void SendNetworkData(uint32_t packetID, uint32_t group, uint16_t additionalFlags, uint16_t clientID, size_t size, const void* data)
{
	instance->backendData->_sendDataFunction(packetID, group, additionalFlags, clientID, size, data);
}
void SendNetworkData(uint32_t packetID, uint32_t group, uint16_t additionalFlags, uint16_t clientID, const std::string& str)
{
	instance->backendData->_sendDataFunction(packetID, group, additionalFlags, clientID, str.size(), str.data());
}





static int AddPlayer(const ClientData* added)
{
	int pos = game.hands->size();
	game.hands->push_back(added->clientID);
	game.players.push_back({ added->name, added->clientID, nullptr });
	const size_t num = game.players.size();
	for (size_t i = 0; i < num; i++)
	{
		game.hands->at(i).rotation = i * 360.0f / (float)num;
		game.hands->at(i).willBeSorted = false;
	}
	for (int i = 0; i < game.players.size(); i++)
	{
		if (instance->backendData->localPlayer.clientID == game.players.at(i).id)
		{
			instance->g_objs->localPlayerIndex = i;
		}
		for (int j = 0; j < game.hands->size(); j++)
		{
			if (game.players.at(i).id == game.hands->at(j).handID)
			{
				game.players.at(i).hand = &game.hands->at(j);
				break;
			}
		}
	}
	const float localPlayerRot = game.players.at(instance->g_objs->localPlayerIndex).hand->rotation;
	for (int i = 0; i < game.hands->size(); i++)
	{
		game.hands->at(i).rotation -= localPlayerRot;
	}
	game.players.at(instance->g_objs->localPlayerIndex).hand->willBeSorted = true;	// only sort local player cards
	return pos;
}
static void RemovePlayer(const ClientData* removed)
{
	for (int i = 0; i < game.players.size(); i++)
	{
		if (game.players.at(i).id == removed->clientID) {
			game.players.erase(game.players.begin() + i);
			break;
		}
	}
	for (int i = 0; i < game.hands->size(); i++)
	{
		if (game.hands->at(i).handID == removed->clientID)
		{
			game.hands->erase(game.hands->begin() + i);
			break;
		}
	}
	for (int i = 0; i < game.players.size(); i++)
	{
		for (int j = 0; j < game.hands->size(); j++)
		{
			if (game.players.at(i).id == game.hands->at(j).handID)
			{
				game.players.at(i).hand = &game.hands->at(j);
				break;
			}
		}
	}
}


static std::mutex data_mutex;
static std::vector<std::vector<CARD_ID>> pulled_card_list;
struct PlayedCardData
{
	uint32_t playerIdx = -1;
	CARD_ID  card = (CARD_ID)-1;
}played_card_data;
static void SyncSinglePlayer(uint32_t senderId)
{
	std::string syncData;
	Base::SyncResponse sync;
	instance->FetchSyncData(syncData);
	sync.set_state(syncData);
	SendNetworkData((uint32_t)PacketID::FORCE_SYNC, ADDITIONAL_DATA_FLAG_ADMIN, AdditionalDataFlags::ADDITIONAL_DATA_FLAG_ADMIN | AdditionalDataFlags::ADDITIONAL_DATA_FLAG_TO_SENDER_ID, senderId, sync.SerializeAsString());
}
void UnoPlugin::NetworkCallback(Packet* packet) // NOT IN MAIN THREAD
{
	if (packet->header.type == UNO_MESSAGES::UNO_PULL_CARD_REQUEST && (backendData->localPlayer.groupMask & ADMIN_GROUP_MASK))
	{
		if (game.playerInTurn >= 0 && game.playerInTurn < game.players.size() && packet->header.senderID == game.hands->at(game.playerInTurn).handID)
		{
			CARD_ID pulled = g_objs->deck.PullCard();
			Uno::PullCardResponse resp;
			auto* pull = resp.add_pullresponses();
			pull->add_cards((uint32_t)pulled);
			pull->set_playerid(game.players.at(game.playerInTurn).id);
			SendNetworkData(UNO_MESSAGES::UNO_PULL_CARD_RESPONSE, LISTEN_GROUP_ALL, AdditionalDataFlags::ADDITIONAL_DATA_FLAG_ADMIN, backendData->localPlayer.clientID, resp.SerializeAsString());
		}
		else
		{
			SyncSinglePlayer(packet->header.senderID);
		}
	}
	else if (packet->header.type == UNO_MESSAGES::UNO_PULL_CARD_RESPONSE && packet->header.additionalData & AdditionalDataFlags::ADDITIONAL_DATA_FLAG_ADMIN)
	{
		Uno::PullCardResponse resp;
		resp.ParseFromArray(packet->body.data(), packet->body.size());
		data_mutex.lock();
		for (const auto& pull : resp.pullresponses())
		{
			const uint32_t id = pull.playerid();
			int playerIdx = 0;
			for (uint32_t i = 0; i < game.players.size(); i++)
			{
				if (id == game.players.at(i).id)
				{
					playerIdx = i;
					break;
				}
			}
			while(pulled_card_list.size() < playerIdx) {
				pulled_card_list.push_back({ });
			}
			for (uint32_t card : pull.cards())
			{
				pulled_card_list.at(playerIdx).push_back((CARD_ID)card);
			}
		}
		data_mutex.unlock();
	}
	else if (packet->header.type == UNO_MESSAGES::UNO_PLAY_CARD_REQUEST && (backendData->localPlayer.groupMask & ADMIN_GROUP_MASK))
	{
		if (game.playerInTurn >= 0 && game.playerInTurn < game.players.size() && packet->header.senderID == game.hands->at(game.playerInTurn).handID)
		{
			Uno::PlayCardRequest req;
			req.ParseFromArray(packet->body.data(), packet->body.size());

			COLOR_ID topCardColorID;
			CARD_ID topCard = g_objs->stack.GetTop(topCardColorID);
			if (CardIsPlayable(topCard, (CARD_ID)req.card(), topCardColorID))
			{
				Uno::PlayCard resp;
				resp.set_playerid(game.players.at(game.playerInTurn).id);
				resp.set_card(req.card());
				game.playerInTurn = (game.playerInTurn + 1) % game.players.size();
				resp.set_nextplayerid(game.players.at(game.playerInTurn).id);

				data_mutex.lock();
				played_card_data.playerIdx = game.playerInTurn;
				played_card_data.card = (CARD_ID)req.card();
				data_mutex.unlock();

				SendNetworkData(UNO_MESSAGES::UNO_PLAY_CARD_RESPONSE, LISTEN_GROUP_ALL, AdditionalDataFlags::ADDITIONAL_DATA_FLAG_ADMIN, backendData->localPlayer.clientID, resp.SerializeAsString());
			}
			else
			{
				SyncSinglePlayer(packet->header.senderID);
			}
		}
	}
	else if (packet->header.type == UNO_MESSAGES::UNO_PLAY_CARD_RESPONSE && (packet->header.additionalData & AdditionalDataFlags::ADDITIONAL_DATA_FLAG_ADMIN))
	{
		Uno::PlayCard resp;
		resp.ParseFromArray(packet->body.data(), packet->body.size());
		const uint32_t id = resp.playerid();
		const uint32_t nextId = resp.nextplayerid();
		data_mutex.lock();
		bool curFound = false;
		bool nextFound = false;
		played_card_data.playerIdx = -1;
		for (int i = 0; i < game.players.size(); i++)
		{
			if (!curFound && game.players.at(i).id == id)
			{
				played_card_data.playerIdx = i;
				played_card_data.card = (CARD_ID)resp.card();
				curFound = true;
			}
			if (!nextFound && game.players.at(i).id == nextId)
			{
				game.playerInTurn = i;
				nextFound = true;
			}
			if (curFound && nextFound)
				break;
		}
		if (id >= game.players.size()) nextFound = true;	// last move
		if (!curFound || !nextFound) {
			SendNetworkData((uint32_t)PacketID::SYNC_REQUEST, ADMIN_GROUP_MASK, 0, backendData->localPlayer.clientID, 0, nullptr);
		}
		played_card_data.card = (CARD_ID)resp.card();
		data_mutex.unlock();
	}
	else if (packet->header.type == UNO_MESSAGES::UNO_PICK_COLOR_REQUEST & (backendData->localPlayer.groupMask & ADMIN_GROUP_MASK))
	{


		if (!game.isChoosingColor) SyncSinglePlayer(packet->header.senderID);
		else
		{
			data_mutex.lock();
			Uno::ChooseColorRequest req;
			req.ParseFromArray(packet->body.data(), packet->body.size());
			uint32_t colorID = req.colorid();

			data_mutex.unlock();
		}

	}
	else if (packet->header.type == UNO_MESSAGES::UNO_PICK_COLOR_RESPONSE && (packet->header.additionalData & AdditionalDataFlags::ADDITIONAL_DATA_FLAG_ADMIN))
	{
		data_mutex.lock();



		data_mutex.unlock();
	}
}
void UnoPlugin::FetchSyncData(std::string& str) // NOT IN MAIN THREAD
{
	while(!g_objs || !g_objs->anims.inputsAllowed) { }
	data_mutex.lock();
	Uno::GameState state;

	COLOR_ID topCardColorID = COLOR_INVALID;
	CARD_ID topCard = g_objs->stack.GetTop(topCardColorID);
	state.set_topcard((uint32_t)topCard); state.set_topcardcolorid((uint32_t)topCardColorID);
	if (0 <= game.playerInTurn && game.players.size() < game.playerInTurn)
	{
		const std::string& cur = game.players.at(game.playerInTurn).name;
		state.set_playerinturn(cur);
	}
	else
	{
		state.set_playerinturn("");
	}
	for (int i = 0; i < game.players.size(); i++)
	{
		const std::string& name = game.players.at(i).name;
		const CardHand& hand = *game.players.at(i).hand;
		const uint32_t id = game.players.at(i).id;
		Uno::Player* players = state.add_players();
		players->set_name(name);
		players->set_id(id);
		for (const CardInfo& card : hand.cards)
		{
			players->add_cardsonhand(card.front);
		}
	}
	str = state.SerializeAsString();
	data_mutex.unlock();
}
void UnoPlugin::HandleAddClient(const ClientData* added)
{
	AddPlayer(added);
}
void UnoPlugin::HandleRemovedClient(const ClientData* removed)
{
	RemovePlayer(removed);
}
void UnoPlugin::HandleSync(const std::string& syncData)
{
	Uno::GameState state;
	state.ParseFromString(syncData);
	game.hands->clear();
	game.players.clear();
	const std::string& inTurn = state.playerinturn();

	AddPlayer(&backendData->localPlayer);
	game.playerInTurn = -1;
	for (const auto& player : state.players())
	{
		bool found = false;
		const std::string name = player.name();
		for (const auto& client : backendData->players)
		{
			if (client.name == name)
			{
				int handIdx = AddPlayer(&client);
				if (handIdx < game.hands->size() && handIdx >= 0)
				{
					CardHand& hand = game.hands->at(handIdx);
					for (uint32_t card : player.cardsonhand())
					{
						hand.Add((CARD_ID)card);
					}

					if (name == inTurn)
					{
						game.playerInTurn = handIdx;
					}

				}
				found = true;
				break;
			}
		}
		if (!found)
		{
			if (name == backendData->localPlayer.name)
			{
				CardHand& hand = game.hands->at(0);
				for (uint32_t card : player.cardsonhand())
				{
					hand.Add((CARD_ID)card);
				}
				if (name == inTurn)
				{
					game.playerInTurn = 0;
				}
			}
		}
	}

	uint32_t topCard = state.topcard();
	uint32_t topColorID = state.topcardcolorid();
	if (topCard < CARD_ID::NUM_AVAILABLE_CARDS)
	{
		if (topCard != -1) { g_objs->stack.SetTop((CARD_ID)topCard, (COLOR_ID)topColorID); }
	}
}






void GameUpdateFunction(UnoGlobals* g_objs, float dt)
{
	static int curPulledPlayer = 0;

	for (int i = 0; i < pulled_card_list.size(); i++)
	{
		int actualIdx = (curPulledPlayer + i + 1) % pulled_card_list.size();
		if (!pulled_card_list.at(actualIdx).empty())
		{
			curPulledPlayer = actualIdx;
			CARD_ID card = pulled_card_list.at(actualIdx).at(pulled_card_list.at(actualIdx).size() - 1);
			int idx = g_objs->hands.at(actualIdx).AddTemp(g_objs->playerCam, card);
			CardHand& h = g_objs->hands.at(actualIdx);
			g_objs->anims.AddAnim(g_objs->stack, h.cards.at(idx), h.handID, CARD_ANIMATIONS::ANIM_FETCH_CARD);
			break;
		}
	}
	if (played_card_data.playerIdx  < game.players.size())
	{
		auto& hand = g_objs->hands.at(played_card_data.playerIdx);
		int idx = -1;
		for (int i = 0; i < hand.cards.size(); i++)
		{
			if (hand.cards.at(i).front == played_card_data.card)
			{
				idx = i;
				break;
			}
		}
		if (idx == -1)
		{
			idx = hand.cards.size();
			if (idx == 0)
			{
				hand.Add(played_card_data.card);
			}
			else
			{
				hand.cards.at(0).front = played_card_data.card;
				idx = 0;
			}
		}
		g_objs->anims.AddAnim(g_objs->stack, hand.cards.at(idx), hand.handID, CARD_ANIMATIONS::ANIM_PLAY_CARD);
		hand.cards.erase(hand.cards.begin() + idx);
		played_card_data.playerIdx = -1;
		played_card_data.card = (CARD_ID)-1;

	}

	if (game.playerInTurn == game.players.at(g_objs->localPlayerIndex).id)
	{
		CardHand* hand = game.players.at(g_objs->localPlayerIndex).hand;
		hand->Update(g_objs->stack, g_objs->anims, g_objs->picker, g_objs->playerCam, g_objs->moveComp.mouseRay, g_objs->p, g_objs->anims.inputsAllowed);

		if (game.isChoosingColor) {
			g_objs->picker.Draw((float)g_objs->playerCam.screenX / (float)g_objs->playerCam.screenY, dt);
		}
	}
	else
	{
		Pointer p;
		memset(&p, 0, sizeof(Pointer));
		p.dx = g_objs->p.dx;
		p.dy = g_objs->p.dy;
		p.x = g_objs->p.x;
		p.y = g_objs->p.y;
		g_objs->hands.at(0).Update(g_objs->stack, g_objs->anims, g_objs->picker, g_objs->playerCam, g_objs->moveComp.mouseRay, p, g_objs->anims.list.empty());
	}
}
















#define ALLOW_FREEMOVEMENT
SceneDirLight* light = nullptr;
void UnoPlugin::Init(ApplicationData* data)
{
	instance = this;
	initialized = true;
	this->backendData = data;

	InitializeOpenGL(data->assetManager);
	InitializeCardPipeline(data->assetManager);
	ImGui::SetCurrentContext(data->imGuiCtx);


	GLint tex;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &tex);
	LOG("MAX_TEXTURE_SIZE: %d\n", tex);


	g_objs = new UnoGlobals;
	g_objs->moveComp.pos = { 0.0f, 2.6f, 3.0f };
	g_objs->moveComp.SetRotation(-90.0f, -50.0f, 0.0f);

	game.hands = &g_objs->hands;

	g_objs->rendererData.Create(10, 10, 10, 10, 0, true, true);
	g_objs->reflectFBO = CreateSingleFBO(1200, 800);

	g_objs->lightDir = { -1.0f / sqrtf(3.0f), -1.0f / sqrt(3.0f), -1.0f / sqrt(3.0f) };

	g_objs->shadowCam.pos = { 2.0f, 4.0f, 2.0f };
	g_objs->shadowCam.view = glm::lookAtLH(g_objs->shadowCam.pos, g_objs->shadowCam.pos - g_objs->lightDir, glm::vec3(0.0f, 1.0f, 0.0f));
	g_objs->shadowCam.proj = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, -10.0f, 10.0f);
	g_objs->shadowCam.viewProj = g_objs->shadowCam.proj * g_objs->shadowCam.view;


	{
		glGenBuffers(1, &g_objs->shadowCam.uniform);
		glBindBuffer(GL_UNIFORM_BUFFER, g_objs->shadowCam.uniform);

		CameraData camData;
		camData.camPos = g_objs->shadowCam.pos;
		camData.projection = g_objs->shadowCam.proj;
		camData.view = g_objs->shadowCam.view;

		glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraData), &camData, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
	{
		glGenBuffers(1, &g_objs->playerCam.uniform);
		glBindBuffer(GL_UNIFORM_BUFFER, g_objs->playerCam.uniform);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraData), nullptr, GL_DYNAMIC_DRAW);
	}
	void* pbrModel = CreateInternalPBRFromFile("Assets/Helmet.gltf", 1.0f);
	//void* pbrModel = CreateInternalPBRFromFile("C:/Users/deder/OneDrive/Desktop/3DModels/glTF-Sample-Models-master/2.0/Sponza/glTF/Sponza.gltf", 1.0f);


	// CREATE SCENE
	{
		g_objs->UnoScene = CreateAndInitializeSceneAsDefault();
		AddCardTypeToScene(g_objs->UnoScene);

		g_objs->cardRenderObject = CreateCardBatchSceneObject(g_objs->UnoScene);

		glm::vec3 reflectPos = { 0.0f, 0.0f, 0.0f };
		glm::vec3 normal = { 0.0f, 1.0f, 0.0f };
		ReflectiveSurfaceMaterialData data{ };
		data.distortionFactor = 0.0f;
		data.moveFactor = 0.0f;
		data.tintColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

		ReflectiveSurfaceTextures texs;
		texs.reflect = g_objs->reflectFBO.texture;
		texs.refract = g_objs->refGroundTexture;
		texs.dudv = 0;
		g_objs->offscreenX = 1200;
		g_objs->offscreenY = 800;
		g_objs->basePlatform = AddReflectiveSurface(g_objs->UnoScene, &reflectPos, &normal, 80.0f, 80.0f, &data, &texs);


		light = SC_AddDirectionalLight(g_objs->UnoScene);
		light->data.ambient = { 0.2f, 0.2f, 0.2f };
		light->data.diffuse = { 1.1f, 1.1f, 1.1f };
		light->data.dir = { -1.0f / sqrtf(3.0f), -1.0f / sqrt(3.0f), -1.0f / sqrt(3.0f) };
		light->data.specular = { 0.8f, 0.8f, 0.8f };
		light->data.hasShadow = true;
		light->data.mapper[0].start = { 0.0f, 0.0f };
		light->data.mapper[0].end = { 1.0f, 1.0f };
		light->data.mapper[0].viewProj = g_objs->shadowCam.viewProj;
		light->data.numCascades = 1;


		UBOParams params = UBOParams();
		PBRSceneObject* o = AddPbrModelToScene(g_objs->UnoScene, pbrModel, params, glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.7f, -2.0f)));
		o->base.flags &= ~(SCENE_OBJECT_BLEND);
	}
	g_objs->skybox = LoadCubemap(
		"Assets/CitySkybox/right.jpg",
		"Assets/CitySkybox/left.jpg",
		"Assets/CitySkybox/top.jpg",
		"Assets/CitySkybox/bottom.jpg",
		"Assets/CitySkybox/front.jpg",
		"Assets/CitySkybox/back.jpg");


	GLuint colTex;
	glGenTextures(1, &colTex);
	glBindTexture(GL_TEXTURE_2D, colTex);
	uint32_t cols[4] = {
		0x000000FF, 0x0000FF00,
		0x0000FFFF, 0x00FF0000
	};
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, cols);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	g_objs->refGroundTexture = colTex;
	ReflectiveSurfaceTextures texs;
	texs.reflect = g_objs->reflectFBO.texture;
	texs.refract = colTex;
	texs.dudv = 0;
	ReflectiveSurfaceSetTextureData(g_objs->basePlatform, &texs);
	
	AddPlayer(&this->backendData->localPlayer);
	g_objs->hands.at(0).Add(CARD_ID::CARD_ID_ADD_4);
	game.playerInTurn = g_objs->hands.at(0).handID;
	for (const auto& p : backendData->players)
	{
		int idx = AddPlayer(&p);
		for (int i = 0; i < 4; i++)
		{
			game.hands->at(idx).Add(CARD_ID::CARD_ID_BLUE_0);
		}
	}
	
	if(backendData->localPlayer.groupMask & ADMIN_GROUP_MASK)
	{
		Base::SyncResponse resp;
		std::string state;
		FetchSyncData(state);
		resp.set_state(state);
		SendNetworkData((uint32_t)PacketID::FORCE_SYNC, LISTEN_GROUP_ALL, ADDITIONAL_DATA_FLAG_ADMIN, backendData->localPlayer.clientID, resp.SerializeAsString());
	}

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glDepthMask(GL_TRUE);
	
#ifndef ANDROID
	glEnable(GL_MULTISAMPLE);
#endif
}

void UnoPlugin::Resize(ApplicationData* data)
{
	if(sizeY && sizeX)
		g_objs->playerCam.SetPerspective(90.0f, (float)this->sizeX / (float)this->sizeY, 0.1f, 300.0f);
	g_objs->playerCam.screenX = sizeX;
	g_objs->playerCam.screenY = sizeY;
	GLint defaultFBO;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultFBO);
	SetScreenFramebuffer(defaultFBO, { sizeX, sizeY });


	g_objs->rendererData.Recreate(sizeX, sizeY, SHADOW_TEXTURE_SIZE, SHADOW_TEXTURE_SIZE);
	g_objs->rendererData.MakeMainFramebuffer();
}
static ColorPicker picker;
void UnoPlugin::Render(ApplicationData* data)
{
	if (!(sizeX && sizeY)) return;

	static auto prev = std::chrono::high_resolution_clock::now();
	auto now = std::chrono::high_resolution_clock::now();

	float dt = std::chrono::duration<float>(now - prev).count();
	prev = now;

	g_objs->moveComp.Update();
	g_objs->playerCam.Update(&g_objs->moveComp);


	auto& ray = g_objs->moveComp.mouseRay;
	ray = g_objs->playerCam.ScreenToWorld(g_objs->p.x, g_objs->p.y);

	data_mutex.lock();
	if (game.state == STATE_PENDING && (backendData->localPlayer.groupMask & ADMIN_GROUP_MASK))
	{
		g_objs->cardHandlingTimer += dt;
		if (g_objs->cardHandlingTimer > 2.0f)
		{
			
		}
	}
	else if (game.state == STATE_PLAYING)
	{
		g_objs->cardHandlingTimer = 0.0f;
		GameUpdateFunction(g_objs, dt);
		{ // render all cards
			ClearCards();
			g_objs->deck.Draw();
			g_objs->stack.Draw();
			g_objs->anims.Update(g_objs->hands, g_objs->stack, dt);

			for (int i = 0; i < g_objs->hands.size(); i++)
			{
				g_objs->hands.at(i).Draw(g_objs->playerCam);
			}

		}
	}
	data_mutex.unlock();

	BeginScene(g_objs->UnoScene);

	StandardRenderPassData stdData;
	stdData.skyBox = g_objs->skybox;
	stdData.shadowMap = 0;
	stdData.ambientOcclusionMap = 0;

	glm::vec4 plane = { 0.0f, 1.0f, 0.0f, 0.0f };


	
	glBindFramebuffer(GL_FRAMEBUFFER, g_objs->rendererData.shadowFBO.fbo);
	glClearDepthf(1.0f);
	glClear(GL_DEPTH_BUFFER_BIT);


	RenderSceneCascadeShadow(g_objs->UnoScene, &g_objs->rendererData, &g_objs->playerCam, &g_objs->shadowCam, &light->data, { 0.0f, 0.0f }, { 1.0f, 1.0f }, 0.8f);
	
	stdData.shadowMap = g_objs->rendererData.shadowFBO.depth;

	glBindFramebuffer(GL_FRAMEBUFFER, g_objs->reflectFBO.fbo);
	glViewport(0, 0, g_objs->offscreenX, g_objs->offscreenY);
	glClearColor(1.0f, 0.4f, 0.4f, 1.0f);
	glClearDepthf(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	stdData.renderSize = { g_objs->offscreenX, g_objs->offscreenY };

	Camera reflected = Camera::GetReflected(&g_objs->playerCam, plane);
	{
		glBindBuffer(GL_UNIFORM_BUFFER, g_objs->playerCam.uniform);
		CameraData camData;
		camData.camPos = reflected.pos;
		camData.view = reflected.view;
		camData.projection = reflected.perspective;
		glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraData), &camData, GL_DYNAMIC_DRAW);
	}
	stdData.cameraUniform = g_objs->playerCam.uniform;
	stdData.camView = &reflected.view;
	stdData.camProj = &reflected.perspective;
	stdData.camPos = &reflected.pos;
	ReflectPlanePassData reflectData;
	reflectData.base = &stdData;
	reflectData.planeEquation = &plane;

	RenderSceneReflectedOnPlane(g_objs->UnoScene, &reflectData);
	{
		glBindBuffer(GL_UNIFORM_BUFFER, g_objs->playerCam.uniform);
		CameraData camData;
		camData.camPos = g_objs->playerCam.pos;
		camData.view = g_objs->playerCam.view;
		camData.projection = g_objs->playerCam.perspective;
		
		glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraData), &camData, GL_DYNAMIC_DRAW);
	}
	stdData.camView = &g_objs->playerCam.view;
	stdData.camProj = &g_objs->playerCam.perspective;
	stdData.camPos = &g_objs->playerCam.pos;
	
	glm::ivec2 mainSize = GetMainFramebufferSize();
	glBindFramebuffer(GL_FRAMEBUFFER, GetMainFramebuffer());
	glViewport(0, 0, mainSize.x, mainSize.y);
	glClearColor(1.0f, 0.4f, 0.4f, 1.0f);
	glClearDepthf(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	stdData.renderSize = { mainSize.x, mainSize.y };

	RenderAmbientOcclusion(g_objs->UnoScene, &stdData, &g_objs->rendererData, sizeX, sizeY);
	glBindFramebuffer(GL_FRAMEBUFFER, GetMainFramebuffer());
	glViewport(0, 0, mainSize.x, mainSize.y);
	RenderSceneStandard(g_objs->UnoScene, &stdData);



	RenderPostProcessing(&g_objs->rendererData, GetScreenFramebuffer(), sizeX, sizeY);

	DrawUI();
	
	
	
	EndScene();

	g_objs->ms.FrameEnd();
	g_objs->p.EndFrame();
	EndFrameAndResetData();

	
}



void UnoPlugin::MouseCallback(const PB_MouseData* mData)
{
#ifdef ALLOW_FREEMOVEMENT
	if (mData->lDown && (mData->dx || mData->dy))
	{
		g_objs->moveComp.UpdateFromMouseMovement(-mData->dx, mData->dy);
	}
#endif
	g_objs->ms.SetFromPBState(mData);
	g_objs->p.FillFromMouse(mData);
}
void UnoPlugin::KeyDownCallback(Key k, bool isRepeat)
{
#ifdef ALLOW_FREEMOVEMENT
	if (!isRepeat)
	{
		if (k == Key::Key_W)g_objs->moveComp.SetMovementDirection(MovementComponent::DIRECTION::FORWARD, true);
		if (k == Key::Key_A)g_objs->moveComp.SetMovementDirection(MovementComponent::DIRECTION::LEFT, true);
		if (k == Key::Key_S)g_objs->moveComp.SetMovementDirection(MovementComponent::DIRECTION::BACKWARD, true);
		if (k == Key::Key_D)g_objs->moveComp.SetMovementDirection(MovementComponent::DIRECTION::RIGHT, true);
	}
#endif
	if (k == Key::Key_0) {
		for (auto& h : g_objs->hands)
		{
			h.FetchCard(g_objs->playerCam, g_objs->stack, g_objs->deck, g_objs->anims);
		}
	}
}
void UnoPlugin::KeyUpCallback(Key k, bool isRepeat)
{
#ifdef ALLOW_FREEMOVEMENT
	if (!isRepeat)
	{
		if (k == Key::Key_W)g_objs->moveComp.SetMovementDirection(MovementComponent::DIRECTION::FORWARD, false);
		if (k == Key::Key_A)g_objs->moveComp.SetMovementDirection(MovementComponent::DIRECTION::LEFT, false);
		if (k == Key::Key_S)g_objs->moveComp.SetMovementDirection(MovementComponent::DIRECTION::BACKWARD, false);
		if (k == Key::Key_D)g_objs->moveComp.SetMovementDirection(MovementComponent::DIRECTION::RIGHT, false);

		if (k == Key::Key_P) // PRINT DEBUG GAME INFO
		{
			LOG("<<<<<<<<<<<<<<<<GAME_INFO<<<<<<<<<<<<<<<<\n");
			for (int i = 0; i < game.hands->size(); i++)
			{
				if (game.players.size() <= i)
				{
					LOG("[ERROR]: THIS SHOULD NEVER EVERY HAPPEN!!!\n");
				}
				else
				{
					const PlayerInfo* info = &game.players.at(i);
					if (i == g_objs->localPlayerIndex) LOG("LOCAL_PLAYER:\n");
					LOG("PLAYER: %s\n", info->name.c_str());
					for (int j = 0; j < info->hand->cards.size(); j++)
					{
						LOG("CARD(%d): %d\n", j, info->hand->cards.at(j).front);
					}
				}
			}
			LOG("CURRENT_PLAYER_IN_TURN: %d\n", game.playerInTurn);
		}
	}
#endif
}

void UnoPlugin::TouchDownCallback(int x, int y, int touchID)
{
#ifdef ALLOW_FREEMOVEMENT
	g_objs->moveComp.UpdateTouch(g_objs->playerCam.screenX,x, y, touchID, false);
#endif
	g_objs->p.OnTouchDown(x, y, touchID);
}
void UnoPlugin::TouchUpCallback(int x, int y, int touchID)
{
#ifdef ALLOW_FREEMOVEMENT
	g_objs->moveComp.UpdateTouch(g_objs->playerCam.screenX, x, y, touchID, true);
#endif
	g_objs->p.OnTouchUp(x, y, touchID);
}
void UnoPlugin::TouchMoveCallback(int x, int y, int dx, int dy, int touchID)
{
#ifdef ALLOW_FREEMOVEMENT
	g_objs->moveComp.UpdateTouchMove(x, y, dx, dy, touchID);
#endif
	g_objs->p.OnTouchMove(x, y, dx, dy, touchID);
}
void UnoPlugin::CleanUp()
{
	glDeleteTextures(1, &g_objs->skybox);
	CleanUpOpenGL();
	DestroySingleFBO(&g_objs->reflectFBO);
	
	delete g_objs;
}



