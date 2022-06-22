#pragma once
#include <glm/glm.hpp>
#include <glad/glad.h>



struct Vertex2D
{
	glm::vec2 pos;
	glm::vec2 texPos;
	uint32_t color;
};

void InitializeUiPipeline();

void DebugDrawTexture(GLuint texture);