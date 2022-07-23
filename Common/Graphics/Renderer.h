#pragma once
#include "Scene.h"
#include "Camera.h"

struct CameraData
{
	glm::mat4 projection;
	glm::mat4 view;
	glm::vec3 camPos;
};

struct StandardRenderPassData
{
	const glm::mat4* camView;
	const glm::mat4* camProj;
	const glm::vec3* camPos;
	const glm::vec3* lightDir;	// base light of the scene
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