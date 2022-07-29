#pragma once
#include "Scene.h"
#include "Camera.h"
#include "Helper.h"


struct RendererSettings
{
	int msaaQuality;
	int shadowRatio;
	bool ambientOcclusionActive;
	bool bloomActive;
};

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
};
struct SceneRenderData
{
	void Create(int width, int height, const RendererSettings* settings);
	void Recreate(int width, int size);
	SingleFBO msaaFBO;
	SingleFBO aoFBO;
	DepthFBO shadowFBO;
	PostProcessingFBO ppFBO;
	BloomFBO bloomFBO;
	uint32_t _internal;
};



void BeginScene(PScene scene);
void EndScene();
void RenderSceneGeometry(PScene scene, const StandardRenderPassData* data);
void RenderSceneShadow(PScene scene, const StandardRenderPassData* data);
void RenderSceneReflectedOnPlane(PScene scene, const ReflectPlanePassData* data);
void RenderSceneStandard(PScene scene, const StandardRenderPassData* data);
void RenderPostProcessingBloom(struct BloomFBO* bloomData, GLuint finalFBO, int finalSizeX, int finalSizeY);

const CurrentLightInformation* GetCurrentLightInformation();