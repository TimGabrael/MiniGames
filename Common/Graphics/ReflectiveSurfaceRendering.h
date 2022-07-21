#pragma once
#include "Scene.h"

struct ReflectiveSurfaceMaterialData
{
	glm::vec4 tintColor;
	float moveFactor;
	float distortionFactor;
	int type;
};
struct ReflectiveSurfaceTextures
{
	GLuint reflect;
	GLuint refract;
	GLuint dudv;
};

void InitializeReflectiveSurfacePipeline();

SceneObject* AddReflectiveSurface(PScene scene, const glm::vec3* pos, const glm::vec3* normal, float scaleX, float scaleY, const ReflectiveSurfaceMaterialData* data, const ReflectiveSurfaceTextures* texData);

void ReflectiveSurfaceSetTextureData(SceneObject* obj, const ReflectiveSurfaceTextures* texData);

PFUNCDRAWSCENEOBJECT ReflectiveSurfaceGetDrawFunction(TYPE_FUNCTION f);