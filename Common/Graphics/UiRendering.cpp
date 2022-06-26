#include "UiRendering.h"
#include "Helper.h"
#include "logging.h"

static const char* uiVertexShader = "#version 300 es\n\
\n\
layout(location = 0) in vec2 pos;\n\
layout(location = 1) in vec2 texPos;\n\
layout(location = 2) in vec4 color;\n\
out vec2 tPos;\
out vec4 col;\
void main(){\
	gl_Position = vec4(pos, 0.0f, 1.0f);\
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


struct StreamVertexBuffer
{
	GLuint buffer = 0;
	void* mapped = nullptr;
	int size = 0;
};

struct UiObjects {
	GLuint program;
	GLuint vao;
	GLuint debugVertexBuffer;
	StreamVertexBuffer streamBuffer;
}g_ui;


Vertex2D debugVertices[] = {
	{{-1.0f,-1.0f},{0.0f, 0.0f}, 0xFFFFFFFF},
	{{ 1.0f,-1.0f},{1.0f, 0.0f}, 0xFFFFFFFF},
	{{ 1.0f, 1.0f},{1.0f, 1.0f}, 0xFFFFFFFF},

	{{ 1.0f, 1.0f},{1.0f, 1.0f}, 0xFFFFFFFF},
	{{-1.0f, 1.0f},{0.0f, 1.0f}, 0xFFFFFFFF},
	{{-1.0f,-1.0f},{0.0f, 0.0f}, 0xFFFFFFFF},
};

void InitializeUiPipeline()
{
	g_ui.program = CreateProgram(uiVertexShader, uiFragmentShader);
	glGenVertexArrays(1, &g_ui.vao);


	glGenBuffers(1, &g_ui.streamBuffer.buffer);
	glBindBuffer(GL_ARRAY_BUFFER, g_ui.streamBuffer.buffer);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	


	glGenBuffers(1, &g_ui.debugVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, g_ui.debugVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(debugVertices), debugVertices, GL_STATIC_DRAW);

	glBindVertexArray(g_ui.vao);


	glBindBuffer(GL_ARRAY_BUFFER, g_ui.debugVertexBuffer);
	
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(Vertex2D), (const void*)offsetof(Vertex2D,pos));
	glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(Vertex2D), (const void*)offsetof(Vertex2D, texPos));
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, true, sizeof(Vertex2D), (const void*)offsetof(Vertex2D, color));


	glBindVertexArray(0);
}

void DebugDrawTexture(GLuint texture)
{
	glDisable(GL_DEPTH_TEST);

	glUseProgram(g_ui.program);

	glBindVertexArray(g_ui.vao);


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glEnable(GL_DEPTH_TEST);
}