#pragma once
#include <glm/glm.hpp>
#include "GLCompat.h"


struct Vertex2D
{
	glm::vec2 pos;
	glm::vec2 texPos;
	uint32_t color;
};

void InitializeUiPipeline();

void DrawQuad(const glm::vec2& tl, const glm::vec2& br, uint32_t color);

void DrawUI(); // draws everything that has been accumulated so far