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
void CleanUpUiPipeline();

void DrawQuad(const glm::vec2& tl, const glm::vec2& br, uint32_t color);
void DrawQuad(const glm::vec2& tl, const glm::vec2& br, uint32_t color, GLuint texture);
void DrawCircle(const glm::vec2& center, const glm::vec2& rad, float angleStart, float fillAngle, uint32_t color, int samples);	// angle start = 0.0f is top
void DrawLine(const glm::vec2& first, const glm::vec2& second, uint32_t col);

void DrawUI(); // draws everything that has been accumulated so far