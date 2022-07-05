#include "Card.h"
#include "Graphics/GLCompat.h"
#include "Graphics/Helper.h"
#include <glm/glm.hpp>
#include "FileQuery.h"
#include "logging.h"
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include "Graphics/UiRendering.h"

// this is related to the UNO_DECK.png

static constexpr float IMAGE_SIZE_X = 1681.0f;
static constexpr float IMAGE_SIZE_Y = 944.0f;

static constexpr float CARD_SIZE_X = 120.0f;
static constexpr float CARD_SIZE_Y = 180.0f;

static constexpr float CARD_HEIGHT_HALF = 180.0f / 120.0f * 0.5f;


static constexpr float CARD_EFFECT_START_Y = 724.0f;
static constexpr float CARD_EFFECT_X = 160.0f;
static constexpr float CARD_EFFECT_Y = 219.0f;

static constexpr float CARD_STEP_SIZEX = CARD_SIZE_X / IMAGE_SIZE_X;
static constexpr float CARD_STEP_SIZEY = CARD_SIZE_Y / IMAGE_SIZE_Y;

static constexpr float CARD_EFFECT_SIZEX = CARD_EFFECT_X / IMAGE_SIZE_X;
static constexpr float CARD_EFFECT_SIZEY = CARD_EFFECT_Y / IMAGE_SIZE_Y;


static const char* cardVertexShader = "#version 300 es\n\
\n\
layout(location = 0) in vec2 pos;\n\
layout(location = 1) in vec2 texPos;\n\
layout(location = 2) in mat4 instanceMatrix;\n\
layout(location = 6) in vec2 texOffset;\n\
layout(location = 7) in vec2 texSize;\n\
layout(location = 8) in vec4 addColor;\n\
uniform mat4 projection;\n\
uniform mat4 view;\n\
out vec3 outNormal;\
out vec2 tPos;\
out vec4 addCol;\
void main(){\
	gl_Position = projection * view * instanceMatrix * vec4(pos, 0.0f, 1.0f);\
	tPos = texPos * texSize + texOffset;\
	outNormal = (instanceMatrix * vec4(0.0f, 0.0f, 1.0f, 0.0f)).xyz;\
	addCol = addColor;\
}\
";

static const char* cardFragmentShader = "#version 300 es\n\
precision highp float;\n\
\n\
in vec3 outNormal;\
in vec2 tPos;\
in vec4 addCol;\
uniform sampler2D tex;\
out vec4 outCol;\
const vec3 lPos = vec3(0.0f, 1.0f, 2.0f);\
const vec3 lDir = vec3(0.0f, -1.0f, 0.0f);\
const vec3 lDif = vec3(0.2f, 0.2f, 0.2f);\
const vec3 fSpe = vec3(0.8f, 0.8f, 0.8f);\
void main(){\
	vec4 c = texture(tex, tPos) * addCol;\n\
	vec3 norm = normalize(outNormal);\
	float diff = min(max(dot(norm, lDir), 0.0), 0.8);\
	outCol = vec4((diff+lDif) * c.rgb, c.a);\n\
}\
";


glm::vec2 CardIDToTextureIndex(CARD_ID id)
{
	const int xPos = id % 14;
	const int yPos = id / 14;
	return { CARD_STEP_SIZEX * xPos, CARD_STEP_SIZEY * yPos };
}
glm::vec2 CardEffectToTextureIndex(CARD_EFFECT eff)
{
	const int xPos = eff % 1;

	return { CARD_EFFECT_SIZEX * xPos, CARD_EFFECT_START_Y / IMAGE_SIZE_Y };
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
	glm::vec2 tSz;
	uint32_t col;
};
static constexpr size_t instSize = sizeof(InstanceData);

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

		cardHeight = CARD_HEIGHT_HALF;



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
	g_cards.height_half = CARD_HEIGHT_HALF;
	CardVertex vert[6] = {
		{{-0.5f, -cardHeight}, {1.0f, 1.0f}},	// br
		{{-0.5f, cardHeight}, {1.0f, 0.0f}},	// tr
		{{0.5f, cardHeight}, {0.0f, 0.0f}},		// tl
		{{0.5f, cardHeight}, {0.0f, 0.0f}},		// tl
		{{0.5f, -cardHeight}, {0.0f, 1.0f}},	// bl
		{{-0.5f, -cardHeight}, {1.0f, 1.0f}},	// br
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
	glVertexAttribPointer(7, 2, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(offsetof(InstanceData, tSz)));
	glVertexAttribPointer(8, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(InstanceData), (void*)(offsetof(InstanceData, col)));
	
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);
	glEnableVertexAttribArray(5);
	glEnableVertexAttribArray(6);
	glEnableVertexAttribArray(7);
	glEnableVertexAttribArray(8);

	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);
	glVertexAttribDivisor(6, 1);
	glVertexAttribDivisor(7, 1);
	glVertexAttribDivisor(8, 1);
	







	glBindVertexArray(0);
}

void RendererAddCard(CARD_ID back, CARD_ID front, const glm::mat4& transform)
{
	static constexpr int numReserved = 100;

	auto& b = g_cards.bufs;
	b.numInstances+=2;

	InstanceData data[2];
	data[0].mat = transform * glm::rotate(glm::mat4(1.0f), 3.14159f, glm::vec3(0.0f, 1.0f, 0.0f));
	data[0].tOff = CardIDToTextureIndex(front);
	data[0].tSz = glm::vec2(CARD_STEP_SIZEX, CARD_STEP_SIZEY);
	data[0].col = 0xFFFFFFFF;
	data[1].mat = transform * glm::mat4(1.0f);
	data[1].tOff = CardIDToTextureIndex(back);
	data[1].tSz = glm::vec2(CARD_STEP_SIZEX, CARD_STEP_SIZEY);
	data[1].col = 0xFFFFFFFF;

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
void RendererAddEffect(CARD_EFFECT effect, const glm::mat4& transform, uint32_t col)
{
	static constexpr int numReserved = 100;

	auto& b = g_cards.bufs;
	b.numInstances += 2;


	const glm::vec2 effPos = CardEffectToTextureIndex(effect);

	InstanceData data[2];
	data[0].mat = transform * glm::rotate(glm::mat4(1.0f), 3.14159f, glm::vec3(0.0f, 1.0f, 0.0f));
	data[0].tOff = effPos;
	data[0].tSz = glm::vec2(CARD_EFFECT_SIZEX, CARD_EFFECT_SIZEY);
	data[0].col = col;
	data[1].mat = transform;
	data[1].tOff = effPos;
	data[1].tSz = data[0].tSz;
	data[1].col = col;


	if (b.avSize <= b.numInstances)
	{
		b.avSize = (b.avSize + b.numInstances + numReserved);
		if (b.mapped)
		{
			uint8_t* newData = new uint8_t[b.avSize * sizeof(InstanceData)];
			memcpy(newData, b.mapped, (b.numInstances - 2) * sizeof(InstanceData));
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
	memcpy(b.mapped + (b.numInstances - 2) * sizeof(InstanceData), &data, sizeof(InstanceData) * 2);
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
	glm::vec3 normal = glm::normalize(model * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
	glm::vec3 tangent1 = model * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 tangent2 = model * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

	float scaledWidth_half = sqrtf(glm::dot(tangent1, tangent1)) * g_cards.width_half;
	float scaledHeight_half = sqrtf(glm::dot(tangent2, tangent2)) * g_cards.height_half;
	tangent1 = glm::normalize(tangent1);
	tangent2 = glm::normalize(tangent2);

	glm::vec3 center = model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);


	float rayDot = glm::dot(mouseRay, normal);
	if (rayDot == 0.0f) return false;	// the ray is parallel to the plane
	float interSectionTime = glm::dot((center - camPos), normal) / rayDot;
	if (interSectionTime < 0.0f) return false;
	
	glm::vec3 planePos = camPos + mouseRay * interSectionTime;

	float dx = glm::dot(tangent1, planePos) - glm::dot(tangent1, center);
	float dy = glm::dot(tangent2, planePos) - glm::dot(tangent2, center);

	

	if (-scaledWidth_half < dx && dx < scaledWidth_half)
	{
		if (-scaledHeight_half < dy && dy < scaledHeight_half)
		{
			return true;
		}
	}
	return false;
}

// use exact order of webiste
std::vector<CARD_ID> GenerateDeck()
{
	return {
		CARD_ID::CARD_ID_RED_0, CARD_ID::CARD_ID_RED_1, CARD_ID::CARD_ID_RED_2, CARD_ID::CARD_ID_RED_3, CARD_ID::CARD_ID_RED_4, CARD_ID::CARD_ID_RED_5, CARD_ID::CARD_ID_RED_6, 
		CARD_ID::CARD_ID_RED_7, CARD_ID::CARD_ID_RED_8, CARD_ID::CARD_ID_RED_9, CARD_ID::CARD_ID_RED_PAUSE, CARD_ID::CARD_ID_RED_SWAP, CARD_ID::CARD_ID_RED_ADD, CARD_ID::CARD_ID_CHOOSE_COLOR,

		CARD_ID::CARD_ID_YELLOW_0, CARD_ID::CARD_ID_YELLOW_1, CARD_ID::CARD_ID_YELLOW_2, CARD_ID::CARD_ID_YELLOW_3, CARD_ID::CARD_ID_YELLOW_4, CARD_ID::CARD_ID_YELLOW_5, CARD_ID::CARD_ID_YELLOW_6,
		CARD_ID::CARD_ID_YELLOW_7, CARD_ID::CARD_ID_YELLOW_8, CARD_ID::CARD_ID_YELLOW_9, CARD_ID::CARD_ID_YELLOW_PAUSE, CARD_ID::CARD_ID_YELLOW_SWAP, CARD_ID::CARD_ID_YELLOW_ADD, CARD_ID::CARD_ID_CHOOSE_COLOR,

		CARD_ID::CARD_ID_BLUE_0, CARD_ID::CARD_ID_BLUE_1, CARD_ID::CARD_ID_BLUE_2, CARD_ID::CARD_ID_BLUE_3, CARD_ID::CARD_ID_BLUE_4, CARD_ID::CARD_ID_BLUE_5, CARD_ID::CARD_ID_BLUE_6,
		CARD_ID::CARD_ID_BLUE_7, CARD_ID::CARD_ID_BLUE_8, CARD_ID::CARD_ID_BLUE_9, CARD_ID::CARD_ID_BLUE_PAUSE, CARD_ID::CARD_ID_BLUE_SWAP, CARD_ID::CARD_ID_BLUE_ADD, CARD_ID::CARD_ID_CHOOSE_COLOR,

		CARD_ID::CARD_ID_GREEN_0, CARD_ID::CARD_ID_GREEN_1, CARD_ID::CARD_ID_GREEN_2, CARD_ID::CARD_ID_GREEN_3, CARD_ID::CARD_ID_GREEN_4, CARD_ID::CARD_ID_GREEN_5, CARD_ID::CARD_ID_GREEN_6,
		CARD_ID::CARD_ID_GREEN_7, CARD_ID::CARD_ID_GREEN_8, CARD_ID::CARD_ID_GREEN_9, CARD_ID::CARD_ID_GREEN_PAUSE, CARD_ID::CARD_ID_GREEN_SWAP, CARD_ID::CARD_ID_GREEN_ADD, CARD_ID::CARD_ID_CHOOSE_COLOR,


		CARD_ID::CARD_ID_RED_1, CARD_ID::CARD_ID_RED_2, CARD_ID::CARD_ID_RED_3, CARD_ID::CARD_ID_RED_4, CARD_ID::CARD_ID_RED_5, CARD_ID::CARD_ID_RED_6,
		CARD_ID::CARD_ID_RED_7, CARD_ID::CARD_ID_RED_8, CARD_ID::CARD_ID_RED_9, CARD_ID::CARD_ID_RED_PAUSE, CARD_ID::CARD_ID_RED_SWAP, CARD_ID::CARD_ID_RED_ADD, CARD_ID::CARD_ID_ADD_4,

		CARD_ID::CARD_ID_YELLOW_1, CARD_ID::CARD_ID_YELLOW_2, CARD_ID::CARD_ID_YELLOW_3, CARD_ID::CARD_ID_YELLOW_4, CARD_ID::CARD_ID_YELLOW_5, CARD_ID::CARD_ID_YELLOW_6,
		CARD_ID::CARD_ID_YELLOW_7, CARD_ID::CARD_ID_YELLOW_8, CARD_ID::CARD_ID_YELLOW_9, CARD_ID::CARD_ID_YELLOW_PAUSE, CARD_ID::CARD_ID_YELLOW_SWAP, CARD_ID::CARD_ID_YELLOW_ADD, CARD_ID::CARD_ID_ADD_4,

		CARD_ID::CARD_ID_BLUE_1, CARD_ID::CARD_ID_BLUE_2, CARD_ID::CARD_ID_BLUE_3, CARD_ID::CARD_ID_BLUE_4, CARD_ID::CARD_ID_BLUE_5, CARD_ID::CARD_ID_BLUE_6,
		CARD_ID::CARD_ID_BLUE_7, CARD_ID::CARD_ID_BLUE_8, CARD_ID::CARD_ID_BLUE_9, CARD_ID::CARD_ID_BLUE_PAUSE, CARD_ID::CARD_ID_BLUE_SWAP, CARD_ID::CARD_ID_BLUE_ADD, CARD_ID::CARD_ID_ADD_4,

		CARD_ID::CARD_ID_GREEN_1, CARD_ID::CARD_ID_GREEN_2, CARD_ID::CARD_ID_GREEN_3, CARD_ID::CARD_ID_GREEN_4, CARD_ID::CARD_ID_GREEN_5, CARD_ID::CARD_ID_GREEN_6,
		CARD_ID::CARD_ID_GREEN_7, CARD_ID::CARD_ID_GREEN_8, CARD_ID::CARD_ID_GREEN_9, CARD_ID::CARD_ID_GREEN_PAUSE, CARD_ID::CARD_ID_GREEN_SWAP, CARD_ID::CARD_ID_GREEN_ADD, CARD_ID::CARD_ID_ADD_4,
	};
}

bool CardSort(CARD_ID low, CARD_ID high)
{
	return low < high;
}


bool CardIsPlayable(CARD_ID topCard, CARD_ID playing, CARD_ID colorRefrenceCard)
{
	if (topCard == CARD_ID::CARD_ID_BLANK || topCard == CARD_ID::CARD_ID_BLANK2) return true;

	int colorIDTop = topCard / 14;
	int colorIDPlay = playing / 14;
	int colorIDRef = colorRefrenceCard / 14;

	int numIDTop = topCard % 14;
	int numIDPlay = playing % 14;

	if (topCard == CARD_ID::CARD_ID_ADD_4 || topCard == CARD_ID::CARD_ID_CHOOSE_COLOR) {
		if (colorIDRef == colorIDPlay) return true;
		return false;
	}
	else if (playing == CARD_ID::CARD_ID_ADD_4 || playing == CARD_ID::CARD_ID_CHOOSE_COLOR)
	{
		return true;
	}

	return ((colorIDTop == colorIDPlay) || (numIDTop == numIDPlay));
}


uint32_t GetCardColorFromID(CARD_ID id)
{
	const int colorID = id / 14;
	if (colorID == 0) return 0xFF0000FF;
	else if (colorID == 1) return 0xFF00FFFF;
	else if (colorID == 2) return 0xFF00FF00;
	else if (colorID == 3) return 0xFFFF0000;
	return 0xFFFFFFFF;
}
































CARD_ID ColorPicker::GetSelected(int mx, int my, int screenX, int screenY, bool pressed, bool released)
{
	const float mX = 2.0f * ((float)mx / (float)screenX) - 1.0f;
	const float mY = 1.0f - 2.0f * ((float)my / (float)screenY);
	const float aspectRatio = (float)screenX / (float)screenY;

	const float oa = aspectRatio * o;
	const float r_2 = (r + o) * (r + o);
	const float ra_2 = aspectRatio * aspectRatio * r_2;


	const float ellipseHitEq = mX * mX / r_2 + mY * mY / ra_2;

	CARD_ID output = CARD_ID_BLANK;

	if (ellipseHitEq < 1.0f)
	{
		if (mX > 0.0f && mY > 0.0f)			// region red
		{
			this->hoveredColor = 0;
		}
		else if (mX > 0.0f && mY < 0.0f)	// region yellow
		{
			this->hoveredColor = 1;

		}
		else if (mX < 0.0f && mY < 0.0f)	// region green
		{
			this->hoveredColor = 2;
		}
		else								// region blue 
		{
			this->hoveredColor = 3;
		}
		if (pressed) pressedColor = hoveredColor;
		if (released && pressedColor != -1 && pressedColor == hoveredColor) {
			if (pressedColor == 0) output = CARD_ID_RED_2;
			else if (pressedColor == 1) output = CARD_ID_YELLOW_2;
			else if (pressedColor == 2) output = CARD_ID_GREEN_2;
			else if (pressedColor == 3) output = CARD_ID_BLUE_2;
		}
		isCurrentlyHovered = true;
	}
	else
	{
		if (pressed) pressedColor = -1;
		isCurrentlyHovered = false;
	}

	return output;
}
void ColorPicker::Draw(float aspectRatio, float dt)
{
	static constexpr float hoverAnimDuration = 0.4f;
	static constexpr int numSamples = 16;

	const float hoa = aspectRatio * ho;
	const float oa = aspectRatio * o;
	const float ra = aspectRatio * r;
	const float hra = aspectRatio * hr;

	const uint32_t alpha = 0x60;
	const uint32_t hAlpha = 0xB0;

	if (isCurrentlyHovered) hoverTimer = std::min(hoverTimer + dt, hoverAnimDuration);
	else hoverTimer = std::max(hoverTimer - dt, 0.0f);


	const float interpol = hoverTimer / hoverAnimDuration;
	const uint32_t ia = InterpolateInteger(alpha, hAlpha, interpol) << 24;
	const float io = InterpolateFloat(o, ho, interpol);
	const float ioa = InterpolateFloat(oa, hoa, interpol);
	const float ir = InterpolateFloat(r, hr, interpol);
	const float ira = InterpolateFloat(ra, hra, interpol);


	if(hoveredColor == 0) DrawCircle({ io, ioa }, { ir, ira }, glm::radians(0.0f), glm::radians(90.0f), ia | 0x0000FF, numSamples);			// red
	else DrawCircle({ o, oa}, { r, ra },		glm::radians(0.0f),   glm::radians(90.0f), 0x600000FF, numSamples);							// red

	if(hoveredColor == 1) DrawCircle({ io, -ioa }, { ir, ira }, glm::radians(90.0f), glm::radians(90.0f), ia | 0x00FFFF, numSamples);		// yellow
	else DrawCircle({ o, -oa}, { r, ra },	glm::radians(90.0f),  glm::radians(90.0f), 0x6000FFFF, numSamples);								// yellow

	if(hoveredColor == 2) DrawCircle({ -io, -ioa }, { ir, ira }, glm::radians(180.0f), glm::radians(90.0f), ia | 0x00FF00, numSamples);		// green
	else DrawCircle({ -o, -oa}, { r, ra },	glm::radians(180.0f), glm::radians(90.0f), 0x6000FF00, numSamples);								// green

	if(hoveredColor == 3) DrawCircle({ -io, ioa }, { ir, ira }, glm::radians(270.0f), glm::radians(90.0f), ia | 0xFF0000, numSamples);		// blue
	else DrawCircle({ -o, oa}, { r, ra },	glm::radians(270.0f), glm::radians(90.0f), 0x60FF0000, numSamples);								// blue
}







void CardDeck::Draw()
{
	glm::mat4 baseTransform = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(g_cardScale, g_cardScale, g_cardScale));
	for (int i = 0; i < numCards; i++)
	{
		glm::mat4 mat = glm::translate(glm::mat4(1.0f), glm::vec3(dx, i * dy, 0)) * baseTransform;
		RendererAddCard(CARD_ID_BLANK, CARD_ID_BLANK, mat);
	}
}
CARD_ID CardDeck::PullCard()
{
	if (deck.empty()) deck = GenerateDeck();
	int randIdx = rand() % deck.size();
	CARD_ID res = deck.at(randIdx);
	deck.erase(deck.begin() + randIdx);
	return res;
}

void CardStack::Draw()
{
	for (int i = 0; i < cards.size(); i++)
	{
		auto& c = cards.at(i);
		if ((i == cards.size() - 1) && (c.front == CARD_ID::CARD_ID_ADD_4 || c.front == CARD_ID::CARD_ID_CHOOSE_COLOR) && (blackColorID != CARD_ID::CARD_ID_BLANK && blackColorID))
		{
			if (countDown)
			{
				topAnim = std::max(topAnim - 0.02f, 0.0f);
				if (topAnim == 0.0f) countDown = false;
			}
			else
			{
				topAnim = std::min(topAnim + 0.02f, 1.0f);
				if (topAnim == 1.0f) countDown = true;
			}
			const float s = 1.3f + 0.4f * topAnim;
			const uint32_t col = GetCardColorFromID(this->blackColorID);
			RendererAddEffect(CARD_EFFECT_BLUR, c.transform * glm::scale(glm::mat4(1.0f), glm::vec3(s, s-0.1f, s)), col);
		}
		RendererAddCard(c.back, c.front, c.transform);
	}
}
void CardStack::AddToStack(CARD_ID card, const glm::mat4& mat)
{
	if (cards.size() > 10)
	{
		for (int i = 0; i < cards.size() - 1; i++)
		{
			cards.at(i) = cards.at(i + 1);
		}
		auto& ad = cards.at(cards.size() - 1);
		ad.front = card;
		ad.hoverCounter = 0.0f; ad.transform = mat; ad.visible = true;
	}
	else
	{
		cards.emplace_back(CARD_ID::CARD_ID_BLANK, card, mat, 0.0f, true);
	}
}
CARD_ID CardStack::GetTop(CARD_ID& blackColorRef) const
{
	if (cards.empty()) return CARD_ID_BLANK;

	CARD_ID top = cards.at(cards.size() - 1).front;
	if (top == CARD_ID::CARD_ID_ADD_4 || top == CARD_ID::CARD_ID_CHOOSE_COLOR) blackColorRef = blackColorID;
	return top;
}



void CardHand::Add(CARD_ID id)
{
	if (wideCardsStart >= 0.0f) wideCardsStart += g_cardMinDiff * 0.5f;

	cards.emplace_back(CARD_ID::CARD_ID_BLANK, id, glm::mat4(1.0f), 0.0f, true);

	std::sort(cards.begin(), cards.end(), [](CardInfo& i1, CardInfo& i2) {
		return CardSort(i1.front, i2.front);
		});

	needRegen = true;
}
int CardHand::AddTemp(const Camera& cam, CARD_ID id)
{
	if (wideCardsStart >= 0.0f) wideCardsStart += g_cardMinDiff * 0.5f;

	cards.emplace_back(CARD_ID::CARD_ID_BLANK, id, glm::mat4(1.0f), -1.0f, false);
	std::sort(cards.begin(), cards.end(), [](CardInfo& i1, CardInfo& i2) {
		return CardSort(i1.front, i2.front);
		});
	int idx = 0;
	for (int i = 0; i < cards.size(); i++)
	{
		if (cards.at(i).hoverCounter < 0.0f)
		{
			idx = i;
			cards.at(i).hoverCounter = 0.0f;
			cards.at(i).transitionID = currentAssignedAnimID;
			currentAssignedAnimID++;
			break;
		}
	}
	needRegen = true;
	GenTransformations(cam);
	return idx;
}
void CardHand::PlayCard(const CardStack& stack, CardsInAnimation& anim, int cardIdx)
{
	CARD_ID blackColRef;
	CARD_ID top = stack.GetTop(blackColRef);

	if (CardIsPlayable(top, cards.at(cardIdx).front, blackColRef))
	{
		auto& c = cards.at(cardIdx);
		anim.AddAnim(stack, c, handID, CARD_ANIMATIONS::ANIM_PLAY_CARD);
		
		if (c.front == CARD_ID::CARD_ID_ADD_4 || c.front == CARD_ID::CARD_ID_CHOOSE_COLOR)
		{
			this->choosingCardColor = true;
		}

		cards.erase(cards.begin() + cardIdx);
	}
	else
	{
		//LOG("CAN'T PLAY CARD\n");
	}
}
void CardHand::FetchCard(const Camera& cam, const CardStack& stack, CardDeck& deck, CardsInAnimation& anim)
{
	CARD_ID card = deck.PullCard();
	int idx = AddTemp(cam, card);
	anim.AddAnim(stack, cards.at(idx), handID, CARD_ANIMATIONS::ANIM_FETCH_CARD);
}
void CardHand::Update(CardStack& stack, CardsInAnimation& anim, ColorPicker& picker, const Camera& cam, const Pointer& p, bool allowInput)
{
	needRegen = true;
	if (needRegen) GenTransformations(cam);
	glm::vec3 cp = cam.pos;
	glm::vec3 mRay = cam.mouseRay;

	
	if (mouseAttached) {
		static constexpr float pixelStepSize = 0.01f;
		wideCardsStart += pixelStepSize * p.dx;
		if (abs(p.dx) != 0)
		{
			mouseSelectedCard = -1;
		}
	}
	int oldMouseSelectedCard = mouseSelectedCard;
	bool wasReleased = false;
	if (p.Released()) {
		wasReleased = true;
		mouseAttached = false;
		mouseSelectedCard = -1;
	}
	int hitIdx = -1;
	for (int i = cards.size() - 1; i >= 0; i--)
	{
		auto& c = cards.at(i);
		if (hitIdx == -1 && HitTest(c.transform, cp, mRay))
		{
			hitIdx = i;
			highlightedCardIdx = hitIdx;
			if (p.Pressed()) {
				mouseSelectedCard = hitIdx;
				mouseAttached = true;
			}
			break;
		}
	}

	if (choosingCardColor)
	{
		CARD_ID id = picker.GetSelected(p.x, p.y, cam.screenX, cam.screenY, p.Pressed(), p.Released());
		if (id != CARD_ID_BLANK)
		{
			choosenCardColor = id;
			stack.blackColorID = id;
			// update the game state to go to the next player here!
			choosingCardColor = false;
		}
	}
	else if (allowInput)
	{
		if (wasReleased && oldMouseSelectedCard == hitIdx && hitIdx != -1)
		{
			PlayCard(stack, anim, oldMouseSelectedCard);
		}
	}

	if (highlightedCardIdx != -1)
	{
		for (int i = 0; i < cards.size(); i++)
		{
			auto& c = cards.at(i);
			if (i == highlightedCardIdx)
			{
				c.hoverCounter = std::min(c.hoverCounter + 0.1f, 1.0f);
			}
			else
			{
				c.hoverCounter = std::max(c.hoverCounter - 0.1f, 0.0f);
			}
		}
	}

}
void CardHand::GenTransformations(const Camera& cam)
{
	static constexpr float maxDiff = 0.8f;
	if (!needRegen || cards.empty()) return;
	const glm::vec2 frustrum = cam.GetFrustrumSquare(1.0f);
	maxWidth = frustrum.x * 1.2f;


	glm::vec3 camUp = cam.GetRealUp();
	glm::vec3 baseTranslate = cam.pos + cam.GetFront() + camUp * -1.f;
	glm::vec3 right = cam.GetRight();
	float camYaw = cam.GetYaw() + 90.0f;
	glm::mat4 frontFaceingRotation = glm::rotate(glm::mat4(1.0f), glm::radians(camYaw), glm::vec3(0.0f, -1.0f, 0.0f));
	glm::mat4 std = glm::translate(glm::mat4(1.0f), baseTranslate) * glm::rotate(glm::mat4(1.0f), glm::radians(-40.0f), right) * frontFaceingRotation * glm::scale(glm::mat4(1.0f), glm::vec3(g_cardScale));

	int numCards = cards.size();

	float distBetween = std::min(std::max(maxWidth * 2.0f / (float)cards.size(), g_cardMinDiff), maxDiff);

	if (distBetween * numCards > maxWidth * 2.0f)
	{
		if (wideCardsStart < 0.0f) wideCardsStart = 0.0f;
		const float smallWidth = 0.125f * maxWidth;
		const float bigWidth = 7.0f / 4.0f * maxWidth;
		const float xOff = maxWidth;

		const float unchangedSize = distBetween * numCards;
		if (wideCardsStart > unchangedSize - bigWidth - smallWidth) wideCardsStart = unchangedSize - bigWidth - smallWidth;
		const float sigma1 = wideCardsStart;
		const float sigma2 = unchangedSize - (wideCardsStart + bigWidth);


		const int numCardsInSigma1 = (int)(sigma1 / g_cardMinDiff);
		const int numCardsInSigma2 = (int)(sigma2 / g_cardMinDiff);

		const float distSigma1 = std::min(smallWidth * g_cardMinDiff / sigma1, g_cardMinDiff);	// the real distance between sigma1 elements
		const float distSigma2 = std::min(smallWidth * g_cardMinDiff / sigma2, g_cardMinDiff);	// the real distance between sigma2 elements

		float curScale = -xOff;	// overall start of the whole thing
		for (int i = 0; i < numCardsInSigma1 && i < numCards; i++)
		{
			auto& c = cards.at(i);
			c.transform = glm::translate(glm::mat4(1.0f), right * curScale * g_cardScale + camUp * 0.2f * c.hoverCounter) * std;
			curScale += distSigma1;
		}
		int idx = numCardsInSigma1;
		const float bigEnd = -xOff + smallWidth + bigWidth;
		while (curScale < bigEnd && idx < numCards)
		{
			auto& c = cards.at(idx);
			c.transform = glm::translate(glm::mat4(1.0f), right * curScale * g_cardScale + camUp * 0.2f * c.hoverCounter) * std;
			curScale += g_cardMinDiff;
			idx++;
		}

		for (int i = idx; i < numCards; i++)
		{
			auto& c = cards.at(i);
			c.transform = glm::translate(glm::mat4(1.0f), right * curScale * g_cardScale + camUp * 0.2f * c.hoverCounter) * std;
			curScale += distSigma2;
		}
	}
	else
	{
		wideCardsStart = distBetween * numCards / 2;
		float start = -distBetween * numCards / 2;
		if ((numCards % 2) == 1) start += distBetween / 2.0f;
		for (int i = 0; i < cards.size(); i++)
		{
			auto& c = cards.at(i);
			float scale = (start + i * distBetween) * g_cardScale;
			c.transform = glm::translate(glm::mat4(1.0f), right * scale + camUp * 0.2f * c.hoverCounter) * std;
		}
	}

	needRegen = false;
}
void CardHand::Draw(const Camera& cam)
{
	static float idx = 0.0f;
	static bool countDown = false;
	if (needRegen) GenTransformations(cam);

	if (countDown) idx = std::max(idx - 0.02f, 0.0f);
	else idx = std::min(idx + 0.02f, 1.0f);
	
	if (idx == 0.0f) countDown = false;
	else if (idx == 1.0f) countDown = true;
	
	for (int i = 0; i < cards.size(); i++) {
		auto& c = cards.at(i);

		if (!c.visible) {
			continue;	// don't draw temporary cards
		}
		if (i == highlightedCardIdx)
		{
			float scale = 1.3f + 0.1f * idx;
			glm::mat4 s = glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale - 0.1f, scale));
			RendererAddEffect(CARD_EFFECT::CARD_EFFECT_BLUR, c.transform * s, 0xFF00D7FF);
		}
		RendererAddCard(c.back, c.front, c.transform);
	}
	
}








AnimatedCard::AnimatedCard(const CardStack& stack, CARD_ID front, const glm::vec3& pos, float yaw, float pitch, float roll, float dur, CARD_ANIMATIONS a, int cardID, int handID) {
	back = CARD_ID_BLANK; this->front = front; anim.AddState(pos, yaw, pitch, roll, AnimationType::ANIMATION_LINEAR, dur); this->cardID = cardID; this->handID = handID;
	if (a == ANIM_PLAY_CARD) {
		AddStackAnimation(stack);
	}
	else if (a == ANIM_FETCH_CARD)
	{
		AddDeckAnimation();
	}
}
AnimatedCard::AnimatedCard(const CardStack& stack, CARD_ID front, const glm::mat4& mat, float dur, CARD_ANIMATIONS a, int cardID, int handID)
{
	back = CARD_ID_BLANK; this->front = front; anim.AddState(mat, AnimationType::ANIMATION_LINEAR, dur); this->cardID = cardID; this->handID = handID;
	if (a == ANIM_PLAY_CARD) {
		AddStackAnimation(stack);
	}
	else if (a == ANIM_FETCH_CARD)
	{
		AddDeckAnimation();
	}
}
void AnimatedCard::AddStackAnimation(const CardStack& stack)
{
	type = CARD_ANIMATIONS::ANIM_PLAY_CARD;
	if (anim.states.empty()) return;// first state should be set already
	auto& s = anim.states.at(0);
	glm::vec3 startPos = s.transform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

	float startYaw = glm::degrees((glm::vec4(0.0f, 1.0f, 0.0f, 0.0f) * s.transform).y);
	float startPitch = glm::degrees((glm::vec4(1.0f, 0.0f, 0.0f, 0.0f) * s.transform).x);
	float startRoll = glm::degrees((glm::vec4(0.0f, 0.0f, 1.0f, 0.0f) * s.transform).z);
	glm::vec3 endPos = glm::vec3(0.0f, CardStack::dy * stack.cards.size(), 0.0f);

	float endYaw = (rand() % 360) * 1.0f;	// 0� -> 360�
	float endPitch = -90.0f;
	float endRoll = 0.0f;

	glm::vec3 middlePos = (endPos + startPos) / 2.0f + glm::vec3(0.0f, 0.4f, 0.0f);

	float middleYaw = (endYaw - startYaw) / 2.0f;
	float middlePitch = (endPitch - startPitch) / 2.0f;
	float middleRoll = (endRoll - startRoll) / 2.0f;

	anim.AddState(middlePos, middleYaw, middlePitch, middleRoll, AnimationType::ANIMATION_LINEAR, 0.15f);
	anim.AddState(endPos, endYaw, endPitch, endRoll, AnimationType::ANIMATION_LINEAR, 0.3f);
}
void AnimatedCard::AddDeckAnimation()
{
	type = CARD_ANIMATIONS::ANIM_FETCH_CARD;
	if (anim.states.empty()) return;
	anim.AddState(glm::vec3(CardDeck::dx, CardDeck::topY, 0.0f), 0.0f, 90.0f, 0.0f, AnimationType::ANIMATION_LINEAR, 0.0f);
	std::swap(anim.states.at(0), anim.states.at(1));
	anim.states.at(1).duration = 0.4f;
}



void CardsInAnimation::AddAnim(const CardStack& stack, const CardInfo& info, int handID, CARD_ANIMATIONS id)
{
	const float inv_scale = 1.0f / g_cardScale;
	list.emplace_back(stack, info.front, glm::scale(info.transform, glm::vec3(inv_scale)), 0.0f, id, info.transitionID, handID);
}
void CardsInAnimation::Update(std::vector<CardHand>& hands, CardStack& stack, float dt)
{
	bool anyTrue = false;
	for (auto& a : list)
	{
		glm::mat4 curTransform = a.anim.GetTransform(dt, a.done) * glm::scale(glm::mat4(1.0f), glm::vec3(g_cardScale));
		glm::vec3 pos = curTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		RendererAddCard(a.back, a.front, curTransform);
		if (a.done) anyTrue = true;
	}
	if (anyTrue)
	{
		std::vector<AnimatedCard> newList;
		for (auto& a : list)
		{
			if (!a.done) newList.push_back(a);
			else
			{
				OnFinish(hands, stack, a.anim.currentTransform, a.front, a.type, a.handID, a.cardID);
			}
		}
		list = std::move(newList);
	}
}

void CardsInAnimation::OnFinish(std::vector<CardHand>& hands, CardStack& stack, const glm::mat4& transform, CARD_ID id, CARD_ANIMATIONS type, int handID, int transitionID)
{
	CardHand* hand = nullptr;
	for (auto& h : hands)
	{
		if (h.handID == handID) hand = &h;
	}

	if (hand) {
		for (auto& c : hand->cards)
		{
			if (!c.visible && c.transitionID == transitionID)
			{
				if (type == CARD_ANIMATIONS::ANIM_FETCH_CARD) {
					c.visible = true;
				}
				return;
			}
		}
	}
	if (type == CARD_ANIMATIONS::ANIM_PLAY_CARD) {
		stack.AddToStack(id, transform * glm::scale(glm::mat4(1.0f), glm::vec3(g_cardScale)));
	}
}