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
struct PBRSceneObject : public SceneObject
{
	void* model;
	glm::mat4 transform;
	GLuint uboParamsUniform;
    void Set(void* internal, UBOParams params, const glm::mat4& transform);

    virtual ~PBRSceneObject();
    virtual size_t GetType() const override;
    virtual void DrawGeometry(StandardRenderPassData* pass) override;
    virtual void DrawOpaque(StandardRenderPassData* pass) override;
    virtual void DrawBlend(StandardRenderPassData* pass) override;
    virtual void DrawBlendClip(ReflectPlanePassData* pass) override;
    virtual void DrawOpaqueClip(ReflectPlanePassData* pass) override;
};


void InitializePbrPipeline(void* assetManager);
void CleanUpPbrPipeline();

// returns Internal Object
void* CreateInternalPBRFromFile(const char* filename, float scale);
void CleanUpInternal(void* internalObj);

void UpdateAnimation(void* internalObj, uint32_t index, float time);


PBRSceneObject* AddPbrModelToScene(PScene scene, void* internal, UBOParams params, const glm::mat4& transform);

