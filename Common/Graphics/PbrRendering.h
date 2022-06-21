#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glad/glad.h>

struct UBO
{
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	glm::vec3 camPos;
};

// returns Internal Object
void* CreateInternalPBRFromFile(const char* filename, float scale);
void CleanUpInternal(void* internalObj);

void InitializePbrPipeline();
void DrawPBRModel(void* internalObj, GLuint UboUniform, GLuint environmentMap);