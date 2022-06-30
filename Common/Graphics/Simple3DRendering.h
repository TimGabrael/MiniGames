#pragma once
#include <glm/glm.hpp>
#include "Graphics/GLCompat.h"
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

void InitializeSimple3DPipeline();
void CleanUpSimple3DPipeline();

S3DCombinedBuffer S3DGenerateBuffer(SVertex3D* verts, size_t numVerts, uint32_t* indices, size_t numIndices);
S3DVertexBuffer S3DGenerateBuffer(SVertex3D* verts, size_t numVerts);


void DrawSimple3D(const S3DCombinedBuffer& buf, const glm::mat4& proj, const glm::mat4& view, const glm::mat4& model = glm::mat4(1.0f));
void DrawSimple3D(const S3DCombinedBuffer& buf, const glm::mat4& proj, const glm::mat4& view, GLuint texture, const glm::mat4& model = glm::mat4(1.0f));
void DrawSimple3D(const S3DVertexBuffer& buf, const glm::mat4& proj, const glm::mat4& view, const glm::mat4& model = glm::mat4(1.0f));
void DrawSimple3D(const S3DVertexBuffer& buf, const glm::mat4& proj, const glm::mat4& view, GLuint texture, const glm::mat4& model = glm::mat4(1.0f));