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
	GLuint cameraUniform;
	GLuint skyBox;
};
struct ReflectPlanePassData
{
	StandardRenderPassData base;
	const glm::vec4* planeEquation;
};


void RenderSceneReflectedOnPlane(PScene scene, const Camera* cam, const glm::vec4* planeEquation, GLuint cameraUniform, GLuint skybox);
void RenderSceneStandard(PScene scene, const glm::mat4* camView, const glm::mat4* camProj, const glm::vec3* camPos, GLuint cameraUniform, GLuint skybox);