#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "GLCompat.h"

struct UBO
{
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	glm::vec3 camPos;
};
struct UBOParams
{
	glm::vec4 lightDir;
	float exposure = 4.5f;
	float gamma = 2.2f;
	float prefilteredCubeMipLevels = 1.0f;
	float scaleIBLAmbient = 1.0f;
	float debugViewInputs = 0.0f;
	float debugViewEquation = 0.0f;
};

void InitializePbrPipeline(void* assetManager);
void CleanUpPbrPipeline();

// returns Internal Object
void* CreateInternalPBRFromFile(const char* filename, float scale);
void CleanUpInternal(void* internalObj);

void DrawPBRModel(void* internalObj, GLuint UboUniform, GLuint UBOParamsUniform, GLuint environmentMap, bool drawOpaque);
void DrawPBRModelNode(void* internalObj, GLuint UboUniform, GLuint UBOParamsUniform, GLuint environmentMap, int nodeIdx, bool drawOpaque);
void UpdateAnimation(void* internalObj, uint32_t index, float time);