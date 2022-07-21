#pragma once
#include <glm/glm.hpp>
#include "GLCompat.h"


typedef void* PScene;
typedef void* PMesh;
typedef void* PMaterial;
typedef void* PTransform;
struct SceneObject;

typedef void(*PFUNCDRAWSCENEOBJECT)(SceneObject* obj, void* renderPassData);

struct BoundingBox
{
	glm::vec3 leftTopFront;
	glm::vec3 rightBottomBack;
};

enum SCENE_OBJECT_FLAGS
{
	SCENE_OBJECT_OPAQUE = 1,				// requires		Opaque Draw					Function
	SCENE_OBJECT_BLEND = 1 << 1,			// requires		Blend Draw					Function
	SCENE_OBJECT_CAST_SHADOW = 1 << 2,		// requires		Geometry Draw				Function
	SCENE_OBJECT_REFLECTED = 1 << 3,		// requires		Opaque or Blend Draw		Function
	SCENE_OBJECT_SURFACE_REFLECTED = 1 << 4,// requires		Clip Plane Draw				Function 
};
enum TYPE_FUNCTION
{
	TYPE_FUNCTION_GEOMETRY,
	TYPE_FUNCTION_OPAQUE,
	TYPE_FUNCTION_BLEND,
	TYPE_FUNCTION_CLIP_PLANE_OPAQUE,	// drawing with a set clipping plane, used for reflective surfaces
	TYPE_FUNCTION_CLIP_PLANE_BLEND,		// drawing with a set clipping plane, used for reflective surfaces
	NUM_TYPE_FUNCTION
};
typedef PFUNCDRAWSCENEOBJECT(*PFUNCGETDRAWFUNCTION)(TYPE_FUNCTION f);

struct BaseSceneObject
{
	BoundingBox bbox;
	uint32_t flags;
	uint32_t additionalFlags;
};

struct SceneObject	// the mesh/material/transform component can be interchanged with anything, but the Base shall stay the same for all scene object types
{
	BaseSceneObject base;
	PMesh mesh;					// these just show intended uses, nothing is enforced here
	PMaterial material;			// these just show intended uses, nothing is enforced here
	PTransform transform;		// these just show intended uses, nothing is enforced here
};


struct ObjectRenderStruct
{
	SceneObject* obj;
	PFUNCDRAWSCENEOBJECT DrawFunc;
};




PScene SC_CreateScene();

// returns the type index, increases by one every time a new type is added starting from 0
uint32_t SC_AddType(PScene scene, PFUNCGETDRAWFUNCTION functions);
PMaterial SC_AddMaterial(PScene scene, uint32_t typeIndex, uint32_t materialSize);
PMesh SC_AddMesh(PScene scene, uint32_t typeIndex, uint32_t meshSize);
PTransform SC_AddTransform(PScene scene, uint32_t typeIndex, uint32_t transformSize);
SceneObject* SC_AddSceneObject(PScene scene, uint32_t typeIndex);

void SC_RemoveType(PScene scene, uint32_t typeIndex);
void SC_RemoveMaterial(PScene scene, uint32_t typeIndex, uint32_t materialSize, PMaterial rmMaterial);
void SC_RemoveMesh(PScene scene, uint32_t typeIndex, uint32_t meshSize, PMesh rmMesh);
void SC_RemoveTransform(PScene scene, uint32_t typeIndex, uint32_t transformSize, PTransform rmTransform);
void SC_RemoveSceneObject(PScene scene, uint32_t typeIndex, SceneObject* rmObj);


ObjectRenderStruct* SC_GenerateRenderList(PScene scene);
ObjectRenderStruct* SC_FillRenderList(PScene scene, ObjectRenderStruct* list, const glm::mat4* viewProjMatrix, int* num, TYPE_FUNCTION funcType, uint32_t objFlag);
void SC_FreeRenderList(ObjectRenderStruct* list);