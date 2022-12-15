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
	if (instance->g_objs->localPlayerIndex < game.players.size())
	{
		const float localPlayerRot = game.players.at(instance->g_objs->localPlayerIndex).hand->rotation;
		for (int i = 0; i < game.hands->size(); i++)
		{
			game.hands->at(i).rotation -= localPlayerRot;
		}
		game.players.at(instance->g_objs->localPlayerIndex).hand->willBeSorted = true;	// only sort local player cards
	}
	return pos;
}
static void RemovePlayer(const ClientData* removed)
{
	for (int i = 0; i < game.players.size(); i++)
	{
		if (game.players.at(i).id == removed->clientID) {
			game.players.erase(game.players.begin() + i);
			if (i < instance->g_objs->localPlayerIndex)
			{
				instance->g_objs->localPlayerIndex--;
			}
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



static std::vector<std::vector<CARD_ID>> pulled_card_list;
struct PlayedCardData
{
	uint32_t playerIdx = -1;
	CARD_ID  card = (CARD_ID)-1;
}played_card_data;

static void PullCardsAction(const char* data, size_t size)
{
	pulled_card_list.resize(game.players.size());
	
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
			pulled_card_list.at(actualIdx).pop_back();
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

	if (g_objs->localPlayerIndex < game.players.size())
	{
		if (game.playerInTurn == g_objs->localPlayerIndex && g_objs->localPlayerIndex < game.players.size())
		{
			CardHand* hand = game.players.at(g_objs->localPlayerIndex).hand;
			hand->choosingCardColor = game.isChoosingColor;
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
			game.players.at(g_objs->localPlayerIndex).hand->Update(g_objs->stack, g_objs->anims, g_objs->picker, g_objs->playerCam, g_objs->moveComp.mouseRay, p, g_objs->anims.list.empty());
		}
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


	if (game.state == STATE_PENDING)
	{
		
	}
	else if (game.state == STATE_PLAYING)
	{
		g_objs->cardHandlingTimer = 0.0f;
		GameUpdateFunction(g_objs, dt);
	}

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
			COLOR_ID topColor = COLOR_RED;
			LOG("TOP_MOST_CARD: %d\n", g_objs->stack.GetTop(topColor));
			LOG("TOP_MOST_COLOR: %d\n", topColor);
			LOG("IS_CHOOSING_COLOR: %d\n", game.isChoosingColor);
			LOG("LOCAL_PLAYER_INDEX: %d\n", g_objs->localPlayerIndex);
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



