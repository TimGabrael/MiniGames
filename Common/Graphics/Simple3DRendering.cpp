#include "Simple3DRendering.h"
#include "Helper.h"
#include "logging.h"
#include "Renderer.h"

static const char* uiVertexShader = "#version 300 es\n\
#extension GL_EXT_clip_cull_distance : enable\
\n\
layout(location = 0) in vec3 pos;\n\
layout(location = 1) in vec2 texPos;\n\
layout(location = 2) in vec4 color;\n\
uniform mat4 projection;\n\
uniform mat4 model;\n\
uniform mat4 view;\n\
uniform vec4 clipPlane;\n\
out vec2 tPos;\
out vec4 col;\
void main(){\
	vec4 pos = model * vec4(pos, 1.0f);\
	gl_Position = projection * view * pos;\
	tPos = texPos;\
	col = color;\
	gl_ClipDistance[0] = dot(gl_Position, clipPlane);\
}\
";

static const char* uiFragmentShader = "#version 300 es\n\
precision highp float;\n\
\n\
in vec2 tPos;\
in vec4 col;\
uniform sampler2D tex;\
out vec4 outCol;\
void main(){\
	outCol = texture(tex, tPos).rgba * col;\
}\
";

struct UniformLocations
{
	GLuint proj;
	GLuint model;
	GLuint view;
	GLuint plane;
};

struct Basic3DPipelineObjects
{
	GLuint program;
	GLuint defaultTexture;

	UniformLocations unis;

}g_3d;


void InitializeSimple3DPipeline()
{
	g_3d.program = CreateProgram(uiVertexShader, uiFragmentShader);

	g_3d.unis.proj = glGetUniformLocation(g_3d.program, "projection");
	g_3d.unis.model = glGetUniformLocation(g_3d.program, "model");
	g_3d.unis.view = glGetUniformLocation(g_3d.program, "view");
	g_3d.unis.plane = glGetUniformLocation(g_3d.program, "clipPlane");

	glGenTextures(1, &g_3d.defaultTexture);
	glBindTexture(GL_TEXTURE_2D, g_3d.defaultTexture);

	uint32_t defTexData[2] = { 0xFFFFFFFF, 0xFFFFFFFF };
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, defTexData);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindTexture(GL_TEXTURE_2D, 0);



}

S3DCombinedBuffer S3DGenerateBuffer(SVertex3D* verts, size_t numVerts, uint32_t* indices, size_t numIndices)
{
	S3DCombinedBuffer res;
	res.vtxBuf = S3DGenerateBuffer(verts, numVerts);
	res.numIndices = numIndices;
	glBindVertexArray(res.vtxBuf.vao);

	glGenBuffers(1, &res.idxBuf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, res.idxBuf);

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * numIndices, indices, GL_STATIC_DRAW);

	glBindVertexArray(0);

	return res;
}
S3DVertexBuffer S3DGenerateBuffer(SVertex3D* verts, size_t numVerts)
{
	S3DVertexBuffer res;
	res.numVertices = numVerts;
	glGenVertexArrays(1, &res.vao);

	glBindVertexArray(res.vao);
	glGenBuffers(1, &res.vtxBuf);

	glBindBuffer(GL_ARRAY_BUFFER, res.vtxBuf);
	glBufferData(GL_ARRAY_BUFFER, numVerts * sizeof(SVertex3D), verts, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SVertex3D), (const void*)0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(SVertex3D), (const void*)(offsetof(SVertex3D, tex)));
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, true, sizeof(SVertex3D), (const void*)(offsetof(SVertex3D, col)));
	
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);
	return res;
}

BoundingBox GenerateBoundingBoxFromVertices(SVertex3D* verts, size_t numVerts)
{
	assert(numVerts != 0);
	BoundingBox r = { verts[0].pos, verts[0].pos };
	glm::vec3& rl = r.leftTopFront;
	glm::vec3& rh = r.rightBottomBack;

	for (size_t i = 1; i < numVerts; i++)
	{
		glm::vec3 p = verts[i].pos;
		p += glm::vec3(0.1f);
		if (p.x > rh.x) rh.x = p.x;
		if (p.y > rh.y) rh.y = p.y;
		if (p.z > rh.z) rh.z = p.z;
		p -= glm::vec3(0.2f);
		if (p.x < rl.x) rl.x = p.x;
		if (p.y < rl.y) rl.y = p.y;
		if (p.z < rl.z) rl.z = p.z;
	}

	return r;
}
S3DSceneObject* AddSceneObject(PScene scene, uint32_t s3dTypeIndex, SVertex3D* verts, size_t numVerts)
{
	S3DSceneObject* obj = (S3DSceneObject*)SC_AddSceneObject(scene, s3dTypeIndex);
	if (!obj) return nullptr;

	obj->mesh = (S3DCombinedBuffer*)SC_AddMesh(scene, s3dTypeIndex, sizeof(S3DCombinedBuffer));

	obj->mesh->vtxBuf = S3DGenerateBuffer(verts, numVerts);
	obj->texture = 0;
	obj->transform = nullptr;
	obj->base.bbox = GenerateBoundingBoxFromVertices(verts, numVerts);
	obj->base.flags = SCENE_OBJECT_FLAGS::SCENE_OBJECT_OPAQUE | SCENE_OBJECT_FLAGS::SCENE_OBJECT_CAST_SHADOW | SCENE_OBJECT_FLAGS::SCENE_OBJECT_REFLECTED;
	obj->base.additionalFlags = 1;
	

	return obj;
}
S3DSceneObject* AddSceneObject(PScene scene, uint32_t s3dTypeIndex, SVertex3D* verts, size_t numVerts, uint32_t* indices, size_t numIndices)
{
	S3DSceneObject* obj = (S3DSceneObject*)SC_AddSceneObject(scene, s3dTypeIndex);
	if (!obj) return nullptr;

	obj->mesh = (S3DCombinedBuffer*)SC_AddMesh(scene, s3dTypeIndex, sizeof(S3DCombinedBuffer));

	*obj->mesh = S3DGenerateBuffer(verts, numVerts, indices, numIndices);
	obj->texture = 0;
	obj->transform = nullptr;
	obj->base.bbox = GenerateBoundingBoxFromVertices(verts, numVerts);
	obj->base.flags = SCENE_OBJECT_FLAGS::SCENE_OBJECT_OPAQUE | SCENE_OBJECT_FLAGS::SCENE_OBJECT_CAST_SHADOW | SCENE_OBJECT_FLAGS::SCENE_OBJECT_REFLECTED;
	obj->base.additionalFlags = 1;


	return obj;
}

void CleanUpSimple3DPipeline()
{
	glDeleteProgram(g_3d.program);
	glDeleteTextures(1, &g_3d.defaultTexture);
}



void DrawSimple3D(const S3DCombinedBuffer& buf, const glm::mat4& proj, const glm::mat4& view, const glm::mat4& model)
{
	glUseProgramWrapper(g_3d.program);
	glUniformMatrix4fv(g_3d.unis.proj, 1, GL_FALSE, (const GLfloat*)&proj);
	glUniformMatrix4fv(g_3d.unis.view, 1, GL_FALSE, (const GLfloat*)&view);
	glUniformMatrix4fv(g_3d.unis.model, 1, GL_FALSE, (const GLfloat*)&model);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_3d.defaultTexture);

	glBindVertexArray(buf.vtxBuf.vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf.idxBuf);


	glDrawElementsWrapper(GL_TRIANGLES, buf.numIndices, GL_UNSIGNED_INT, nullptr);

	glBindVertexArray(0);
}
void DrawSimple3D(const S3DCombinedBuffer& buf, const glm::mat4& proj, const glm::mat4& view, GLuint texture, const glm::mat4& model)
{
	glUseProgramWrapper(g_3d.program);
	glUniformMatrix4fv(g_3d.unis.proj, 1, GL_FALSE, (const GLfloat*)&proj);
	glUniformMatrix4fv(g_3d.unis.view, 1, GL_FALSE, (const GLfloat*)&view);
	glUniformMatrix4fv(g_3d.unis.model, 1, GL_FALSE, (const GLfloat*)&model);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glBindVertexArray(buf.vtxBuf.vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf.idxBuf);


	glDrawElementsWrapper(GL_TRIANGLES, buf.numIndices, GL_UNSIGNED_INT, nullptr);

	glBindVertexArray(0);

}
void DrawSimple3D(const S3DVertexBuffer& buf, const glm::mat4& proj, const glm::mat4& view, const glm::mat4& model)
{
	glUseProgramWrapper(g_3d.program);
	glUniformMatrix4fv(g_3d.unis.proj, 1, GL_FALSE, (const GLfloat*)&proj);
	glUniformMatrix4fv(g_3d.unis.view, 1, GL_FALSE, (const GLfloat*)&view);
	glUniformMatrix4fv(g_3d.unis.model, 1, GL_FALSE, (const GLfloat*)&model);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_3d.defaultTexture);

	glBindVertexArray(buf.vao);

	glDrawArraysWrapper(GL_TRIANGLES, 0, buf.numVertices);

	glBindVertexArray(0);
}
void DrawSimple3D(const S3DVertexBuffer& buf, const glm::mat4& proj, const glm::mat4& view, GLuint texture, const glm::mat4& model)
{
	glUseProgramWrapper(g_3d.program);
	glUniformMatrix4fv(g_3d.unis.proj, 1, GL_FALSE, (const GLfloat*)&proj);
	glUniformMatrix4fv(g_3d.unis.view, 1, GL_FALSE, (const GLfloat*)&view);
	glUniformMatrix4fv(g_3d.unis.model, 1, GL_FALSE, (const GLfloat*)&model);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glBindVertexArray(buf.vao);

	glDrawArraysWrapper(GL_TRIANGLES, 0, buf.numVertices);

	glBindVertexArray(0);
}
void DrawSimple3D(const S3DVertexBuffer& buf, const glm::mat4& proj, const glm::mat4& view, GLuint texture, const glm::mat4& model, const glm::vec4& clipPlane)
{
	glUseProgramWrapper(g_3d.program);
	glUniformMatrix4fv(g_3d.unis.proj, 1, GL_FALSE, (const GLfloat*)&proj);
	glUniformMatrix4fv(g_3d.unis.view, 1, GL_FALSE, (const GLfloat*)&view);
	glUniformMatrix4fv(g_3d.unis.model, 1, GL_FALSE, (const GLfloat*)&model);
	glUniform4fv(g_3d.unis.plane, 1, (const GLfloat*)&clipPlane);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glBindVertexArray(buf.vao);

	glDrawArraysWrapper(GL_TRIANGLES, 0, buf.numVertices);

	glBindVertexArray(0);
}
void DrawSimple3D(const S3DCombinedBuffer& buf, const glm::mat4& proj, const glm::mat4& view, GLuint texture, const glm::mat4& model, const glm::vec4& clipPlane)
{
	glUseProgramWrapper(g_3d.program);
	glUniformMatrix4fv(g_3d.unis.proj, 1, GL_FALSE, (const GLfloat*)&proj);
	glUniformMatrix4fv(g_3d.unis.view, 1, GL_FALSE, (const GLfloat*)&view);
	glUniformMatrix4fv(g_3d.unis.model, 1, GL_FALSE, (const GLfloat*)&model);
	glUniform4fv(g_3d.unis.plane, 1, (const GLfloat*)&clipPlane);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glBindVertexArray(buf.vtxBuf.vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf.idxBuf);


	glDrawElementsWrapper(GL_TRIANGLES, buf.numIndices, GL_UNSIGNED_INT, nullptr);

	glBindVertexArray(0);

}


void DrawSimple3DOpaque(SceneObject* sceneObject, void* renderPassData)
{
	S3DSceneObject* obj = (S3DSceneObject*)sceneObject;
	StandardRenderPassData* data = (StandardRenderPassData*)renderPassData;
	glm::mat4 matrix(1.0f);
	if (obj->transform)
	{
		matrix = *obj->transform;
	}
	if (obj->base.additionalFlags)	// 0 => combined buf,  1 => vertex buf
	{
		if (obj->texture) DrawSimple3D(*obj->mesh, *data->camProj, *data->camView, obj->texture, matrix);
		else DrawSimple3D(*obj->mesh, *data->camProj, *data->camView, matrix);
	}
	else
	{
		if (obj->texture) DrawSimple3D(obj->mesh->vtxBuf, *data->camProj, *data->camView, obj->texture, matrix);
		else DrawSimple3D(obj->mesh->vtxBuf, *data->camProj, *data->camView, matrix);
	}
}
void DrawSimple3DOpaqueClipPlane(SceneObject* sceneObject, void* renderPassData)
{
	S3DSceneObject* obj = (S3DSceneObject*)sceneObject;
	ReflectPlanePassData* planeData = (ReflectPlanePassData*)renderPassData;
	StandardRenderPassData* data = &planeData->base;
	glm::mat4 matrix(1.0f);
	
	if (obj->transform)
	{
		matrix = *obj->transform;
	}
	if (obj->base.additionalFlags)	// 0 => combined buf,  1 => vertex buf
	{
		if (obj->texture) DrawSimple3D(*obj->mesh, *data->camProj, *data->camView, obj->texture, matrix, *planeData->planeEquation);
		else DrawSimple3D(*obj->mesh, *data->camProj, *data->camView, matrix);
	}
	else
	{
		if (obj->texture) DrawSimple3D(obj->mesh->vtxBuf, *data->camProj, *data->camView, obj->texture, matrix, *planeData->planeEquation);
		else DrawSimple3D(obj->mesh->vtxBuf, *data->camProj, *data->camView, g_3d.defaultTexture, matrix);
	}
}


TypeFunctions S3DGetDrawFunctions()
{
	TypeFunctions tf;
	tf.GeometryDraw = nullptr;
	tf.OpaqueDraw = DrawSimple3DOpaque;
	tf.BlendDraw = nullptr;
	tf.ClipPlaneOpaqueDraw = DrawSimple3DOpaqueClipPlane;
	tf.ClipPlaneBlendDraw = nullptr;
	return tf;
}