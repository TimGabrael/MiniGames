#include "Simple3DRendering.h"
#include "Helper.h"

static const char* uiVertexShader = "#version 300 es\n\
\n\
layout(location = 0) in vec3 pos;\n\
layout(location = 1) in vec2 texPos;\n\
layout(location = 2) in vec4 color;\n\
uniform mat4 projection;\n\
uniform mat4 model;\n\
uniform mat4 view;\n\
out vec2 tPos;\
out vec4 col;\
void main(){\
	gl_Position = projection * view * model * vec4(pos, 1.0f);\
	tPos = texPos;\
	col = color;\
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


void CleanUpSimple3DPipeline()
{
	glDeleteProgram(g_3d.program);
	glDeleteTextures(1, &g_3d.defaultTexture);
}



void DrawSimple3D(const S3DCombinedBuffer& buf, const glm::mat4& proj, const glm::mat4& view, const glm::mat4& model)
{
	glUseProgram(g_3d.program);
	glUniformMatrix4fv(g_3d.unis.proj, 1, GL_FALSE, (const GLfloat*)&proj);
	glUniformMatrix4fv(g_3d.unis.view, 1, GL_FALSE, (const GLfloat*)&view);
	glUniformMatrix4fv(g_3d.unis.model, 1, GL_FALSE, (const GLfloat*)&model);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_3d.defaultTexture);

	glBindVertexArray(buf.vtxBuf.vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf.idxBuf);


	glDrawElements(GL_TRIANGLES, buf.numIndices, GL_UNSIGNED_INT, nullptr);

	glBindVertexArray(0);
}
void DrawSimple3D(const S3DCombinedBuffer& buf, const glm::mat4& proj, const glm::mat4& view, GLuint texture, const glm::mat4& model)
{
	glUseProgram(g_3d.program);
	glUniformMatrix4fv(g_3d.unis.proj, 1, GL_FALSE, (const GLfloat*)&proj);
	glUniformMatrix4fv(g_3d.unis.view, 1, GL_FALSE, (const GLfloat*)&view);
	glUniformMatrix4fv(g_3d.unis.model, 1, GL_FALSE, (const GLfloat*)&model);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glBindVertexArray(buf.vtxBuf.vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf.idxBuf);


	glDrawElements(GL_TRIANGLES, buf.numIndices, GL_UNSIGNED_INT, nullptr);

	glBindVertexArray(0);

}
void DrawSimple3D(const S3DVertexBuffer& buf, const glm::mat4& proj, const glm::mat4& view, const glm::mat4& model)
{
	glUseProgram(g_3d.program);
	glUniformMatrix4fv(g_3d.unis.proj, 1, GL_FALSE, (const GLfloat*)&proj);
	glUniformMatrix4fv(g_3d.unis.view, 1, GL_FALSE, (const GLfloat*)&view);
	glUniformMatrix4fv(g_3d.unis.model, 1, GL_FALSE, (const GLfloat*)&model);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_3d.defaultTexture);

	glBindVertexArray(buf.vao);

	glDrawArrays(GL_TRIANGLES, 0, buf.numVertices);

	glBindVertexArray(0);
}
void DrawSimple3D(const S3DVertexBuffer& buf, const glm::mat4& proj, const glm::mat4& view, GLuint texture, const glm::mat4& model)
{
	glUseProgram(g_3d.program);
	glUniformMatrix4fv(g_3d.unis.proj, 1, GL_FALSE, (const GLfloat*)&proj);
	glUniformMatrix4fv(g_3d.unis.view, 1, GL_FALSE, (const GLfloat*)&view);
	glUniformMatrix4fv(g_3d.unis.model, 1, GL_FALSE, (const GLfloat*)&model);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glBindVertexArray(buf.vao);

	glDrawArrays(GL_TRIANGLES, 0, buf.numVertices);

	glBindVertexArray(0);
}