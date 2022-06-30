#include "Uno.h"
#include <string>
#include "Graphics/Helper.h"
#include "logging.h" // REMINDER USE THIS !! USE THIS NOT <iostream> !!
#include "Graphics/PbrRendering.h"
#include "Graphics/UiRendering.h"
#include <chrono>
#include "Card.h"
#include "Graphics/Simple3DRendering.h"



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
	S3DCombinedBuffer platform;
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
	UBO ubos;
	ubos.camPos = g_objs.playerCam.pos;
	ubos.view = g_objs.playerCam.view;
	ubos.model = glm::mat4(1.0f);
	ubos.projection = g_objs.playerCam.perspective;

	glBufferData(GL_UNIFORM_BUFFER, sizeof(UBO), &ubos, GL_DYNAMIC_DRAW);
}




#define ALLOW_FREEMOVEMENT
void UnoPlugin::Init(void* backendData, PLATFORM_ID id)
{
	initialized = true;
	this->backendData = backendData;
	InitializeOpenGL(backendData);
	InitializeCardPipeline(backendData);

	static constexpr float cubeSize = 2.0f;
	static constexpr float cubeHeight = 1.0f;
	SVertex3D cubeVertices[4] = {
		{{-cubeSize, 0.0f, -cubeSize}, {0.0f, 0.0f}, 0xFF402040},
		{{ cubeSize, 0.0f, -cubeSize}, {0.0f, 0.0f}, 0xFF406040},
		{{ cubeSize, 0.0f,  cubeSize}, {0.0f, 0.0f}, 0xFF402040},
		{{-cubeSize, 0.0f,  cubeSize}, {0.0f, 0.0f}, 0xFF406040},
	};
	uint32_t cubeIndices[] = {
		0,3,2,2,1,0,
	};
	g_objs.platform = S3DGenerateBuffer(cubeVertices, sizeof(cubeVertices)/sizeof(SVertex3D), cubeIndices, sizeof(cubeIndices)/sizeof(uint32_t));



	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);

	glGenBuffers(1, &g_objs.uboUniform);
	glBindBuffer(GL_UNIFORM_BUFFER, g_objs.uboUniform);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(UBO), nullptr, GL_DYNAMIC_DRAW);
	glGenBuffers(1, &g_objs.uboParamsUniform);
	glBindBuffer(GL_UNIFORM_BUFFER, g_objs.uboParamsUniform);
	UBOParams params;
	params.lightDir = { 0.0f, -1.0f, 0.0f, 1.0f };
	params.exposure = 4.5f;
	params.gamma = 2.2f;
	params.prefilteredCubeMipLevels = 1.0f;
	params.scaleIBLAmbient = 1.0f;
	params.debugViewInputs = 0.0f;
	params.debugViewEquation = 0.0f;
	glBufferData(GL_UNIFORM_BUFFER, sizeof(UBOParams), &params, GL_STATIC_DRAW);

	g_objs.playerCam.pos = { 0.0f, 1.4f, 2.0f };
	g_objs.playerCam.SetRotation(0.0f, -0.5f, 0.0f);
	g_objs.gltfModel = nullptr;


	g_objs.skybox = LoadCubemap(
		"Assets/CitySkybox/right.jpg",
		"Assets/CitySkybox/left.jpg",
		"Assets/CitySkybox/top.jpg",
		"Assets/CitySkybox/bottom.jpg",
		"Assets/CitySkybox/front.jpg",
		"Assets/CitySkybox/back.jpg");
	
	//g_objs.gltfModel = CreateInternalPBRFromFile("Assets/Helmet.gltf", 1.0f);
	g_objs.gltfModel = CreateInternalPBRFromFile("Assets/BoxAnimated.glb", 1.0f);

	
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	AddCard(CARD_ID::CARD_ID_BLANK, CARD_ID::CARD_ID_YELLOW_0, glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
	AddCard(CARD_ID::CARD_ID_BLANK, CARD_ID::CARD_ID_YELLOW_0, glm::translate(glm::mat4(1.0f), glm::vec3(1.1f, 0.0f, 0.01f)));
	AddCard(CARD_ID::CARD_ID_BLANK, CARD_ID::CARD_ID_YELLOW_0, glm::translate(glm::mat4(1.0f), glm::vec3(1.2f, 0.0f, 0.02f)));
	AddCard(CARD_ID::CARD_ID_BLANK, CARD_ID::CARD_ID_YELLOW_0, glm::translate(glm::mat4(1.0f), glm::vec3(1.3f, 0.0f, 0.03f)));
	AddCard(CARD_ID::CARD_ID_BLANK, CARD_ID::CARD_ID_YELLOW_0, glm::translate(glm::mat4(1.0f), glm::vec3(1.4f, 0.0f, 0.04f)));
}

void UnoPlugin::Resize(void* backendData)
{
	if(sizeY && sizeX)
		g_objs.playerCam.SetPerspective(90.0f, (float)this->sizeX / (float)this->sizeY, 0.1f, 256.0f);
	g_objs.playerCam.screenX = sizeX;
	g_objs.playerCam.screenY = sizeY;
}
void UnoPlugin::Render(void* backendData)
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
	UpdateUBOBuf();




	glDisable(GL_BLEND);

	DrawSimple3D(g_objs.platform, g_objs.playerCam.perspective, g_objs.playerCam.view);

	if (g_objs.gltfModel)
	{
		UpdateAnimation(g_objs.gltfModel, 0, dt);
		DrawPBRModel(g_objs.gltfModel, g_objs.uboUniform, g_objs.uboParamsUniform, g_objs.skybox, true);
	}
	DrawSkybox(g_objs.skybox, g_objs.playerCam.view, g_objs.playerCam.perspective);



	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);


	if(g_objs.gltfModel)
		DrawPBRModel(g_objs.gltfModel, g_objs.uboUniform, g_objs.uboParamsUniform, g_objs.skybox, false);



	DrawCards(g_objs.playerCam.perspective, g_objs.playerCam.view);
	DrawUI();

	glEnable(GL_DEPTH_TEST);
}



void UnoPlugin::MouseCallback(const PB_MouseData* mData)
{
#ifdef ALLOW_FREEMOVEMENT
	if (mData->lPressed && (mData->dx || mData->dy))
	{
		g_objs.playerCam.UpdateFromMouseMovement(-mData->dx, mData->dy);
	}
#endif
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
}
void UnoPlugin::TouchUpCallback(int x, int y, int touchID)
{
#ifdef ALLOW_FREEMOVEMENT
	g_objs.playerCam.UpdateTouch(x, y, touchID, true);
#endif
}
void UnoPlugin::TouchMoveCallback(int x, int y, int dx, int dy, int touchID)
{
#ifdef ALLOW_FREEMOVEMENT
	g_objs.playerCam.UpdateTouchMove(x, y, dx, dy, touchID);
#endif
}
void UnoPlugin::CleanUp()
{
	glDeleteTextures(1, &g_objs.skybox);
	glDeleteBuffers(1, &g_objs.uboParamsUniform);
	glDeleteBuffers(1, &g_objs.uboUniform);

	if (g_objs.gltfModel)
	{
		CleanUpInternal(g_objs.gltfModel);
		g_objs.gltfModel = nullptr;
	}

	CleanUpOpenGL();


	// ADD GLTF CLEANUP!!!
}