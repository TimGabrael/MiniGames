#pragma once
#include "Scene.h"
#include "Camera.h"
#include "Helper.h"




struct CameraData
{
	glm::mat4 projection;
	glm::mat4 view;
	glm::vec3 camPos;
};

struct CurrentLightInformation
{
	ScenePointLight curPointLights[MAX_NUM_LIGHTS];
	SceneDirLight curDirLights[MAX_NUM_LIGHTS];
	int numCurPointLights = 0;
	int numCurDirLights = 0;
	glm::vec4 reflectionPlane = { 0, 0, 0, 0 };
	bool isReflected = false;
};


struct StandardRenderPassData
{
	const glm::mat4* camView;
	const glm::mat4* camProj;
	const glm::vec3* camPos;
	const glm::mat4* camViewProj;
	GLuint cameraUniform;
	GLuint skyBox;
	GLuint shadowMap;
	GLuint lightData;
};
struct ReflectPlanePassData
{
	const StandardRenderPassData* base;
	const glm::vec4* planeEquation;
};

void InitializeRendererBackendData();
void CleanUpRendererBackendData();

GLuint GetWhiteTexture2D();
GLuint GetBlackTexture2D();


struct PostProcessingFBO
{
	GLuint fbo;
	GLuint texture;
	GLuint maybeDepth;	// might not be initialized, depending on the state of the SceneRenderData
};
struct SceneRenderData
{
	void Create(int width, int height, int shadowWidth, int shadowHeight, uint8_t msaaQuality, bool useAmbientOcclusion, bool useBloom);
	void Recreate(int width, int height, int shadowWidth, int shadowHeight);
	void CleanUp();

	void MakeMainFramebuffer();

	SingleFBO msaaFBO;
	SingleFBO aoFBO;
	DepthFBO shadowFBO;
	PostProcessingFBO ppFBO;
	BloomFBO bloomFBO;
	int baseWidth;
	int baseHeight;
	int shadowWidth;
	int shadowHeight;
	uint8_t _internal_msaa_quality;
	uint8_t _internal_flags;
};



void BeginScene(PScene scene);
void EndScene();
void RenderAmbientOcclusion(PScene scene, const StandardRenderPassData* data, const SceneRenderData* frameData);
void RenderSceneGeometry(PScene scene, const StandardRenderPassData* data);
void RenderSceneShadow(PScene scene, const StandardRenderPassData* data);
void RenderSceneReflectedOnPlane(PScene scene, const ReflectPlanePassData* data);
void RenderSceneStandard(PScene scene, const StandardRenderPassData* data);
void RenderPostProcessingBloom(const struct BloomFBO* bloomData, GLuint finalFBO, int finalSizeX, int finalSizeY, GLuint ppFBOTexture, int ppSizeX, int ppSizeY);

void RenderPostProcessing(const SceneRenderData* rendererData, GLuint finalFBO, int finalSizeX, int finalSizeY);

const CurrentLightInformation* GetCurrentLightInformation();