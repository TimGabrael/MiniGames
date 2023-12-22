#include "Uno.h"
#include <string>
#include "Graphics/GLCompat.h"
#include "Graphics/Helper.h"
#include "Graphics/Camera.h"
#include "Graphics/Scene.h"
#include "Plugins/Client/Uno/Card.h"
#include "Plugins/Client/Uno/NetHandlers.h"
#include "Plugins/Shared/Uno/UnoBase.h"
#include "logging.h" // REMINDER USE THIS !! USE THIS NOT <iostream> !!
#include "Graphics/PbrRendering.h"
#include "Graphics/UiRendering.h"
#include <chrono>
#include <algorithm>
#include "Graphics/Renderer.h"
#include "Graphics/ReflectiveSurfaceRendering.h"
#include "Graphics/BloomRendering.h"
#include "imgui.h"
#include "Graphics/shader.h"

#define _USE_MATH_DEFINES
#include <math.h>



PLUGIN_EXPORT_DEFINITION(UnoPlugin, UNO_PLUGIN_ID, "Assets/Uno.png");
#define SHADOW_TEXTURE_SIZE 2048 * 4




void PrintGLMMatrix(const glm::mat4& m)
{
	LOG("%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n",
		m[0][0], m[1][0], m[2][0], m[3][0],
		m[0][1], m[1][1], m[2][1], m[3][1],
		m[0][2], m[1][2], m[2][2], m[3][2],
		m[0][3], m[1][3], m[2][3], m[3][3]);
}

static UnoPlugin* instance = nullptr;

UnoPlugin* GetInstance()
{
	return instance;
}
GameStateData* GetGameState()
{
	return &instance->g_objs->game;
}


PlayerInfo* GameStateData::GetPlayerInfoForcefully(uint16_t id)
{
	UnoPlugin* uno = GetInstance();
	PlayerInfo* p = GetPlayerInfo(id);
	if (p) return p;

	PlayerInfo adding;
	adding.hand = nullptr;
	adding.id = id;

	if (uno->backendData->localPlayer.clientID == id)
	{
		adding.name = uno->backendData->localPlayer.name;
	}
	else
	{
		for (int i = 0; i < uno->backendData->players.size(); i++)
		{
			if (uno->backendData->players.at(i).clientID == id) {
				adding.name = uno->backendData->players.at(i).name;
				break;
			}
		}
	}

	players.push_back(adding);
	std::sort(players.begin(), players.end(), [](const PlayerInfo& a, const PlayerInfo& b) { return a.id > b.id; });

	return GetPlayerInfoForcefully(id);
}
CardHand* GameStateData::GetHandForcefully(uint16_t id)
{
	UnoPlugin* uno = GetInstance();
	CardHand* h = GetHand(id);
	if (h) return h;
	hands->emplace_back(id);
	for (int i = 0; i < players.size(); i++)
	{
		for (int j = 0; j < hands->size(); j++)
		{
			if (players.at(i).id == hands->at(j).handID)
			{
				players.at(i).hand = &hands->at(j);
				break;
			}
		}
	}
	
	int frontIdx = 0;
	for (int i = 0; i < players.size(); i++)
	{
		if (players.at(i).id == uno->backendData->localPlayer.clientID)
		{
			frontIdx = i;
			break;
		}
	}

	float dA = 360.0f / (float)players.size();
	for (int i = 0; i < players.size(); i++)
	{
		int testIdx = (frontIdx + i) % players.size();
		players.at(testIdx).hand->rotation = dA * i;
	}

	return GetHandForcefully(id);
}
PlayerInfo* GameStateData::GetLocalPlayer()
{
	UnoPlugin* uno = GetInstance();
	for (int i = 0; i < players.size(); i++)
	{
		if (players.at(i).id == uno->backendData->localPlayer.clientID) return &players.at(i);
	}
	return nullptr;
}
CardHand* GameStateData::GetLocalPlayerHand()
{
	UnoPlugin* uno = GetInstance();
	for (int i = 0; i < hands->size(); i++)
	{
		if (hands->at(i).handID == uno->backendData->localPlayer.clientID) return &hands->at(i);
	}
	return nullptr;
}





void GameUpdateFunction(UnoGlobals* g_objs, float dt)
{
	GameStateData& game = *GetGameState();
	CardHand* localPlayer = game.GetLocalPlayerHand();

	if (localPlayer)
	{
		if (game.playerInTurn == localPlayer->handID)
		{
			localPlayer->choosingCardColor = game.isChoosingColor;
			localPlayer->Update(g_objs->stack, g_objs->anims, g_objs->picker, g_objs->playerCam, g_objs->moveComp.mouseRay, g_objs->p, g_objs->anims.inputsAllowed);

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
			localPlayer->Update(g_objs->stack, g_objs->anims, g_objs->picker, g_objs->playerCam, g_objs->moveComp.mouseRay, p, g_objs->anims.list.empty());
		}
	}
}












//#define ALLOW_FREEMOVEMENT
SceneDirLight* light = nullptr;
void UnoPlugin::Init(ApplicationData* data)
{
	instance = this;
	initialized = true;
	this->backendData = data;
	
    UnoFillNetHandlers(data);

    

	InitializeOpenGL(data->assetManager);
	InitializeCardPipeline(data->assetManager);
	ImGui::SetCurrentContext(data->imGuiCtx);

    Shader s;
    SH_Create(&s);



	GLint maxTexSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
	LOG("MAX_TEXTURE_SIZE: %d\n", maxTexSize);


	g_objs = new UnoGlobals;
	g_objs->moveComp.pos = { 0.0f, 2.6f, 3.0f };
	g_objs->moveComp.SetRotation(-90.0f, -50.0f, 0.0f);
    g_objs->largeFont = ImGui::GetIO().Fonts->AddFontFromFileTTF("Assets/consola.ttf", 32.0f);
    g_objs->smallFont = ImGui::GetIO().Fonts->AddFontFromFileTTF("Assets/consola.ttf", 16.0f);
    
	GameStateData& game = *GetGameState();

	game.hands = &g_objs->hands;

	g_objs->rendererData.Create(10, 10, 10, 10, 0, false, false);
    g_objs->rendererData.MakeMainFramebuffer();
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


	// CREATE SCENE
	{
		g_objs->UnoScene = SC_CreateScene();

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
	}
    
    g_objs->environment = LoadEnvironmentMaps("Assets/CitySkybox/environment.env");



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
void UnoPlugin::Render(ApplicationData* data)
{
	if (!(sizeX && sizeY) || !data->isRunning) return;
	
	GameStateData& game = *GetGameState();
	
	static auto prev = std::chrono::high_resolution_clock::now();
	auto now = std::chrono::high_resolution_clock::now();
	
	float dt = std::chrono::duration<float>(now - prev).count();
	prev = now;
	
	g_objs->moveComp.Update();
	g_objs->playerCam.Update(&g_objs->moveComp);
	
	
	auto& ray = g_objs->moveComp.mouseRay;
	ray = g_objs->playerCam.ScreenToWorld((float)g_objs->p.x, (float)g_objs->p.y);
	
	
	g_objs->cardHandlingTimer = 0.0f;
	GameUpdateFunction(g_objs, dt);
	

	// RENDER
	{
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

        SetOpenGLDepthWrite(true);
		glBindFramebuffer(GL_FRAMEBUFFER, g_objs->rendererData.shadowFBO.fbo);
        glViewport(0, 0, g_objs->rendererData.shadowWidth, g_objs->rendererData.shadowHeight);
		glClearDepthf(1.0f);
		glClear(GL_DEPTH_BUFFER_BIT);

		RenderSceneCascadeShadow(g_objs->UnoScene, &g_objs->rendererData, &g_objs->playerCam, &g_objs->shadowCam, &light->data, { 0.0f, 0.0f }, { 1.0f, 1.0f }, 0.8f);
        
        StandardRenderPassData stdData;
		stdData.env = &g_objs->environment;
		stdData.ambientOcclusionMap = 0;
        glm::vec4 plane = { 0.0f, 1.0f, 0.0f, 0.0f };


        SetOpenGLDepthWrite(true);
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
        stdData.drawShadow = false;
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
        stdData.shadowMap = g_objs->rendererData.shadowFBO.depth;
		stdData.camView = &g_objs->playerCam.view;
		stdData.camProj = &g_objs->playerCam.perspective;
		stdData.camPos = &g_objs->playerCam.pos;

        glEnable(GL_DEPTH_TEST);
		glm::ivec2 mainSize = GetMainFramebufferSize();
		glBindFramebuffer(GL_FRAMEBUFFER, GetMainFramebuffer());
		glViewport(0, 0, mainSize.x, mainSize.y);
		glClearColor(1.0f, 0.4f, 0.4f, 1.0f);
		glClearDepthf(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		stdData.renderSize = { mainSize.x, mainSize.y };

		RenderAmbientOcclusion(g_objs->UnoScene, &stdData, &g_objs->rendererData, (float)this->sizeX, (float)this->sizeY);
		glBindFramebuffer(GL_FRAMEBUFFER, GetMainFramebuffer());
		glViewport(0, 0, mainSize.x, mainSize.y);
		RenderSceneStandard(g_objs->UnoScene, &stdData);

		RenderPostProcessing(&g_objs->rendererData, GetScreenFramebuffer(), sizeX, sizeY);

		DrawUI();
        
        if(!game.finished) {
            ImGui::Begin("_pull_button", nullptr, ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_::ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
            ImGui::SetWindowSize(ImVec2(400, 400));
            if(game.playerInTurn == backendData->localPlayer.clientID) {
                ImGui::Text("It's your turn!");
                bool pressed = ImGui::Button("Pull-Cards", ImVec2(200, 100));
                if (g_objs->anims.inputsAllowed && pressed) {
                    backendData->net->SendData(Client_UnoPullCards, nullptr, SendFlags::Send_Reliable);
                }
                pressed = ImGui::Button("Finish", ImVec2(200, 100));
                if(g_objs->anims.inputsAllowed && pressed) {
                    uno::ClientPlayCard skip;
                    skip.mutable_card()->set_face(CARD_UNKNOWN);
                    skip.mutable_card()->set_color(CARD_COLOR_UNKOWN);
                    backendData->net->SendData(Client_UnoPlayCard, &skip, SendFlags::Send_Reliable);
                }
                pressed = ImGui::Button("QUIT", ImVec2(200, 100));
                if(pressed) {
                    data->isRunning = false;
                }
            }
            else {
                bool pressed = ImGui::Button("QUIT", ImVec2(200, 100));
                if (pressed) {
                    data->isRunning = false;
                }
            }
            ImGui::End();
        }
        else {
            ImGui::Begin("_statistics", nullptr, ImGuiWindowFlags_::ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove);
            const float statistics_w = (float)this->sizeX * 2.0f / 3.0f;
            const float statistics_h = (float)this->sizeY * 2.0f / 3.0f;
            const float statistics_sx = ((float)this->sizeX - statistics_w) * 0.5f;
            const float statistics_sy = ((float)this->sizeY - statistics_h) * 0.5f;
            ImGui::SetWindowSize(ImVec2(statistics_w, statistics_h));
            ImGui::SetWindowPos(ImVec2(statistics_sx, statistics_sy));
            ImVec2 headerSize = ImGui::CalcTextSize("Statistics");
            ImGui::SetCursorPosX((statistics_w - headerSize.x) / 2.0f);
            ImGui::Text("Statistics");
            //ImGui::PushFont(g_objs->smallFont);
            if(game.winnerQueue.size() < UINT16_MAX) { // this Should always be the case, but just in case ...
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
                for(uint16_t i = 0; i != (uint16_t)game.winnerQueue.size(); i++) {
                    PlayerInfo* cur = game.GetPlayerInfo(i);
                    if(cur) {
                        char text_buffer[500] = {};
                        int text_len = sprintf_s(text_buffer, "%d %s", i + 1, cur->name.c_str());
                        ImVec2 textSize = ImGui::CalcTextSize(text_buffer, text_buffer + text_len);
                        ImGui::SetCursorPosX((statistics_w - textSize.x) / 2.0f);
                        ImGui::Text("%s", text_buffer);
                    }
                }
                ImGui::PopStyleColor();
            }
            //ImGui::PopFont();

            if(backendData->localPlayer.isAdmin) {
                ImGui::SetCursorPosY(statistics_h - 110);
                bool pressed = ImGui::Button("RESTART", ImVec2(200, 100));
                if (pressed) {
                    backendData->net->SendData(Client_UnoAdminRestart, nullptr, SendFlags::Send_Reliable);
                }
            }
            ImGui::SetCursorPosX(statistics_w - 210);
            ImGui::SetCursorPosY(statistics_h - 110);
            bool pressed = ImGui::Button("QUIT", ImVec2(200, 100));
            if (pressed) {
                data->isRunning = false;
            }
            ImGui::End();
        }

		EndScene();

		g_objs->ms.FrameEnd();
		g_objs->p.EndFrame();
		EndFrameAndResetData();
	}
}


void UnoPlugin::MouseCallback(const PB_MouseData* mData)
{
#ifdef ALLOW_FREEMOVEMENT
	if (mData->lDown && (mData->dx || mData->dy))
	{
		g_objs->moveComp.UpdateFromMouseMovement((float)-mData->dx, (float)mData->dy);
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
			h.PullCardServer(g_objs->playerCam, g_objs->stack, g_objs->deck, g_objs->anims, CARD_ID::CARD_ID_BLUE_2);
		}
	}
}
void UnoPlugin::KeyUpCallback(Key k, bool isRepeat)
{
#ifdef ALLOW_FREEMOVEMENT
    GameStateData& game = *GetGameState();
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
		}
	}
#endif
    if(k == Key::Key_Escape)  {
    }
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
    delete g_objs->basePlatform;
    FreeCardBatchSceneObject(g_objs->UnoScene, g_objs->cardRenderObject);
    g_objs->cardRenderObject = nullptr;

    UnloadEnvironmentMaps(&g_objs->environment);
    
	DestroySingleFBO(&g_objs->reflectFBO);
    SC_DestroyScene(g_objs->UnoScene);
    
    glDeleteBuffers(1, &g_objs->shadowCam.uniform);
    glDeleteBuffers(1, &g_objs->playerCam.uniform);
    g_objs->rendererData.CleanUp();

    UnInitializeCardPipeline();	
	CleanUpOpenGL();
	delete g_objs;
}



