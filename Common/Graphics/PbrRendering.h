#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include "Scene.h"
#include "GLCompat.h"

struct UBO
{
	glm::mat4 projection;
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
struct PBRSceneObject
{
	BaseSceneObject base;
	void* model;
	glm::mat4* transform;
	GLuint uboParamsUniform;
};
static_assert(sizeof(PBRSceneObject) <= sizeof(SceneObject), "INVALID SIZE");


void InitializePbrPipeline(void* assetManager);
void CleanUpPbrPipeline();

// returns Internal Object
void* CreateInternalPBRFromFile(const char* filename, float scale);
void CleanUpInternal(void* internalObj);

void DrawPBRModel(void* internalObj, GLuint UboUniform, GLuint UBOParamsUniform, GLuint environmentMap, GLuint shadowMap, GLuint globalAOMap, GLuint lightDataUniform, const glm::vec2& renderSz, const glm::mat4* model, const glm::vec4* clipPlane, bool drawOpaque, bool geomOnly);
void DrawPBRModelNode(void* internalObj, GLuint UboUniform, GLuint UBOParamsUniform, GLuint lightDataUniform, GLuint environmentMap, const glm::mat4* model, const glm::vec4* clipPlane, int nodeIdx, bool drawOpaque, bool geomOnly);
void UpdateAnimation(void* internalObj, uint32_t index, float time);



PFUNCDRAWSCENEOBJECT PBRModelGetDrawFunction(TYPE_FUNCTION f);

PBRSceneObject* AddPbrModelToScene(PScene scene, void* internalObj, UBOParams params, const glm::mat4& transform);