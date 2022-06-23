#include "Uno.h"
#include <string>
#include "Graphics/Helper.h"
#include "logging.h" // REMINDER USE THIS !! USE THIS NOT <iostream> !!
#include "Graphics/PbrRendering.h"
#include "Graphics/UiRendering.h"
#include <chrono>

#define _USE_MATH_DEFINES
#include <math.h>

PLUGIN_EXPORT_DEFINITION(UnoPlugin, "a3fV-6giK-10Eb-2rdT");

PLUGIN_INFO UnoPlugin::GetPluginInfos()
{
	PLUGIN_INFO plugInfo;
	memcpy(plugInfo.ID, _Plugin_Export_ID_Value, strnlen_s(_Plugin_Export_ID_Value, 19));
	plugInfo.previewResource = nullptr;
	return plugInfo;
}



struct UnoGlobals
{
	Camera playerCam;
	GLuint uboUniform;
	GLuint uboParamsUniform;
	GLuint skybox;
	void* gltfModel;
}g_objs;



void UpdateUBOBuf()
{
	glBindBuffer(GL_UNIFORM_BUFFER, g_objs.uboUniform);
	UBO* data = (UBO*)glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	if (data)
	{
		data->projection = g_objs.playerCam.perspective;
		data->model = glm::rotate(glm::mat4(1.0f), -(float)M_PI/2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
		data->view = g_objs.playerCam.view;
		data->camPos = g_objs.playerCam.pos;
	}
	else
	{
		LOG("FAILED TO MAP BUFFER\n");
	}
	glUnmapBuffer(GL_UNIFORM_BUFFER);
}


void UnoPlugin::Init(void* backendData)
{
	initialized = true;
	InitializeOpenGL();
	glEnable(GL_DEPTH_TEST);
	// glEnable(GL_BLEND);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glGenBuffers(1, &g_objs.uboUniform);
	glBindBuffer(GL_UNIFORM_BUFFER, g_objs.uboUniform);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(UBO), nullptr, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &g_objs.uboParamsUniform);
	glBindBuffer(GL_UNIFORM_BUFFER, g_objs.uboParamsUniform);
	UBOParams params;
	params.lightDir = { 0.0f, -1.0f, 0.0f, 0.0f };
	params.exposure = 4.5f;
	params.gamma = 2.2f;
	params.prefilteredCubeMipLevels = 1.0f;
	params.scaleIBLAmbient = 1.0f;
	params.debugViewInputs = 0.0f;
	params.debugViewEquation = 0.0f;
	glBufferData(GL_UNIFORM_BUFFER, sizeof(UBOParams), &params, GL_STATIC_DRAW);

	g_objs.playerCam.pos = { -1.5f, 0.8f, 0.0f };



	g_objs.skybox = LoadCubemap({
		"Assets/TestCubemap/right.jpg",
		"Assets/TestCubemap/left.jpg",
		"Assets/TestCubemap/top.jpg",
		"Assets/TestCubemap/bottom.jpg",
		"Assets/TestCubemap/front.jpg",
		"Assets/TestCubemap/back.jpg" });
	
	//g_objs.gltfModel = CreateInternalPBRFromFile("Assets/Helmet.gltf", 1.0f);
	g_objs.gltfModel = CreateInternalPBRFromFile("Assets/BoxAnimated.glb", 1.0f);
}

void UnoPlugin::Resize(void* backendData)
{
	if(sizeY && sizeX)
		g_objs.playerCam.SetPerspective(90.0f, (float)this->sizeX / (float)this->sizeY, 0.1f, 256.0f);
}
void UnoPlugin::Render(void* backendData)
{
	if (!(sizeX && sizeY)) return;

	static auto prev = std::chrono::high_resolution_clock::now();
	auto now = std::chrono::high_resolution_clock::now();

	float dt = std::chrono::duration<float>(now - prev).count();
	prev = now;

	UpdateAnimation(g_objs.gltfModel, 0, dt);

	glViewport(0, 0, framebufferX, framebufferY);
	glClearColor(0.0f, 0.4f, 0.4f, 1.0f);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDepthFunc(GL_LESS);
	g_objs.playerCam.Update();
	UpdateUBOBuf();



	if(g_objs.gltfModel)
		DrawPBRModel(g_objs.gltfModel, g_objs.uboUniform, g_objs.uboParamsUniform, g_objs.skybox);

	

	

	DrawSkybox(g_objs.skybox, g_objs.playerCam.view, g_objs.playerCam.perspective);
}



void UnoPlugin::MouseCallback(const PB_MouseData* mData)
{
	if (mData->lPressed && (mData->dx || mData->dy))
	{
		g_objs.playerCam.UpdateFromMouseMovement(-mData->dx, mData->dy);
	}
}
void UnoPlugin::KeyDownCallback(Key k, bool isRepeat)
{
	if (!isRepeat)
	{
		if (k == Key::Key_W)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::FORWARD, true);
		if (k == Key::Key_A)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::LEFT, true);
		if (k == Key::Key_S)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::BACKWARD, true);
		if (k == Key::Key_D)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::RIGHT, true);
	}
}
void UnoPlugin::KeyUpCallback(Key k, bool isRepeat)
{
	if (!isRepeat)
	{
		if (k == Key::Key_W)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::FORWARD, false);
		if (k == Key::Key_A)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::LEFT, false);
		if (k == Key::Key_S)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::BACKWARD, false);
		if (k == Key::Key_D)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::RIGHT, false);
	}
}