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
	GLuint vao = 0;
	GLuint vtxBuf = 0;
	uint32_t numVertices = 0;
};
struct S3DCombinedBuffer
{
	S3DVertexBuffer vtxBuf;
	GLuint idxBuf = 0;
	uint32_t numIndices = 0;
};

struct S3DSceneObject : public SceneObject
{
	S3DCombinedBuffer mesh;
	GLuint texture;
	glm::mat4 transform;

    virtual ~S3DSceneObject();
    virtual size_t GetType() const override;
    virtual void DrawGeometry(StandardRenderPassData* pass) override;
    virtual void DrawOpaque(StandardRenderPassData* pass) override;
    virtual void DrawBlend(StandardRenderPassData* pass) override;
    virtual void DrawBlendClip(ReflectPlanePassData* pass) override;
    virtual void DrawOpaqueClip(ReflectPlanePassData* pass) override;
};

void InitializeSimple3DPipeline();
void CleanUpSimple3DPipeline();

S3DCombinedBuffer S3DGenerateBuffer(SVertex3D* verts, size_t numVerts, uint32_t* indices, size_t numIndices);
S3DVertexBuffer S3DGenerateBuffer(SVertex3D* verts, size_t numVerts);

S3DSceneObject* AddSceneObject(PScene scene, uint32_t s3dTypeIndex, SVertex3D* verts, size_t numVerts);
S3DSceneObject* AddSceneObject(PScene scene, uint32_t s3dTypeIndex, SVertex3D* verts, size_t numVerts, uint32_t* indices, size_t numIndices);


void DrawSimple3D(const S3DCombinedBuffer& buf, const glm::mat4& proj, const glm::mat4& view, const glm::mat4& model, bool geometryOnly);

