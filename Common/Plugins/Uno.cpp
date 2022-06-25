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
#ifdef _WIN32
	memcpy(plugInfo.ID, _Plugin_Export_ID_Value, strnlen_s(_Plugin_Export_ID_Value, 19));
#else
	memcpy(plugInfo.ID, _Plugin_Export_ID_Value, strnlen(_Plugin_Export_ID_Value, 19));
#endif
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


void PrintGLMMatrix(const glm::mat4& m)
{
	LOG("%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n",
		m[0][0], m[1][0], m[2][0], m[3][0],
		m[0][1], m[1][1], m[2][1], m[3][1],
		m[0][2], m[1][2], m[2][2], m[3][2],
		m[0][3], m[1][3], m[2][3], m[3][3]);
}


void UpdateUBOBuf()
{
	glBindBuffer(GL_UNIFORM_BUFFER, g_objs.uboUniform);
	UBO* data = (UBO*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(UBO), GL_MAP_WRITE_BIT);
	if (data)
	{
		data->projection = g_objs.playerCam.perspective;
		data->model = glm::mat4(1.0f);
		data->view = g_objs.playerCam.view;
		data->camPos = g_objs.playerCam.pos;
		glUnmapBuffer(GL_UNIFORM_BUFFER);
	}
	else
	{
		LOG("FAILED TO MAP BUFFER, buf: %d\n", g_objs.uboUniform);
	}
}

void UnoPlugin::Init(void* backendData)
{
	initialized = true;
	this->backendData = backendData;
	InitializeOpenGL(backendData);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);

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

	g_objs.playerCam.pos = { 0.0f, 0.4f, 2.0f };
	g_objs.gltfModel = nullptr;


	g_objs.skybox = LoadCubemap(
		"Assets/CitySkybox/right.jpg",
		"Assets/CitySkybox/left.jpg",
		"Assets/CitySkybox/top.jpg",
		"Assets/CitySkybox/bottom.jpg",
		"Assets/CitySkybox/front.jpg",
		"Assets/CitySkybox/back.jpg");
	
	g_objs.gltfModel = CreateInternalPBRFromFile("Assets/Helmet.gltf", 1.0f);
	//g_objs.gltfModel = CreateInternalPBRFromFile("Assets/BoxAnimated.glb", 1.0f);


	glDisable(GL_CULL_FACE);

	if (sizeY != 0) {	// for android, as resize is never called
		g_objs.playerCam.SetPerspective(90.0f, (float)this->sizeX / (float)this->sizeY, 0.1f, 256.0f);
	}
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

	//UpdateAnimation(g_objs.gltfModel, 0, dt);
	glViewport(0, 0, framebufferX, framebufferY);
	glClearColor(1.0f, 0.4f, 0.4f, 1.0f);
	glClearDepthf(1.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDepthFunc(GL_LESS);
	g_objs.playerCam.Update();
	UpdateUBOBuf();



	if(g_objs.gltfModel)
		DrawPBRModel(g_objs.gltfModel, g_objs.uboUniform, g_objs.uboParamsUniform, g_objs.skybox, true);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if(g_objs.gltfModel)
		DrawPBRModel(g_objs.gltfModel, g_objs.uboUniform, g_objs.uboParamsUniform, g_objs.skybox, false);
	

	glDisable(GL_BLEND);
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

		if (k == Key::Key_1)g_objs.playerCam.pos.x += 0.1f;
		if (k == Key::Key_2)g_objs.playerCam.pos.y += 0.1f;
		if (k == Key::Key_3)g_objs.playerCam.pos.z += 0.1f;

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