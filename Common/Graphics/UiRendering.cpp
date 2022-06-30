#include "UiRendering.h"
#include "Helper.h"
#include "logging.h"
#include <vector>

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


struct DrawInstruction
{
	GLuint boundTexture;
	int numVerts;
	bool isTriangle;
};
struct StreamVertexBuffer
{
	GLuint vao = 0;
	GLuint buffer = 0;
	unsigned char* mapped = nullptr;
	int writeIdx = 0;
	int size = 0;
	std::vector<DrawInstruction> instructions;
};

struct UiObjects {
	GLuint program;
	StreamVertexBuffer streamBuffer;
	GLuint standardTexture;
}g_ui;


Vertex2D debugVertices[] = {
	{{-1.0f,-1.0f},{0.0f, 0.0f}, 0xFFFFFFFF},
	{{ 1.0f,-1.0f},{1.0f, 0.0f}, 0xFFFFFFFF},
	{{ 1.0f, 1.0f},{1.0f, 1.0f}, 0xFFFFFFFF},

	{{ 1.0f, 1.0f},{1.0f, 1.0f}, 0xFFFFFFFF},
	{{-1.0f, 1.0f},{0.0f, 1.0f}, 0xFFFFFFFF},
	{{-1.0f,-1.0f},{0.0f, 0.0f}, 0xFFFFFFFF},
};



void AddVerticesToBuffer(Vertex2D* verts, int numVerts, GLuint boundTexture, bool areTriangles)
{
	static constexpr int sizeSteps = 1000;
	auto& sb = g_ui.streamBuffer;
	const int addingSize = numVerts * sizeof(Vertex2D);
	if (sb.instructions.empty() || sb.instructions.at(sb.instructions.size() - 1).boundTexture != boundTexture || sb.instructions.at(sb.instructions.size() - 1).numVerts != areTriangles)
	{
		sb.instructions.push_back({ boundTexture, numVerts, areTriangles });
	}
	else
	{
		sb.instructions.at(sb.instructions.size() - 1).numVerts += numVerts;
	}
	if (sb.writeIdx + addingSize >= sb.size)
	{
		const int increaseSize = (addingSize / sizeSteps + 1) * sizeSteps;
		if (sb.mapped)
		{
			uint8_t* newData = new uint8_t[sb.size + increaseSize];
			memcpy(newData, sb.mapped, sb.size);
			memcpy(newData + sb.size, verts, addingSize);
			
			glBindBuffer(GL_ARRAY_BUFFER, sb.buffer);
			glUnmapBuffer(GL_ARRAY_BUFFER);

			sb.size = sb.size + increaseSize;
			glBufferData(GL_ARRAY_BUFFER, sb.size, newData, GL_DYNAMIC_DRAW);
			delete[] newData;
			sb.mapped = (unsigned char*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sb.size, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
			return;
		}
		else
		{
			sb.size = sb.size + increaseSize;
			glBindBuffer(GL_ARRAY_BUFFER, sb.buffer);
			glBufferData(GL_ARRAY_BUFFER, sb.size, nullptr, GL_DYNAMIC_DRAW);
			sb.mapped = (unsigned char*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sb.size, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
		}
	}
	if(!sb.mapped)
	{
		glBindBuffer(GL_ARRAY_BUFFER, sb.buffer);
		sb.mapped = (unsigned char*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sb.size, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
	}

	memcpy(sb.mapped + sb.writeIdx, verts, addingSize);
	sb.writeIdx += addingSize;
}


void InitializeUiPipeline()
{
	g_ui.program = CreateProgram(uiVertexShader, uiFragmentShader);

	glGenVertexArrays(1, &g_ui.streamBuffer.vao);
	glBindVertexArray(g_ui.streamBuffer.vao);

	glGenBuffers(1, &g_ui.streamBuffer.buffer);
	glBindBuffer(GL_ARRAY_BUFFER, g_ui.streamBuffer.buffer);
	
	g_ui.streamBuffer.mapped = nullptr;
	g_ui.streamBuffer.size = 0;
	g_ui.streamBuffer.writeIdx = 0;

	
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(Vertex2D), (const void*)offsetof(Vertex2D,pos));
	glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(Vertex2D), (const void*)offsetof(Vertex2D, texPos));
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, true, sizeof(Vertex2D), (const void*)offsetof(Vertex2D, color));

	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);


	glGenTextures(1, &g_ui.standardTexture);
	glBindTexture(GL_TEXTURE_2D, g_ui.standardTexture);
	uint32_t color[2] = { 0xFFFFFFFF, 0xFFFFFFFF };
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, color);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}



void DrawQuad(const glm::vec2& tl, const glm::vec2& br, uint32_t color)
{
	Vertex2D preQuad[6] = {
		{tl, {0.0f, 0.0f}, color}, {{br.x, tl.y}, {0.0f, 0.0f}, color}, {br, {0.0f, 0.0f}, color},
		{br, {0.0f, 0.0f}, color}, {{tl.x, br.y}, {0.0f, 0.0f}, color}, {tl, {0.0f, 0.0f}, color},
	};
	AddVerticesToBuffer(preQuad, 6, g_ui.standardTexture, true);
}


void DrawUI()
{
	if (g_ui.streamBuffer.instructions.empty()) return;

	if (g_ui.streamBuffer.mapped) {
		glBindBuffer(GL_ARRAY_BUFFER, g_ui.streamBuffer.buffer);
		glUnmapBuffer(GL_ARRAY_BUFFER);
		g_ui.streamBuffer.mapped = nullptr;
	}

	glDisable(GL_DEPTH_TEST);
	glUseProgram(g_ui.program);
	glBindVertexArray(g_ui.streamBuffer.vao);

	
	int curIdx = 0;
	GLuint curBoundTex = g_ui.streamBuffer.instructions.at(0).boundTexture;
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, curBoundTex);
	for (int i = 0; i < g_ui.streamBuffer.instructions.size(); i++)
	{
		auto& cur = g_ui.streamBuffer.instructions.at(i);
		if (cur.boundTexture != curBoundTex)
		{
			curBoundTex = cur.boundTexture;
			glBindTexture(GL_TEXTURE_2D, curBoundTex);
		}
		glDrawArrays(cur.isTriangle ? GL_TRIANGLES : GL_LINES, curIdx, cur.numVerts);
		curIdx += cur.numVerts;
	}


	g_ui.streamBuffer.instructions.clear();
	g_ui.streamBuffer.mapped = nullptr;
	g_ui.streamBuffer.writeIdx = 0;

	glBindVertexArray(0);
	glEnable(GL_DEPTH_TEST);
}