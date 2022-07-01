#include "Card.h"
#include "Graphics/GLCompat.h"
#include "Graphics/Helper.h"
#include <glm/glm.hpp>
#include "FileQuery.h"
#include "logging.h"
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>

// this is related to the UNO_DECK.png
static constexpr float CARD_STEP_SIZEX = 1.0f / 14.0f;
static constexpr float CARD_STEP_SIZEY = 1.0f / 4.0f;

static const char* cardVertexShader = "#version 300 es\n\
\n\
layout(location = 0) in vec2 pos;\n\
layout(location = 1) in vec2 texPos;\n\
layout(location = 2) in mat4 instanceMatrix;\n\
layout(location = 6) in vec2 texOffset;\n\
uniform mat4 projection;\n\
uniform mat4 view;\n\
out vec3 outNormal;\
out vec2 tPos;\
void main(){\
	gl_Position = projection * view * instanceMatrix * vec4(pos, 0.0f, 1.0f);\
	tPos = texPos + texOffset;\
	outNormal = (instanceMatrix * vec4(0.0f, 0.0f, 1.0f, 0.0f)).xyz;\
}\
";

static const char* cardFragmentShader = "#version 300 es\n\
precision highp float;\n\
\n\
in vec3 outNormal;\
in vec2 tPos;\
uniform sampler2D tex;\
out vec4 outCol;\
const vec3 lPos = vec3(0.0f, 1.0f, 2.0f);\
const vec3 lDir = vec3(0.0f, -1.0f, 0.0f);\
const vec3 lDif = vec3(0.2f, 0.2f, 0.2f);\
const vec3 fSpe = vec3(0.8f, 0.8f, 0.8f);\
void main(){\
	vec4 c = texture(tex, tPos);\n\
	vec3 norm = normalize(outNormal);\
	float diff = min(max(dot(norm, lDir), 0.0), 0.8);\
	outCol = vec4((diff+lDif) * c.xyz, c.w);\n\
}\
";


glm::vec2 CardIDToTextureIndex(CARD_ID id)
{
	const int xPos = id % 14;
	const int yPos = id / 14;
	return { CARD_STEP_SIZEX * xPos, CARD_STEP_SIZEY * yPos };
}

struct CardVertex
{
	glm::vec2 pos;
	glm::vec2 tex;
};
struct InstanceData
{
	glm::mat4 mat;
	glm::vec2 tOff;
};

struct CardBuffers
{
	GLuint vao;
	GLuint vertexBuffer;	// single card
	GLuint streamBuffer;
	int numInstances = 0;
	int avSize = 0;

	unsigned char* mapped = nullptr;
};
struct CardUniforms
{
	GLuint projection;
	GLuint view;
	GLuint tex;
};

struct CardPipeline
{
	GLuint program;
	GLuint texture;
	
	CardBuffers bufs;
	CardUniforms unis;

	float width_half = 0.0f;
	float height_half = 0.0f;

}g_cards;


void InitializeCardPipeline(void* assetManager)
{
	g_cards.program = CreateProgram(cardVertexShader, cardFragmentShader);
	g_cards.unis.projection = glGetUniformLocation(g_cards.program, "projection");
	g_cards.unis.view = glGetUniformLocation(g_cards.program, "view");
	g_cards.unis.tex = glGetUniformLocation(g_cards.program, "tex");

	// load texture
	glGenTextures(1, &g_cards.texture);
	FileContent content = LoadFileContent(assetManager, "Assets/UNO_DECK.png");
	float cardHeight = 0.5f;
	if (content.data)
	{
		glBindTexture(GL_TEXTURE_2D, g_cards.texture);
		int width, height;
		int channelsInFile;
		stbi_uc* imgData = stbi_load_from_memory(content.data, content.size, &width, &height, &channelsInFile, 4);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgData);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		cardHeight = (height * CARD_STEP_SIZEY / (width * CARD_STEP_SIZEX)) / 2.0f;



		stbi_image_free(imgData);
		delete[] content.data;
	}
	// 


	glGenVertexArrays(1, &g_cards.bufs.vao);
	glBindVertexArray(g_cards.bufs.vao);

	glGenBuffers(1, &g_cards.bufs.vertexBuffer);
	glGenBuffers(1, &g_cards.bufs.streamBuffer);

	
	glBindBuffer(GL_ARRAY_BUFFER, g_cards.bufs.vertexBuffer);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CardVertex), nullptr);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(CardVertex), (void*)(offsetof(CardVertex, tex)));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	g_cards.width_half = 0.5f;
	g_cards.height_half = cardHeight;
	CardVertex vert[6] = {
		{{-0.5f, -cardHeight}, {CARD_STEP_SIZEX, CARD_STEP_SIZEY}},	// br
		{{-0.5f, cardHeight}, {CARD_STEP_SIZEX, 0.0f}},				// tr
		{{0.5f, cardHeight}, {0.0f, 0.0f}},							// tl
		{{0.5f, cardHeight}, {0.0f, 0.0f}},							// tl
		{{0.5f, -cardHeight}, {0.0f, CARD_STEP_SIZEY}},				// bl
		{{-0.5f, -cardHeight}, {CARD_STEP_SIZEX, CARD_STEP_SIZEY}},	// br
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vert), vert, GL_STATIC_DRAW);

	
	glBindBuffer(GL_ARRAY_BUFFER, g_cards.bufs.streamBuffer);
	InstanceData data;
	data.mat = glm::mat4(1.0f);
	data.tOff = { 0.0f, 0.0f };
	glBufferData(GL_ARRAY_BUFFER, sizeof(InstanceData), &data, GL_DYNAMIC_DRAW);

	static constexpr size_t vec4Size = sizeof(glm::vec4);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), nullptr);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(1 * vec4Size));
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(2 * vec4Size));
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(3 * vec4Size));
	glVertexAttribPointer(6, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(offsetof(InstanceData, tOff)));
	
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);
	glEnableVertexAttribArray(5);
	glEnableVertexAttribArray(6);

	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);
	glVertexAttribDivisor(6, 1);
	







	glBindVertexArray(0);
}

void AddCard(CARD_ID back, CARD_ID front, const glm::mat4& transform)
{
	static constexpr int numReserved = 100;

	auto& b = g_cards.bufs;
	b.numInstances+=2;

	InstanceData data[2];
	data[0].mat = transform * glm::rotate(glm::mat4(1.0f), 3.14159f, glm::vec3(0.0f, 1.0f, 0.0f));
	data[0].tOff = CardIDToTextureIndex(front);
	data[1].mat = transform * glm::mat4(1.0f);
	data[1].tOff = CardIDToTextureIndex(back);

	if (b.avSize <= b.numInstances)
	{
		b.avSize = (b.avSize + b.numInstances + numReserved);
		if (b.mapped)
		{
			uint8_t* newData = new uint8_t[b.avSize * sizeof(InstanceData)];
			memcpy(newData, b.mapped, (b.numInstances-2) * sizeof(InstanceData));
			memcpy(newData + (b.numInstances - 2) * sizeof(InstanceData), &data, sizeof(InstanceData) * 2);

			glBindBuffer(GL_ARRAY_BUFFER, b.streamBuffer);
			glUnmapBuffer(GL_ARRAY_BUFFER);

			glBufferData(GL_ARRAY_BUFFER, b.avSize * sizeof(InstanceData), newData, GL_DYNAMIC_DRAW);

			delete[] newData;
			b.mapped = (unsigned char*)glMapBufferRange(GL_ARRAY_BUFFER, 0, b.avSize * sizeof(InstanceData), GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
			return;
		}
		else
		{
			glBindBuffer(GL_ARRAY_BUFFER, b.streamBuffer);
			glBufferData(GL_ARRAY_BUFFER, b.avSize * sizeof(InstanceData), nullptr, GL_DYNAMIC_DRAW);
			b.mapped = (unsigned char*)glMapBufferRange(GL_ARRAY_BUFFER, 0, b.avSize * sizeof(InstanceData), GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
		}
	}
	if (!b.mapped)
	{
		glBindBuffer(GL_ARRAY_BUFFER, b.streamBuffer);
		b.mapped = (unsigned char*)glMapBufferRange(GL_ARRAY_BUFFER, 0, b.avSize * sizeof(InstanceData), GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
	}
	memcpy(b.mapped + (b.numInstances-2) * sizeof(InstanceData), &data, sizeof(InstanceData) * 2);
}
void ClearCards()
{
	g_cards.bufs.numInstances = 0;
}


void DrawCards(const glm::mat4& proj, const glm::mat4& view)
{
	if (g_cards.bufs.mapped) {
		glBindBuffer(GL_ARRAY_BUFFER, g_cards.bufs.streamBuffer);
		glUnmapBuffer(GL_ARRAY_BUFFER);
		g_cards.bufs.mapped = nullptr;
	}
	glUseProgram(g_cards.program);
	glBindVertexArray(g_cards.bufs.vao);
	glUniformMatrix4fv(g_cards.unis.projection, 1, GL_FALSE, (const GLfloat*)&proj);
	glUniformMatrix4fv(g_cards.unis.view, 1, GL_FALSE, (const GLfloat*)&view);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_cards.texture);


	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, g_cards.bufs.numInstances);

	glBindVertexArray(0);
}

bool HitTest(const glm::mat4& model, const glm::vec3& camPos, const glm::vec3& mouseRay)
{
	glm::vec3 normal = model * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
	glm::vec3 tangent1 = model * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 tangent2 = model * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
	glm::vec3 center = model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	
	float rayDot = glm::dot(mouseRay, normal);
	if (rayDot == 0.0f) return false;	// the ray is parallel to the plane
	float interSectionTime = glm::dot((center - camPos), normal) / rayDot;
	if (interSectionTime < 0.0f) return false;
	
	glm::vec3 planePos = camPos + mouseRay * interSectionTime;

	float dx = glm::dot(tangent1, planePos);
	float dy = glm::dot(tangent2, planePos);
	if (-g_cards.width_half < dx && dx < g_cards.width_half)
	{
		if (-g_cards.height_half < dy && dy < g_cards.height_half)
		{
			return true;
		}
	}
	return false;
}