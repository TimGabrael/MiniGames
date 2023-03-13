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
	outCol = texture(tex, tPos).rgba * col;\n\
	outCol = vec4(texture(tex, tPos).rgb, 1.0f) * col;\n\
}\
";


struct DrawInstruction
{
	DrawInstruction(GLuint bTex, int nVerts, bool isTrig) : boundTexture(bTex), numVerts(nVerts), isTriangle(isTrig) {}
	GLuint boundTexture = 0;
	int numVerts = 0;
	bool isTriangle = 0;
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
	GLuint program = 0;
	StreamVertexBuffer streamBuffer;
	GLuint standardTexture = 0;
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
	if (sb.instructions.empty() || sb.instructions.at(sb.instructions.size() - 1).boundTexture != boundTexture || sb.instructions.at(sb.instructions.size() - 1).isTriangle != areTriangles)
	{
		sb.instructions.emplace_back(boundTexture, numVerts, areTriangles);
	}
	else
	{
		sb.instructions.at(sb.instructions.size() - 1).numVerts += numVerts;
	}
	if (sb.writeIdx + addingSize >= sb.size)
	{
		const int increaseSize = (addingSize / sizeSteps + 1) * sizeSteps;
		const int newSize = std::max(sb.size + increaseSize, sb.writeIdx + increaseSize);
		if (sb.mapped)
		{
			uint8_t* newData = new uint8_t[newSize];
			memcpy(newData, sb.mapped, sb.size);
			memcpy(newData + sb.writeIdx, verts, addingSize);
			
			glBindBuffer(GL_ARRAY_BUFFER, sb.buffer);
			glUnmapBuffer(GL_ARRAY_BUFFER);

			sb.size = newSize;
			glBufferData(GL_ARRAY_BUFFER, sb.size, newData, GL_DYNAMIC_DRAW);
			delete[] newData;
			sb.mapped = (unsigned char*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sb.size, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
			return;
		}
		else
		{
			sb.size = newSize;
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
void CleanUpUiPipeline()
{
	if (g_ui.streamBuffer.mapped) {
		glBindBuffer(GL_ARRAY_BUFFER, g_ui.streamBuffer.buffer);
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}

    glDeleteBuffers(1, &g_ui.streamBuffer.buffer);
    glDeleteVertexArrays(1, &g_ui.streamBuffer.vao);
	glDeleteTextures(1, &g_ui.standardTexture);
	glDeleteProgram(g_ui.program);
    g_ui.streamBuffer.instructions.clear();
}


void DrawQuad(const glm::vec2& tl, const glm::vec2& br, uint32_t color)
{
	Vertex2D preQuad[6] = {
		{tl, {0.0f, 0.0f}, color}, {{br.x, tl.y}, {0.0f, 0.0f}, color}, {br, {0.0f, 0.0f}, color},
		{br, {0.0f, 0.0f}, color}, {{tl.x, br.y}, {0.0f, 0.0f}, color}, {tl, {0.0f, 0.0f}, color},
	};
	AddVerticesToBuffer(preQuad, 6, g_ui.standardTexture, true);
}
void DrawQuad(const glm::vec2& tl, const glm::vec2& br, uint32_t color, GLuint texture)
{
	Vertex2D preQuad[6] = {
		{tl, {0.0f, 0.0f}, color}, {{br.x, tl.y}, {1.0f, 0.0f}, color}, {br, {1.0f, 1.0f}, color},
		{br, {1.0f, 1.0f}, color}, {{tl.x, br.y}, {0.0f, 1.0f}, color}, {tl, {0.0f, 0.0f}, color},
	};
	AddVerticesToBuffer(preQuad, 6, texture, true);
}
void DrawCircle(const glm::vec2& center, const glm::vec2& rad, float angleStart, float fillAngle, uint32_t color, int samples)
{
	const float dA = fillAngle / (float)(samples-1);
	Vertex2D* verts = new Vertex2D[samples * 3];

	int curIdx = 0;
	for (int i = 0; i < samples-1; i++)
	{
		const float angle = dA * i + angleStart;
		const float pangle = dA * (i+1) + angleStart;
		
		const glm::vec2 ppos = { center.x + rad.x * sinf(angle), center.y + rad.y * cosf(angle) };
		const glm::vec2 pos = { center.x + rad.x * sinf(pangle), center.y + rad.y * cosf(pangle) };

		verts[curIdx].pos = center;
		verts[curIdx].color = color;
		curIdx++;
		verts[curIdx].pos = pos;
		verts[curIdx].color = color;
		curIdx++;
		verts[curIdx].pos = ppos;
		verts[curIdx].color = color;
		curIdx++;
	}

	AddVerticesToBuffer(verts, 3 * samples, g_ui.standardTexture, true);
	delete[] verts;
}
void DrawLine(const glm::vec2& first, const glm::vec2& second, uint32_t col)
{
	Vertex2D verts[2] = {
		first, {0.0f, 0.0f}, col,
		second, {0.0f, 0.0f}, col,
	};
	AddVerticesToBuffer(verts, 2, g_ui.standardTexture, false);
}
void DrawUI()
{
	if (g_ui.streamBuffer.instructions.empty()) return;

	if (g_ui.streamBuffer.mapped) {
		glBindBuffer(GL_ARRAY_BUFFER, g_ui.streamBuffer.buffer);
		glUnmapBuffer(GL_ARRAY_BUFFER);
		g_ui.streamBuffer.mapped = nullptr;
		g_ui.streamBuffer.writeIdx = 0;
	}
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glUseProgramWrapper(g_ui.program);
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
		glDrawArraysWrapper(cur.isTriangle ? GL_TRIANGLES : GL_LINES, curIdx, cur.numVerts);
		curIdx += cur.numVerts;
	}


	g_ui.streamBuffer.instructions.clear();
	g_ui.streamBuffer.mapped = nullptr;
	g_ui.streamBuffer.writeIdx = 0;

	glBindVertexArray(0);
	glEnable(GL_DEPTH_TEST);
}

