#pragma once
#include <glm/glm.hpp>
#include "Graphics/GLCompat.h"
#include "Scene.h"
// for all the cases that gltf is maybe a little overkill



struct SVertex3D
{
	glm::vec3 pos;
	glm::vec2 tex;
	uint32_t col;
};

struct S3DVertexBuffer
{
	GLuint vao;
	GLuint vtxBuf;
	uint32_t numVertices;
};
struct S3DCombinedBuffer
{
	S3DVertexBuffer vtxBuf;
	GLuint idxBuf;
	uint32_t numIndices;
};

struct S3DSceneObject
{
	BaseSceneObject base;
	S3DCombinedBuffer* mesh;
	uintptr_t texture;
	glm::mat4* transform;
};
static_assert(sizeof(S3DSceneObject) <= sizeof(SceneObject), "SIZE_MISMATCH");

void InitializeSimple3DPipeline();
void CleanUpSimple3DPipeline();

S3DCombinedBuffer S3DGenerateBuffer(SVertex3D* verts, size_t numVerts, uint32_t* indices, size_t numIndices);
S3DVertexBuffer S3DGenerateBuffer(SVertex3D* verts, size_t numVerts);

BoundingBox GenerateBoundingBoxFromVertices(SVertex3D* verts, size_t numVerts);
S3DSceneObject* AddSceneObject(PScene scene, uint32_t s3dTypeIndex, SVertex3D* verts, size_t numVerts);
S3DSceneObject* AddSceneObject(PScene scene, uint32_t s3dTypeIndex, SVertex3D* verts, size_t numVerts, uint32_t* indices, size_t numIndices);



void DrawSimple3D(const S3DCombinedBuffer& buf, const glm::mat4& proj, const glm::mat4& view, const glm::mat4& model = glm::mat4(1.0f));
void DrawSimple3D(const S3DCombinedBuffer& buf, const glm::mat4& proj, const glm::mat4& view, GLuint texture, const glm::mat4& model = glm::mat4(1.0f));
void DrawSimple3D(const S3DVertexBuffer& buf, const glm::mat4& proj, const glm::mat4& view, const glm::mat4& model = glm::mat4(1.0f));
void DrawSimple3D(const S3DVertexBuffer& buf, const glm::mat4& proj, const glm::mat4& view, GLuint texture, const glm::mat4& model = glm::mat4(1.0f));


PFUNCDRAWSCENEOBJECT S3DGetDrawFunctions(TYPE_FUNCTION f);