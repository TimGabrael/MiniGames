#include "Uno.h"
#include <string>
#include "Graphics/Helper.h"
#include "Graphics/Camera.h"
#include "logging.h" // REMINDER USE THIS !! USE THIS NOT <iostream> !!
#include "Graphics/PbrRendering.h"
#include "Graphics/UiRendering.h"
#include <chrono>
#include <algorithm>
#include "Card.h"
#include "Graphics/Simple3DRendering.h"
#include "../InputStates.h"
#include "Animator.h"
#include "Pointer.h"

#define _USE_MATH_DEFINES
#include <math.h>

PLUGIN_EXPORT_DEFINITION(UnoPlugin, "a3fV-6giK-10Eb-2rdT");

PLUGIN_INFO UnoPlugin::GetPluginInfos()
{
	PLUGIN_INFO plugInfo;
#ifdef _WIN32
	memcpy(plugInfo.ID, _Plugin_Export_ID_Value, strnlen_s(_Plugin_Export_ID_Value, 19));
#else
	memcpy(plugInfo.ID, _Plugin_Export_ID_Value, strnlen(_Plugin_Export_ID_Value, 19));
#endif
	plugInfo.previewResource = nullptr;
	return plugInfo;
}

void PrintGLMMatrix(const glm::mat4& m)
{
	LOG("%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n",
		m[0][0], m[1][0], m[2][0], m[3][0],
		m[0][1], m[1][1], m[2][1], m[3][1],
		m[0][2], m[1][2], m[2][2], m[3][2],
		m[0][3], m[1][3], m[2][3], m[3][3]);
}



struct UnoGlobals
{
	Camera playerCam;
	GLuint skybox;
	S3DCombinedBuffer platform;
	CardHand* client;
	std::vector<CardHand> hands;
	MouseState ms;
	Pointer p;
	CardStack stack;
	CardsInAnimation anims;
	CardDeck deck;
	ColorPicker picker;

}g_objs;










//#define ALLOW_FREEMOVEMENT
void UnoPlugin::Init(ApplicationData* data)
{
	initialized = true;
	this->backendData = data;
	InitializeOpenGL(data->assetManager);
	InitializeCardPipeline(data->assetManager);

	static constexpr float cubeSize = 4.0f;
	SVertex3D platformVertices[4] = {
		{{-cubeSize, 0.0f, -cubeSize}, {0.0f, 0.0f}, 0xFF000090},
		{{ cubeSize, 0.0f, -cubeSize}, {0.0f, 0.0f}, 0xFF900000},
		{{ cubeSize, 0.0f,  cubeSize}, {0.0f, 0.0f}, 0xFF009000},
		{{-cubeSize, 0.0f,  cubeSize}, {0.0f, 0.0f}, 0xFF009090},
	};
	uint32_t platformIndices[] = {
		0,3,2,2,1,0,
	};
	g_objs.platform = S3DGenerateBuffer(platformVertices, sizeof(platformVertices)/sizeof(SVertex3D), platformIndices, sizeof(platformIndices)/sizeof(uint32_t));

	g_objs.playerCam.pos = { 0.0f, 1.6f, 2.0f };
	g_objs.playerCam.SetRotation(-90.0f, -40.0f, 0.0f);


	g_objs.skybox = LoadCubemap(
		"Assets/CitySkybox/right.jpg",
		"Assets/CitySkybox/left.jpg",
		"Assets/CitySkybox/top.jpg",
		"Assets/CitySkybox/bottom.jpg",
		"Assets/CitySkybox/front.jpg",
		"Assets/CitySkybox/back.jpg");

	g_objs.hands.emplace_back(-1);
	g_objs.client = &g_objs.hands.at(0);
	g_objs.client->Add(CARD_ID::CARD_ID_ADD_4);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);


#ifndef ANDROID
	glEnable(GL_MULTISAMPLE);
#endif
}

void UnoPlugin::Resize(ApplicationData* data)
{
	if(sizeY && sizeX)
		g_objs.playerCam.SetPerspective(90.0f, (float)this->sizeX / (float)this->sizeY, 0.1f, 256.0f);
	g_objs.playerCam.screenX = sizeX;
	g_objs.playerCam.screenY = sizeY;
}
ColorPicker picker;
void UnoPlugin::Render(ApplicationData* data)
{
	if (!(sizeX && sizeY)) return;
	
	static auto prev = std::chrono::high_resolution_clock::now();
	auto now = std::chrono::high_resolution_clock::now();

	float dt = std::chrono::duration<float>(now - prev).count();
	prev = now;

	glViewport(0, 0, framebufferX, framebufferY);
	glClearColor(1.0f, 0.4f, 0.4f, 1.0f);
	glClearDepthf(1.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDepthFunc(GL_LESS);
	g_objs.playerCam.Update();


	auto& ray = g_objs.playerCam.mouseRay;
	ray = g_objs.playerCam.ScreenToWorld(g_objs.p.x, g_objs.p.y);



	g_objs.client->Update(g_objs.stack, g_objs.anims, g_objs.picker, g_objs.playerCam, g_objs.p, g_objs.anims.list.empty());

	if (g_objs.client->choosingCardColor) {
		g_objs.picker.Draw((float)g_objs.playerCam.screenX / (float)g_objs.playerCam.screenY, dt);
	}


	{ // render all cards
		ClearCards();
		g_objs.deck.Draw();
		g_objs.stack.Draw();
		g_objs.anims.Update(g_objs.hands, g_objs.stack, dt);
		
		g_objs.client->Draw(g_objs.playerCam);
	}
	glDisable(GL_BLEND);

	DrawSimple3D(g_objs.platform, g_objs.playerCam.perspective, g_objs.playerCam.view);

	DrawSkybox(g_objs.skybox, g_objs.playerCam.view, g_objs.playerCam.perspective);



	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);



	DrawCards(g_objs.playerCam.perspective, g_objs.playerCam.view);
	DrawUI();

	glEnable(GL_DEPTH_TEST);

	g_objs.ms.FrameEnd();
	g_objs.p.EndFrame();
}



void UnoPlugin::MouseCallback(const PB_MouseData* mData)
{
#ifdef ALLOW_FREEMOVEMENT
	if (mData->lDown && (mData->dx || mData->dy))
	{
		g_objs.playerCam.UpdateFromMouseMovement(-mData->dx, mData->dy);
	}
#endif
	g_objs.ms.SetFromPBState(mData);
	// auto& ray = g_objs.playerCam.mouseRay;
	// ray = g_objs.playerCam.ScreenToWorld(mData->xPos, mData->yPos);
	
	g_objs.p.FillFromMouse(mData);
	

}
void UnoPlugin::KeyDownCallback(Key k, bool isRepeat)
{
#ifdef ALLOW_FREEMOVEMENT
	if (!isRepeat)
	{
		if (k == Key::Key_W)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::FORWARD, true);
		if (k == Key::Key_A)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::LEFT, true);
		if (k == Key::Key_S)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::BACKWARD, true);
		if (k == Key::Key_D)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::RIGHT, true);
	}
#endif
	if (k == Key::Key_0) g_objs.client->FetchCard(g_objs.playerCam, g_objs.stack, g_objs.deck, g_objs.anims);
}
void UnoPlugin::KeyUpCallback(Key k, bool isRepeat)
{
#ifdef ALLOW_FREEMOVEMENT
	if (!isRepeat)
	{
		if (k == Key::Key_W)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::FORWARD, false);
		if (k == Key::Key_A)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::LEFT, false);
		if (k == Key::Key_S)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::BACKWARD, false);
		if (k == Key::Key_D)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::RIGHT, false);
	}
#endif
}

void UnoPlugin::TouchDownCallback(int x, int y, int touchID)
{
#ifdef ALLOW_FREEMOVEMENT
	g_objs.playerCam.UpdateTouch(x, y, touchID, false);
#endif
	g_objs.p.OnTouchDown(x, y, touchID);
}
void UnoPlugin::TouchUpCallback(int x, int y, int touchID)
{
#ifdef ALLOW_FREEMOVEMENT
	g_objs.playerCam.UpdateTouch(x, y, touchID, true);
#endif
	g_objs.p.OnTouchUp(x, y, touchID);
}
void UnoPlugin::TouchMoveCallback(int x, int y, int dx, int dy, int touchID)
{
#ifdef ALLOW_FREEMOVEMENT
	g_objs.playerCam.UpdateTouchMove(x, y, dx, dy, touchID);
#endif
	g_objs.p.OnTouchMove(x, y, dx, dy, touchID);
}
void UnoPlugin::CleanUp()
{
	glDeleteTextures(1, &g_objs.skybox);
	CleanUpOpenGL();


}