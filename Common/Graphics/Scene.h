#pragma once
#include <glm/glm.hpp>
#include "GLCompat.h"

struct StandardRenderPassData
{
	const glm::mat4* camView;
	const glm::mat4* camProj;
	const glm::vec3* camPos;
	const glm::mat4* camViewProj;
	glm::vec2 renderSize;

	GLuint cameraUniform;
	GLuint skyBox;
	GLuint shadowMap;
	GLuint ambientOcclusionMap;

	// These will be set by the renderer
	GLuint lightData;
    bool drawShadow;
};
struct ReflectPlanePassData
{
	const StandardRenderPassData* base;
	const glm::vec4* planeEquation;
};
struct AmbientOcclusionPassData
{
	const StandardRenderPassData* std;
	GLuint noiseTexture;
	GLuint depthMap;
	GLuint aoUBO;
};



typedef void* PScene;

struct BoundingBox
{
	glm::vec3 leftTopFront;
	glm::vec3 rightBottomBack;
};

enum SCENE_OBJECT_FLAGS
{
	SCENE_OBJECT_OPAQUE = 1,				// requires		Opaque Draw					Function
	SCENE_OBJECT_BLEND = 1 << 1,			// requires		Blend Draw					Function
	SCENE_OBJECT_CAST_SHADOW = 1 << 2,		// requires		Shadow Draw					Function
	SCENE_OBJECT_REFLECTED = 1 << 3,		// requires		Opaque or Blend Draw		Function
	SCENE_OBJECT_SURFACE_REFLECTED = 1 << 4,// requires		Clip Plane Draw				Function 
};
enum TYPE_FUNCTION
{
	TYPE_FUNCTION_SHADOW,
	TYPE_FUNCTION_GEOMETRY,
	TYPE_FUNCTION_OPAQUE,
	TYPE_FUNCTION_BLEND,
	TYPE_FUNCTION_CLIP_PLANE_OPAQUE,	// drawing with a set clipping plane, used for reflective surfaces
	TYPE_FUNCTION_CLIP_PLANE_BLEND,		// drawing with a set clipping plane, used for reflective surfaces
	NUM_TYPE_FUNCTION
};

struct SceneObject {
	BoundingBox bbox;
	uint16_t lightGroups;	// bit field
	uint16_t flags;
	uint32_t additionalFlags;

    virtual ~SceneObject() {}

    virtual size_t GetType() const = 0;
    virtual void DrawGeometry(StandardRenderPassData* pass) = 0;
    virtual void DrawOpaque(StandardRenderPassData* pass) = 0;
    virtual void DrawBlend(StandardRenderPassData* pass) = 0;
    virtual void DrawBlendClip(ReflectPlanePassData* pass) = 0;
    virtual void DrawOpaqueClip(ReflectPlanePassData* pass) = 0;


};



PScene SC_CreateScene();
void SC_DestroyScene(PScene scene);

void SC_AddSceneObject(PScene scene, SceneObject* obj);

ScenePointLight* SC_AddPointLight(PScene scene);
SceneDirLight* SC_AddDirectionalLight(PScene scene);

void SC_RemoveSceneObject(PScene scene, uint32_t typeIndex, SceneObject* rmObj);

void SC_RemovePointLight(PScene scene, ScenePointLight* light);
void SC_RemoveDirectionalLight(PScene scene, SceneDirLight* light);

int SC_GetNumLights(PScene scene);
void SC_FillLights(PScene scene, ScenePointLight** pl, SceneDirLight** dl);


SceneObject** SC_GenerateRenderList(PScene scene);
SceneObject** SC_FillRenderList(PScene scene, SceneObject** list, const glm::mat4* viewProjMatrix, int* num, uint32_t objFlag);
SceneObject** SC_AppendRenderList(PScene scene, SceneObject** list, const glm::mat4* viewProjMatrix, int* num, uint32_t objFlag, int curNum);
void SC_FreeRenderList(SceneObject** list);


