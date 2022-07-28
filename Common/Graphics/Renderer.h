#pragma once
#include "Scene.h"
#include "Camera.h"

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
	GLuint lightData;
	GLuint shadowMap;
};
struct ReflectPlanePassData
{
	const StandardRenderPassData* base;
	const glm::vec4* planeEquation;
};

void InitializeRendererBackendData();
void CleanUpRendererBackendData();


void BeginScene(PScene scene);
void EndScene();
void RenderSceneShadow(PScene scene, const StandardRenderPassData* data);
void RenderSceneReflectedOnPlane(PScene scene, const ReflectPlanePassData* data);
void RenderSceneStandard(PScene scene, const StandardRenderPassData* data);
void RenderPostProcessingBloom(struct BloomFBO* bloomData, GLuint finalFBO, int finalSizeX, int finalSizeY);

const CurrentLightInformation* GetCurrentLightInformation();