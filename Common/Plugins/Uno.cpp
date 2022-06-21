#include "Uno.h"
#include <string>
#include "Graphics/Helper.h"
#include "logging.h" // REMINDER USE THIS !! USE THIS NOT <iostream> !!
#include "Graphics/PbrRendering.h"

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
	UBO uniformData;
	GLuint skybox;
	void* gltfModel;
}g_objs;






void UnoPlugin::Init(void* backendData)
{
	initialized = true;
	InitializeOpenGL();
	glEnable(GL_DEPTH_TEST);

	g_objs.playerCam.UpdateViewMatrix();


	g_objs.skybox = LoadCubemap({
		"Assets/TestCubemap/right.jpg",
		"Assets/TestCubemap/left.jpg",
		"Assets/TestCubemap/top.jpg",
		"Assets/TestCubemap/bottom.jpg",
		"Assets/TestCubemap/front.jpg",
		"Assets/TestCubemap/back.jpg" });

	InitializePbrPipeline();
	// g_objs.gltfModel = CreateInternalPBRFromFile("Assets/test.gltf", 10.0f);
}

void UnoPlugin::Resize(void* backendData)
{
	g_objs.playerCam.SetPerspective(90.0f, (float)this->sizeX / (float)this->sizeY, 0.1f, 256.0f);

}
void UnoPlugin::Render(void* backendData)
{
	glViewport(0, 0, framebufferX, framebufferY);
	glClearColor(0.0f, 0.4f, 0.4f, 1.0f);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



	//if(g_objs.gltfModel)
	//	DrawPBRModel(g_objs.gltfModel, g_objs.uniform, g_objs.environmentMap);



	DrawSkybox(g_objs.skybox, g_objs.playerCam.view, g_objs.playerCam.perspective);
}



void UnoPlugin::MouseCallback(const PB_MouseData* mData)
{
	if (mData->dx || mData->dy)
	{
		g_objs.playerCam.UpdateFromMouseMovement(mData->dx, mData->dy);
	}
}