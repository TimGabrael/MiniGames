#pragma once
#include "Scene.h"

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
	GLuint cameraUniform;
	GLuint skyBox;
};


void RenderSceneStandard(PScene scene, const glm::mat4* camView, const glm::mat4* camProj, GLuint cameraUniform, GLuint skybox);