#pragma once
#include "Scene.h"
#include "Camera.h"

struct CameraData
{
	glm::mat4 projection;
	glm::mat4 view;
	glm::vec3 camPos;
};
struct DirectionalLightData
{
	const glm::mat4* viewProjMat;
	const glm::vec3* dir;
	GLuint shadowMap;
};

struct StandardRenderPassData
{
	const glm::mat4* camView;
	const glm::mat4* camProj;
	const glm::vec3* camPos;
	DirectionalLightData light;	// base light of the scene may be 0
	GLuint cameraUniform;
	GLuint skyBox;
};
struct ReflectPlanePassData
{
	const StandardRenderPassData* base;
	const glm::vec4* planeEquation;
};

void BeginScene(PScene scene);
void EndScene();
void RenderSceneShadow(PScene scene, const StandardRenderPassData* data);
void RenderSceneReflectedOnPlane(PScene scene, const ReflectPlanePassData* data);
void RenderSceneStandard(PScene scene, const StandardRenderPassData* data);