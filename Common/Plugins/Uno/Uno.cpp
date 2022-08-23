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

UnoPlugin* GetInstance()
{
	return instance;
}
void SendNetworkData(uint32_t packetID, uint32_t group, uint16_t additionalFlags, uint16_t clientID, size_t size, const void* data)
{
	instance->backendData->_sendDataFunction(packetID, group, additionalFlags, clientID, size, data);
}
void SendNetworkData(uint32_t packetID, uint32_t group, uint16_t additionalFlags, uint16_t clientID, const std::string& str)
{
	instance->backendData->_sendDataFunction(packetID, group, additionalFlags, clientID, str.size(), str.data());
}




struct GameStateData
{
	std::vector<std::string> playerNames;
	std::vector<CardHand>* hands;
	int playerInTurn;
}game;

static int AddPlayer(const ClientData* added)
{
	int pos = game.hands->size();
	game.hands->push_back(added->clientID);
	game.playerNames.push_back(added->name);
	const size_t num = game.playerNames.size();
	for (size_t i = 0; i < num; i++)
	{
		game.hands->at(i).rotation = i * 360.0f / (float)num;
		game.hands->at(i).willBeSorted = false;
	}
	game.hands->at(0).willBeSorted = true;	// only sort local player cards
	return pos;
}
static void RemovePlayer(const ClientData* removed)
{
	for (int i = 0; i < game.playerNames.size(); i++)
	{
		if (game.playerNames.at(i) == removed->name) {
			game.playerNames.erase(game.playerNames.begin() + i);
			game.hands->erase(game.hands->begin() + i);
			break;
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
void UnoPlugin::NetworkCallback(Packet* packet) // NOT IN MAIN THREAD
{
	if (packet->header.type == UNO_MESSAGES::UNO_PULL_CARD_REQUEST && (backendData->localPlayer.groupMask & ADMIN_GROUP_MASK))
	{
		if (game.playerInTurn >= 0 && game.playerInTurn < game.playerNames.size() && packet->header.senderID == game.hands->at(game.playerInTurn).handID)
		{
			CARD_ID pulled = g_objs->deck.PullCard();
			Uno::PullCardResponse resp;
			auto* pull = resp.add_pullresponses();
			pull->add_cards(pulled);
			pull->set_player(game.playerNames.at(game.playerInTurn));
			SendNetworkData(UNO_MESSAGES::UNO_PULL_CARD_RESPONSE, LISTEN_GROUP_ALL, AdditionalDataFlags::ADDITIONAL_DATA_FLAG_ADMIN, backendData->localPlayer.clientID, resp.SerializeAsString());
		}
	}
	else if (packet->header.type == UNO_MESSAGES::UNO_PULL_CARD_RESPONSE && packet->header.additionalData & AdditionalDataFlags::ADDITIONAL_DATA_FLAG_ADMIN)
	{
		Uno::PullCardResponse resp;
		resp.ParseFromArray(packet->body.data(), packet->body.size());
		data_mutex.lock();
		for (const auto& pull : resp.pullresponses())
		{
			const std::string& name = pull.player();
			int playerIdx = 0;
			for (uint32_t i = 0; i < game.playerNames.size(); i++)
			{
				if (name == game.playerNames.at(i))
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
		if (game.playerInTurn >= 0 && game.playerInTurn < game.playerNames.size() && packet->header.senderID == game.hands->at(game.playerInTurn).handID)
		{
			Uno::PlayCardRequest req;
			req.ParseFromArray(packet->body.data(), packet->body.size());

			CARD_ID topCardColorID;
			CARD_ID topCard = g_objs->stack.GetTop(topCardColorID);
			if (CardIsPlayable(topCard, (CARD_ID)req.card(), topCardColorID))
			{
				Uno::PlayCard resp;
				resp.set_player(game.playerNames.at(game.playerInTurn));
				resp.set_card(req.card());
				game.playerInTurn = (game.playerInTurn + 1) % game.playerNames.size();
				resp.set_nextplayer(game.playerNames.at(game.playerInTurn));

				data_mutex.lock();
				played_card_data.playerIdx = game.playerInTurn;
				played_card_data.card = (CARD_ID)req.card();
				data_mutex.unlock();

				SendNetworkData(UNO_MESSAGES::UNO_PLAY_CARD_RESPONSE, LISTEN_GROUP_ALL, AdditionalDataFlags::ADDITIONAL_DATA_FLAG_ADMIN, backendData->localPlayer.clientID, resp.SerializeAsString());
			}
		}
	}
	else if (packet->header.type == UNO_MESSAGES::UNO_PLAY_CARD_RESPONSE && (packet->header.additionalData & AdditionalDataFlags::ADDITIONAL_DATA_FLAG_ADMIN))
	{
		Uno::PlayCard resp;
		resp.ParseFromArray(packet->body.data(), packet->body.size());
		const std::string& name = resp.player();
		const std::string& nextName = resp.nextplayer();
		data_mutex.lock();
		bool curFound = false;
		bool nextFound = false;
		for (int i = 0; i < game.playerNames.size(); i++)
		{
			if (!curFound && game.playerNames.at(i) == name)
			{
				played_card_data.playerIdx = i;
				played_card_data.card = (CARD_ID)resp.card();
				curFound = true;
			}
			if (!nextFound && game.playerNames.at(i) == nextName)
			{
				game.playerInTurn = i;
				nextFound = true;
			}
			if (curFound && nextFound)
				break;
		}
		if (!curFound || !nextFound) SendNetworkData((uint32_t)PacketID::SYNC_REQUEST, ADMIN_GROUP_MASK, 0, backendData->localPlayer.clientID, "");
		played_card_data.card = (CARD_ID)resp.card();
		data_mutex.unlock();
	}
	
}
void UnoPlugin::FetchSyncData(std::string& str) // NOT IN MAIN THREAD
{
	while (!g_objs->anims.inputsAllowed) { }
	Uno::GameState state;
	CARD_ID topCardColorID;
	CARD_ID topCard = g_objs->stack.GetTop(topCardColorID);
	state.set_topcard(topCard); state.set_topcardcolorid(topCardColorID);
	if (0 <= game.playerInTurn && game.playerNames.size() < game.playerInTurn)
	{
		const std::string& cur = game.playerNames.at(game.playerInTurn);
		state.set_playerinturn(cur);
	}
	else
	{
		state.set_playerinturn("");
	}
	for (int i = 0; i < game.playerNames.size(); i++)
	{
		const std::string& name = game.playerNames.at(i);
		const CardHand& hand = game.hands->at(i);

		Uno::Player* players = state.add_players();
		players->set_name(name);
		for (const CardInfo& card : hand.cards)
		{
			players->add_cardsonhand(card.front);
		}
	}
	str = state.SerializeAsString();
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
	game.playerNames.clear();
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
		g_objs->stack.SetTop((CARD_ID)topCard, (CARD_ID)topColorID);
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
	if (played_card_data.playerIdx  < game.playerNames.size())
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
			hand.Add(played_card_data.card);
		}
		g_objs->anims.AddAnim(g_objs->stack, hand.cards.at(idx), hand.handID, CARD_ANIMATIONS::ANIM_PLAY_CARD);
		played_card_data.playerIdx = -1;
		played_card_data.card = (CARD_ID)-1;

	}

	if (game.playerInTurn == g_objs->hands.at(0).handID)
	{
		g_objs->hands.at(0).Update(g_objs->stack, g_objs->anims, g_objs->picker, g_objs->playerCam, g_objs->moveComp.mouseRay, g_objs->p, g_objs->anims.list.empty());

		if (g_objs->hands.at(0).choosingCardColor) {
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

		//ScenePointLight* plight = SC_AddPointLight(g_objs->UnoScene);
		//plight->data.ambient = { 0.2f, 0.2f, 0.2f };
		//plight->data.diffuse = { 10.0f, 0.0f, 0.0f };
		//plight->data.pos = { 0, 4.0f, 0.0f };
		//plight->data.specular = { 0.8f, 0.8f, 0.8f };
		//plight->data.constant = 1.0f;
		//plight->data.linear = 0.1f;
		//plight->data.quadratic = 0.1f;
		//plight->data.hasShadow = false;
		//
		//plight = SC_AddPointLight(g_objs->UnoScene);
		//plight->data.ambient = { 0.2f, 0.2f, 0.2f };
		//plight->data.diffuse = { 0.0f, 10.0f, 0.0f };
		//plight->data.pos = { -4.0, 4.0f, -4.0f };
		//plight->data.specular = { 0.8f, 0.8f, 0.8f };
		//plight->data.constant = 1.0f;
		//plight->data.linear = 0.1f;
		//plight->data.quadratic = 0.1f;
		//plight->data.hasShadow = false;
		//
		//plight = SC_AddPointLight(g_objs->UnoScene);
		//plight->data.ambient = { 0.2f, 0.2f, 0.2f };
		//plight->data.diffuse = { 0.0f, 0.0f, 10.0f };
		//plight->data.pos = { 4.0, 4.0f, -4.0f };
		//plight->data.specular = { 0.8f, 0.8f, 0.8f };
		//plight->data.constant = 1.0f;
		//plight->data.linear = 0.1f;
		//plight->data.quadratic = 0.1f;
		//plight->data.hasShadow = false;


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
		for (int i = 0; i < 100; i++)
		{
			game.hands->at(idx).Add(CARD_ID::CARD_ID_BLUE_0);
		}
	}
	
	if (!(backendData->localPlayer.groupMask & ADMIN_GROUP_MASK))
	{
		SendNetworkData((uint32_t)PacketID::SYNC_REQUEST, ADMIN_GROUP_MASK, 0, backendData->localPlayer.clientID, 0, nullptr);
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
	
	//g_objs->offscreenX = sizeX;
	//g_objs->offscreenY = sizeY;
	//RecreateSingleFBO(&g_objs->reflectFBO, sizeX, sizeY);
	//ReflectiveSurfaceTextures texs;
	//texs.reflect = g_objs->reflectFBO.texture;
	//texs.refract = g_objs->refGroundTexture;
	//texs.dudv = 0;
	//ReflectiveSurfaceSetTextureData(g_objs->basePlatform, &texs);


	g_objs->rendererData.Recreate(sizeX, sizeY, SHADOW_TEXTURE_SIZE, SHADOW_TEXTURE_SIZE);
	g_objs->rendererData.MakeMainFramebuffer();
}
static ColorPicker picker;
static bool overrideAOMap = false;
static bool drawAOMap = false;
static bool camToShadowView = false;
static bool stopUpdateShadow = false;
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
	GameUpdateFunction(g_objs, dt);
	data_mutex.unlock();

	{ // render all cards
		ClearCards();
		g_objs->deck.Draw();
		g_objs->stack.Draw();
		g_objs->anims.Update(g_objs->hands, g_objs->stack, dt);
	
		for (auto& c : g_objs->hands)
		{
			c.Draw(g_objs->playerCam);
		}
		
	}

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
		if (camToShadowView)
		{
			camData.camPos = g_objs->shadowCam.pos;
			camData.view = g_objs->shadowCam.view;
			camData.projection = g_objs->shadowCam.proj;
		}
		else
		{
			camData.camPos = g_objs->playerCam.pos;
			camData.view = g_objs->playerCam.view;
			camData.projection = g_objs->playerCam.perspective;
		}
		glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraData), &camData, GL_DYNAMIC_DRAW);
	}
	if (camToShadowView)
	{
		stdData.camView = &g_objs->shadowCam.view;
		stdData.camProj = &g_objs->shadowCam.proj;
		stdData.camPos = &g_objs->shadowCam.pos;
	}
	else
	{
		stdData.camView = &g_objs->playerCam.view;
		stdData.camProj = &g_objs->playerCam.perspective;
		stdData.camPos = &g_objs->playerCam.pos;
	}
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
	if (overrideAOMap) stdData.ambientOcclusionMap = 0;
	RenderSceneStandard(g_objs->UnoScene, &stdData);

	//if (drawAOMap) DrawQuad({ -1.0f, -1.0f }, { 1.0f, 1.0f }, 0xFFFFFFFF, g_objs->rendererData.aoFBO.texture);
	if (drawAOMap) DrawQuad({ -1.0f, -1.0f }, { 1.0f, 1.0f }, 0xFFFFFFFF, g_objs->rendererData.shadowFBO.depth);
	//if (drawAOMap) DrawQuad({ -1.0f, -1.0f }, { 1.0f, 1.0f }, 0xFFFFFFFF, g_objs->rendererData.bloomFBO.bloomTexture1);


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
		if (k == Key::Key_E)overrideAOMap = !overrideAOMap;
		if (k == Key::Key_R)drawAOMap = !drawAOMap;
		if (k == Key::Key_T)camToShadowView = !camToShadowView;
		if (k == Key::Key_Z)stopUpdateShadow = !stopUpdateShadow;
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



