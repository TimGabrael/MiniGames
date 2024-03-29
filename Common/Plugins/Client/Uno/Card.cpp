#include "Card.h"
#include "Graphics/GLCompat.h"
#include "Graphics/Helper.h"
#include <glm/glm.hpp>
#include "FileQuery.h"
#include "Graphics/Scene.h"
#include "imgui_internal.h"
#include "logging.h"
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <glm/gtc/quaternion.hpp>
#include "Graphics/UiRendering.h"
#include "Graphics/Renderer.h"
#include "Uno.h"

#define CARD_SCENE_TYPE_INDEX DEFAULT_SCENE_RENDER_TYPES::NUM_DEFAULT_RENDERABLES
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
layout(location = 0) in vec3 pos;\n\
layout(location = 1) in vec3 nor;\n\
layout(location = 2) in vec2 texPos;\n\
layout(location = 3) in vec4 col;\n\
uniform mat4 projection;\n\
uniform mat4 view;\n\
uniform vec4 clipPlane;\n\
out vec3 outNormal;\
out vec2 tPos;\
out vec4 addCol;\
out float clipDist;\
void main(){\
	vec4 worldPos = vec4(pos, 1.0f);\
	gl_Position = projection * view * worldPos;\
	tPos = texPos;\
	outNormal = nor;\
	addCol = col;\
	clipDist = dot(worldPos, clipPlane);\
}\
";
static const char* cardFragmentShader = "#version 300 es\n\
precision highp float;\n\
\n\
in vec3 outNormal;\
in vec2 tPos;\
in vec4 addCol;\
in float clipDist;\
uniform vec3 lDir;\
uniform sampler2D tex;\
out vec4 outCol;\
void main(){\
	if(clipDist < 0.0f) discard;\
	vec4 c = texture(tex, tPos) * addCol;\n\
	vec3 norm = normalize(outNormal);\
	float diff = min(max(dot(norm, lDir), 0.0), 0.8);\
	outCol = vec4((diff+0.2) * c.rgb, c.a);\n\
}";
static const char* cardGeometryFragmentShader = "#version 300 es\n\
precision highp float;\n\
\n\
in vec3 outNormal;\
in vec2 tPos;\
in vec4 addCol;\
uniform vec3 lDir;\
uniform sampler2D tex;\
void main(){\
	vec4 c = texture(tex, tPos);\
	if (c.a < 1.0f) discard;\n\
}";


static glm::vec2 CardIDToTextureIndex(uint32_t id)
{
	if (id < CARD_ID::NUM_AVAILABLE_CARDS)
	{
		const int xPos = id % 14;
		const int yPos = id / 14;
		return { CARD_STEP_SIZEX * xPos, CARD_STEP_SIZEY * yPos };
	}
	else
	{
		if (id == CARD_ID::NUM_AVAILABLE_CARDS) return { 0, CARD_EFFECT_START_Y / IMAGE_SIZE_Y };
	}
	return { 0.0f,0.0f };
}
static glm::vec2 CardIDToTextureSize(uint32_t id)
{
	if (id < CARD_ID::NUM_AVAILABLE_CARDS)
	{
		return { CARD_STEP_SIZEX, CARD_STEP_SIZEY };
	}
	else
	{
		return { CARD_EFFECT_SIZEX, CARD_EFFECT_SIZEY };
	}
}

struct CardVertex
{
	glm::vec3 pos;
	glm::vec3 nor;
	glm::vec2 tex;
	uint32_t col;
};
struct _InternalCard
{
	glm::vec3 pos;
	glm::quat rot;
	float scaleX;
	float scaleY;
	uint32_t col;
	uint16_t frontID;
	uint16_t backID;
};

static void GenerateCardListsFromCache(CardVertex* vertList, uint32_t* indList, const std::vector<_InternalCard>& cache, const glm::vec3& camPos, int* numInds, bool backwards)
{
	const size_t cardCount = cache.size() - 1;
	uint32_t curVert = 0;
	int curIdx = 0;
	for (size_t i = 0; i < cache.size(); i++)
	{
		const _InternalCard& card = cache.at(backwards ? cardCount - i : i);
		glm::vec3 delta = card.pos - camPos;
		glm::vec3 cardNormal = glm::normalize(card.rot * glm::vec3(0.0f, 0.0f, 1.0f));
		glm::vec3 dx = card.rot * glm::vec3(0.5f, 0.0f, 0.0f) * card.scaleX;
		glm::vec3 dy = card.rot * glm::vec3(0.0f, 1.0f, 0.0f) * card.scaleY * CARD_HEIGHT_HALF;

		float dir = glm::dot(delta, cardNormal);
		glm::vec2 texCoord;
		glm::vec2 texSize;
		if (dir > 0.0f) // show back
		{
			texCoord = CardIDToTextureIndex(card.backID);
			texSize = CardIDToTextureSize(card.backID);

			indList[curIdx + 0] = curVert + 0;
			indList[curIdx + 1] = curVert + 3;
			indList[curIdx + 2] = curVert + 2;
			indList[curIdx + 3] = curVert + 2;
			indList[curIdx + 4] = curVert + 1;
			indList[curIdx + 5] = curVert + 0;

		}
		else if (dir < 0.0f) // show front
		{
			texCoord = CardIDToTextureIndex(card.frontID);
			texSize = CardIDToTextureSize(card.frontID);
			cardNormal = glm::normalize(card.rot * glm::vec3(0.0f, 0.0f, -1.0f));

			indList[curIdx + 0] = curVert + 0;
			indList[curIdx + 1] = curVert + 1;
			indList[curIdx + 2] = curVert + 2;
			indList[curIdx + 3] = curVert + 2;
			indList[curIdx + 4] = curVert + 3;
			indList[curIdx + 5] = curVert + 0;
		}
		else
		{
			continue;
		}
		vertList[curVert] = { card.pos - dx - dy, cardNormal, glm::vec2(texCoord.x, texCoord.y + texSize.y), card.col };
		vertList[curVert+1] = { card.pos + dx - dy, cardNormal, glm::vec2(texCoord.x + texSize.x, texCoord.y + texSize.y), card.col };
		vertList[curVert + 2] = { card.pos + dx + dy, cardNormal, { texCoord.x + texSize.x, texCoord.y }, card.col };
		vertList[curVert+3] = { card.pos - dx + dy, cardNormal, glm::vec2(texCoord.x, texCoord.y), card.col };

		curVert += 4;
		curIdx += 6;
	}
	*numInds = curIdx;
}


struct CardBuffers
{
	GLuint vao;
	GLuint verts;
	GLuint inds;
	size_t bufferSize;
	std::vector<_InternalCard> cardCache;
};
struct CardUniforms
{
	GLuint projection;
	GLuint view;
	GLuint tex;
	GLuint lightDir;
	GLuint plane;
};

struct CardPipeline
{
	GLuint geometryProgram;
	GLuint program;
	GLuint texture;
	
	CardBuffers bufs;
	CardUniforms unis;
	CardUniforms geomUnis;

	float width_half = 0.0f;
	float height_half = 0.0f;

}g_cards;


void InitializeCardPipeline(void* assetManager)
{
	g_cards.program = CreateProgram(cardVertexShader, cardFragmentShader);
	g_cards.geometryProgram = CreateProgram(cardVertexShader, cardGeometryFragmentShader);
	g_cards.geomUnis.projection = glGetUniformLocation(g_cards.geometryProgram, "projection");
	g_cards.geomUnis.view = glGetUniformLocation(g_cards.geometryProgram, "view");
	g_cards.geomUnis.tex = glGetUniformLocation(g_cards.geometryProgram, "tex");
	g_cards.geomUnis.plane = glGetUniformLocation(g_cards.geometryProgram, "clipPlane");
	g_cards.geomUnis.lightDir = glGetUniformLocation(g_cards.geometryProgram, "lDir");
	
	g_cards.unis.projection = glGetUniformLocation(g_cards.program, "projection");
	g_cards.unis.view = glGetUniformLocation(g_cards.program, "view");
	g_cards.unis.tex = glGetUniformLocation(g_cards.program, "tex");
	g_cards.unis.plane = glGetUniformLocation(g_cards.program, "clipPlane");
	g_cards.unis.lightDir = glGetUniformLocation(g_cards.program, "lDir");

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
	g_cards.width_half = 0.5f;
	g_cards.height_half = CARD_HEIGHT_HALF;

	glGenVertexArrays(1, &g_cards.bufs.vao);
	glBindVertexArray(g_cards.bufs.vao);

	glGenBuffers(1, &g_cards.bufs.verts);
	glBindBuffer(GL_ARRAY_BUFFER, g_cards.bufs.verts);
	

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CardVertex), nullptr);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(CardVertex), (void*)(offsetof(CardVertex, nor)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(CardVertex), (void*)(offsetof(CardVertex, tex)));
	glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(CardVertex), (void*)(offsetof(CardVertex, col)));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);

	glBindVertexArray(0);

	glGenBuffers(1, &g_cards.bufs.inds);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_cards.bufs.inds);

	g_cards.bufs.bufferSize = 0;

}
void UnInitializeCardPipeline() {
    glDeleteBuffers(1, &g_cards.bufs.inds);
    glDeleteBuffers(1, &g_cards.bufs.verts);
    glDeleteVertexArrays(1, &g_cards.bufs.vao);
    glDeleteProgram(g_cards.program);
    glDeleteProgram(g_cards.geometryProgram);
    glDeleteTextures(1, &g_cards.texture);
    g_cards.bufs.cardCache.clear();
    g_cards.bufs.bufferSize = 0;
    
}

void RendererAddCard(CARD_ID back, CARD_ID front, const glm::vec3& pos, const glm::quat& rot, float sx, float sy, uint32_t col)
{
	g_cards.bufs.cardCache.push_back({ pos, rot, sx, sy, col, (uint16_t)front, (uint16_t)back });
}
void RendererAddEffect(CARD_EFFECT effect, const glm::vec3& pos, const glm::quat& rot, float sx, float sy, uint32_t col)
{
	uint16_t faceID = (uint16_t)effect + (uint16_t)CARD_ID::NUM_AVAILABLE_CARDS;
	g_cards.bufs.cardCache.push_back({ pos, rot, sx, sy, col, faceID, faceID });
	
}
void ClearCards()
{
	g_cards.bufs.cardCache.clear();
}

CardSceneObject* CreateCardBatchSceneObject(PScene scene)
{
	CardSceneObject* obj = new CardSceneObject();
	obj->flags = SCENE_OBJECT_BLEND | SCENE_OBJECT_REFLECTED | SCENE_OBJECT_CAST_SHADOW | SCENE_OBJECT_SURFACE_REFLECTED;
	obj->bbox.leftTopFront = { -3.0f, -3.0f, -3.0f };
	obj->bbox.rightBottomBack = { 3.0f, 3.0f, 3.0f };
	obj->lightGroups = 0xFFFF;
    obj->additionalFlags = 0;
    SC_AddSceneObject(scene, obj);
	return obj;
}
void FreeCardBatchSceneObject(PScene scene, CardSceneObject* obj) {
    SC_RemoveSceneObject(scene, obj);
    delete obj;
}

int FillCardListAndMapToBuffer(const glm::vec3& pos, bool backwards)
{
	glBindBuffer(GL_ARRAY_BUFFER, g_cards.bufs.verts);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_cards.bufs.inds);
	if (g_cards.bufs.cardCache.size() > g_cards.bufs.bufferSize)
	{
		g_cards.bufs.bufferSize = g_cards.bufs.cardCache.size() + 100;
		const uint32_t vtxSize = (uint32_t)(sizeof(CardVertex) * g_cards.bufs.bufferSize * 4);
		const uint32_t idxSize = (uint32_t)(sizeof(uint32_t) * g_cards.bufs.bufferSize * 6);
		glBufferData(GL_ARRAY_BUFFER, vtxSize, nullptr, GL_DYNAMIC_DRAW);

		glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxSize, nullptr, GL_DYNAMIC_DRAW);
	}

	CardVertex* vtxBuf = (CardVertex*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(CardVertex) * g_cards.bufs.bufferSize * 4, GL_MAP_WRITE_BIT);
	uint32_t* idxBuf = (uint32_t*)glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(uint32_t) * g_cards.bufs.bufferSize * 6, GL_MAP_WRITE_BIT);
	int numInds = 0;
	GenerateCardListsFromCache(vtxBuf, idxBuf, g_cards.bufs.cardCache, pos, &numInds, backwards);
	glUnmapBuffer(GL_ARRAY_BUFFER);
	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
	return numInds;
}
void DrawCards(const glm::mat4& proj, const glm::mat4& view, const glm::vec3& camPos, const glm::vec3& lDir, bool geomOnly)
{
	CardUniforms* unis = geomOnly ? &g_cards.geomUnis : &g_cards.unis;
	glUseProgramWrapper(geomOnly ? g_cards.geometryProgram : g_cards.program);
	
	glBindVertexArray(g_cards.bufs.vao);
	glUniformMatrix4fv(unis->projection, 1, GL_FALSE, (const GLfloat*)&proj);
	glUniformMatrix4fv(unis->view, 1, GL_FALSE, (const GLfloat*)&view);
	glUniform3fv(unis->lightDir, 1, (const GLfloat*)&lDir);
	glUniform4f(unis->plane, 0.0f, 1.0f, 0.0f, 10000000.0f);
	int numInds = FillCardListAndMapToBuffer(camPos, false);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_cards.texture);

	
	glDrawElementsWrapper(GL_TRIANGLES, numInds, GL_UNSIGNED_INT, nullptr);
	
	glBindVertexArray(0);
}

CardSceneObject::~CardSceneObject() {
}

size_t CardSceneObject::GetType() const {
    return 4;
}
void CardSceneObject::DrawGeometry(StandardRenderPassData* pass)  {
	const CurrentLightInformation* cur = GetCurrentLightInformation();
	glm::vec3 dir(0.0f);
	if (cur->numCurDirLights > 0) {
		dir = cur->curDirLights[0].data.dir;
	}
    if(pass->drawShadow) { 
        DrawCards(*pass->camProj, *pass->camView, *pass->camPos, dir, true);
    }
}
void CardSceneObject::DrawOpaque(StandardRenderPassData* pass)  {

}
void CardSceneObject::DrawBlend(StandardRenderPassData* pass)  {
	const CurrentLightInformation* cur = GetCurrentLightInformation();
	glm::vec3 dir(0.0f);
	if (cur->numCurDirLights > 0)
	{
		dir = cur->curDirLights[0].data.dir;
	}	
	SetDefaultBlendState();
	DrawCards(*pass->camProj, *pass->camView, *pass->camPos, dir, false);
}
void CardSceneObject::DrawBlendClip(ReflectPlanePassData* pass)  {
	const glm::vec3* camPos = pass->base->camPos;

	SetDefaultBlendState();

	const CurrentLightInformation* cur = GetCurrentLightInformation();
	glm::vec3 lightDir(0.0f);
	if (cur->numCurDirLights > 0)
	{
		lightDir = cur->curDirLights[0].data.dir;
	}

	glUseProgramWrapper(g_cards.program);
	glBindVertexArray(g_cards.bufs.vao);
	glUniformMatrix4fv(g_cards.unis.projection, 1, GL_FALSE, (const GLfloat*)pass->base->camProj);
	glUniformMatrix4fv(g_cards.unis.view, 1, GL_FALSE, (const GLfloat*)pass->base->camView);
	glUniform4fv(g_cards.unis.plane, 1, (const GLfloat*)pass->planeEquation);
	glUniform3fv(g_cards.unis.lightDir, 1, (const GLfloat*)&lightDir);

	const int numInds = FillCardListAndMapToBuffer(*pass->base->camPos, true);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_cards.texture);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_cards.bufs.inds);

	glDrawElementsWrapper(GL_TRIANGLES, numInds, GL_UNSIGNED_INT, nullptr);
}
void CardSceneObject::DrawOpaqueClip(ReflectPlanePassData* pass)  {

}

static bool HitTest(const glm::vec3& center, const glm::quat& rot, float scaleX, float scaleY, const glm::vec3& camPos, const glm::vec3& mouseRay)
{
	glm::vec3 normal = normalize(rot * glm::vec3(0.0f, 0.0f, 1.0f));
	glm::vec3 tangent1 = rot * glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 tangent2 = rot * glm::vec3(0.0f, 1.0f, 0.0f);

	float scaledWidth_half = g_cards.width_half * scaleX;
	float scaledHeight_half = g_cards.height_half * scaleY;


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


static bool CardSort(CARD_ID low, CARD_ID high)
{
	return low < high;
}



COLOR_ID GetColorIDFromCardID(CARD_ID id)
{
	if (id < CARD_ID_CHOOSE_COLOR) return COLOR_RED;
	else if (id == CARD_ID_CHOOSE_COLOR) return COLOR_BLACK;
	else if (id < CARD_ID_ADD_4) return COLOR_YELLOW;
	else if (id == CARD_ID_ADD_4) return COLOR_BLACK;
	else if (id < CARD_ID_BLANK) return COLOR_GREEN;
	else if (id == CARD_ID_BLANK) return COLOR_INVALID;
	else if (id < CARD_ID_BLANK2) return COLOR_BLUE;
	return COLOR_INVALID;
}
uint32_t GetColorFromColorID(COLOR_ID id)
{
	if (id == COLOR_RED) return 0xFF0000FF;
	else if (id == COLOR_YELLOW) return 0xFF00FFFF;
	else if (id == COLOR_GREEN) return 0xFF00FF00;
	else if (id == COLOR_BLUE) return 0xFFFF0000;
	else if (id == COLOR_BLACK) return 0xFF000000;
	return 0xFFFFFFFF;
}
uint32_t GetColorFromCardID(CARD_ID id)
{
	return GetColorFromColorID(GetColorIDFromCardID(id));
}

































COLOR_ID ColorPicker::GetSelected(int mx, int my, int screenX, int screenY, bool pressed, bool released)
{
	const float mX = 2.0f * ((float)mx / (float)screenX) - 1.0f;
	const float mY = 1.0f - 2.0f * ((float)my / (float)screenY);
	const float aspectRatio = (float)screenX / (float)screenY;

	const float oa = aspectRatio * o;
	const float r_2 = (r + o) * (r + o);
	const float ra_2 = aspectRatio * aspectRatio * r_2;


	const float ellipseHitEq = mX * mX / r_2 + mY * mY / ra_2;

	COLOR_ID output = COLOR_INVALID;

	if (ellipseHitEq < 1.0f)
	{
		if (mX > 0.0f && mY > 0.0f)			// region red
		{
			this->hoveredColor = COLOR_RED;
		}
		else if (mX > 0.0f && mY < 0.0f)	// region yellow
		{
			this->hoveredColor = COLOR_YELLOW;

		}
		else if (mX < 0.0f && mY < 0.0f)	// region green
		{
			this->hoveredColor = COLOR_GREEN;
		}
		else								// region blue 
		{
			this->hoveredColor = COLOR_BLUE;
		}
		if (pressed) pressedColor = hoveredColor;
		if (released && pressedColor != -1 && pressedColor == hoveredColor) {
			if (pressedColor == 0) output = COLOR_RED;
			else if (pressedColor == 1) output = COLOR_YELLOW;
			else if (pressedColor == 2) output = COLOR_GREEN;
			else if (pressedColor == 3) output = COLOR_BLUE;
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
	else DrawCircle({ o, oa}, { r, ra },	glm::radians(0.0f),   glm::radians(90.0f), 0x600000FF, numSamples);								// red

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
	glm::quat rotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	for (int i = 0; i < numCards; i++)
	{
		RendererAddCard(CARD_ID_BLANK, CARD_ID_BLANK, glm::vec3(dx, i * dy, 0.0f), rotation, g_cardScale, g_cardScale);
	}
}

void CardStack::Draw()
{
    UnoPlugin* uno = GetInstance();
	const float scale = g_cardScale;
	for (size_t i = 0; i < cards.size(); i++)
	{
        auto& c = cards.at(i);
        if (i == cards.size() - 1) {
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
            const float s = scale * (1.3f + 0.4f * topAnim);
            const uint32_t col = GetColorFromColorID(this->blackColorID);
            if((c.front == CARD_ID::CARD_ID_ADD_4 || c.front == CARD_ID::CARD_ID_CHOOSE_COLOR) && (blackColorID != COLOR_INVALID)) RendererAddEffect(CARD_EFFECT_BLUR, c.position, c.rotation, s, s - 0.1f * scale, col);
            RendererAddCard(c.back, c.front, c.position, c.rotation, scale, scale);

            const glm::vec3 front = uno->g_objs->playerCam.GetFront();
            const glm::vec3 right = uno->g_objs->playerCam.GetRight();
            const glm::vec3 up = uno->g_objs->playerCam.GetUp();
            const glm::vec3 translated = c.position + 1.5f * right + up * 1.5f + glm::cross(right, up) * 1.5f;
            const glm::quat rotation = glm::quatLookAt(front, up);
            const float extra_scale = 5.0f;
            const uint32_t big_col = (col & 0xFFFFFF) | (0x30 << 24);
            if((c.front == CARD_ID::CARD_ID_ADD_4 || c.front == CARD_ID::CARD_ID_CHOOSE_COLOR) && (blackColorID != COLOR_INVALID)) RendererAddEffect(CARD_EFFECT_BLUR, translated, rotation, s * extra_scale, (s - 0.1f * scale) * extra_scale, big_col);
            RendererAddCard(c.back, c.front, translated, rotation, scale * extra_scale, scale * extra_scale, 0x60FFFFFF);

        }
        else {
            RendererAddCard(c.back, c.front, c.position, c.rotation, scale, scale);
        }
	}
}
void CardStack::AddToStack(CARD_ID card, const glm::vec3& pos, const glm::quat& rot)
{
	if (cards.size() > 10)
	{
		for (int i = 0; i < cards.size() - 1; i++)
		{
			cards.at(i) = cards.at(i + 1);
		}
		auto& ad = cards.at(cards.size() - 1);
		ad.front = card;
		ad.hoverCounter = 0.0f; ad.position = pos; ad.rotation = rot; ad.visible = true;
	}
	else
	{
		cards.emplace_back(CARD_ID::CARD_ID_BLANK, card, glm::vec3(pos.x, pos.y + 0.001f, pos.z), rot, 0.0f, true);
	}
}
CARD_ID CardStack::GetTop(COLOR_ID& blackColorRef) const
{
	if (cards.empty()) return (CARD_ID)-1;

	CARD_ID top = cards.at(cards.size() - 1).front;
	if (top == CARD_ID::CARD_ID_ADD_4 || top == CARD_ID::CARD_ID_CHOOSE_COLOR) blackColorRef = blackColorID;
	return top;
}
void CardStack::SetTop(CARD_ID topCard, COLOR_ID blackColorRef)
{
	this->blackColorID = blackColorRef;
	if (!cards.empty())
	{
		cards.at(cards.size() - 1).front = topCard;
	}
	else
	{
		glm::quat rotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		cards.emplace_back(CARD_ID::CARD_ID_BLANK, topCard, glm::vec3(0.0f, 0.001f, 0.0f), rotation, false, true);
	}
}


void CardHand::Add(CARD_ID id)
{
	wideCardsStart += g_cardMinDiff * 0.5f;
	

	cards.emplace_back(CARD_ID::CARD_ID_BLANK, id, glm::vec3(0.0f), glm::mat4(1.0f), 0.0f, true);
	if (willBeSorted)
	{
		std::sort(cards.begin(), cards.end(), [](CardInfo& i1, CardInfo& i2) {
			return CardSort(i1.front, i2.front);
			});
	}
	needRegen = true;
}
int CardHand::AddTemp(const Camera& cam, CARD_ID id)
{
	if (wideCardsStart >= 0.0f) wideCardsStart += g_cardMinDiff * 0.5f;

	cards.emplace_back(CARD_ID::CARD_ID_BLANK, id, glm::vec3(0.0f), glm::mat4(1.0f), -1.0f, false);
	if (willBeSorted)
	{
		std::sort(cards.begin(), cards.end(), [](CardInfo& i1, CardInfo& i2) {
			return CardSort(i1.front, i2.front);
			});
	}
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
	needRegen = true;
	return idx;
}

void CardHand::PlayCard(const CardStack& stack, CardsInAnimation& anim, int cardIdx)
{
	UnoPlugin* instance = GetInstance();
	GameStateData* state = GetGameState();
    CardData internalData = RenderCardToCardData(cards.at(cardIdx).front, COLOR_ID::COLOR_BLACK);
    if(IsCardPlayable(instance->g_objs->game.topCard, internalData)) {
        if (cards.at(cardIdx).front == CARD_ID::CARD_ID_ADD_4 || cards.at(cardIdx).front == CARD_ID::CARD_ID_CHOOSE_COLOR)
        {
            state->isChoosingColor = true;
			this->choosingCardColor = true;
            this->playIdxAfterChoosing = cardIdx;
        }
        else
        {
            cardIdxSendToServer = cardIdx;
            uno::ClientPlayCard play;
            play.mutable_card()->set_color(internalData.color);
            play.mutable_card()->set_face(internalData.face);
            instance->backendData->net->SendData(Client_UnoPlayCard, &play, SendFlags::Send_Reliable);
            playIdxAfterChoosing = -1;
            
        }
    }
	
}
void CardHand::PlayCardServer(const CardStack& stack, CardsInAnimation& anim, CARD_ID id, COLOR_ID col)
{
	UnoPlugin* instance = GetInstance();
	if (handID != instance->backendData->localPlayer.clientID)
	{
		if (cards.size() == 0)
		{
			this->Add(id);
			GenTransformations(instance->g_objs->playerCam);
		}
		cards.at(0).front = id;
		anim.AddAnim(stack, cards.at(0), handID, CARD_ANIMATIONS::ANIM_PLAY_CARD);
		cards.erase(cards.begin());
	}
	else
	{
		if (cardIdxSendToServer >= 0 && cardIdxSendToServer < cards.size() && cards.at(cardIdxSendToServer).front == id)
		{
			anim.AddAnim(stack, cards.at(cardIdxSendToServer), handID, CARD_ANIMATIONS::ANIM_PLAY_CARD);
			cards.erase(cards.begin() + cardIdxSendToServer);
			cardIdxSendToServer = -1;	
		}
		else
		{
			int foundIdx = -1;
			while (foundIdx < 0)
			{
				for (int i = 0; i < cards.size(); i++)
				{
					const auto& c = cards.at(i);
					if (c.front == id) {
						foundIdx = i;
						break;
					}
				}
				if (foundIdx < 0) {
					Add(id);
					GenTransformations(instance->g_objs->playerCam);
				}
			}

			anim.AddAnim(stack, cards.at(foundIdx), handID, CARD_ANIMATIONS::ANIM_PLAY_CARD);
			cards.erase(cards.begin() + foundIdx);
			cardIdxSendToServer = -1;
		}
	}
}

void CardHand::PullCardServer(const Camera& cam, const CardStack& stack, CardDeck& deck, CardsInAnimation& anim, CARD_ID id)
{
	UnoPlugin* instance = GetInstance();
	int idx = AddTemp(cam, id);
	anim.AddAnim(stack, cards.at(idx), handID, CARD_ANIMATIONS::ANIM_FETCH_CARD);
}
void CardHand::Update(CardStack& stack, CardsInAnimation& anim, ColorPicker& picker, const Camera& cam, const glm::vec3& mouseRay, const Pointer& p, bool allowInput)
{
	needRegen = true;
	if (needRegen) GenTransformations(cam);
	glm::vec3 cp = cam.pos;
	glm::vec3 mRay = mouseRay;

	
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
	for (int i = (int)(cards.size() - 1); i >= 0; i--)
	{
		auto& c = cards.at(i);
		if (hitIdx == -1 && HitTest(c.position, c.rotation, g_cardScale, g_cardScale, cp, mRay))
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
	UnoPlugin* instance = GetInstance();
	GameStateData* state = GetGameState();

	PlayerInfo* localPlayer = state->GetLocalPlayer();
	if (localPlayer)
	{
		if (localPlayer->id == state->playerInTurn && state->isChoosingColor)
		{
			COLOR_ID id = picker.GetSelected(p.x, p.y, cam.screenX, cam.screenY, p.Pressed(), p.Released());
			if (id != COLOR_INVALID)
			{
				choosenCardColor = id;
				stack.blackColorID = id;
				state->isChoosingColor = false;
				state->playerInTurn = (state->playerInTurn + 1) % state->players.size();

                CardData internalData = RenderCardToCardData(this->cards.at(playIdxAfterChoosing).front, choosenCardColor);
                choosenCardColor = id;
                internalData.color = (CardColor)id;

                cardIdxSendToServer = playIdxAfterChoosing;
                uno::ClientPlayCard play;
                play.mutable_card()->set_color(id);
                play.mutable_card()->set_face(internalData.face);
                instance->backendData->net->SendData(Client_UnoPlayCard, &play, SendFlags::Send_Reliable);
                playIdxAfterChoosing = -1;
			}
		}
		else if (allowInput)
		{
			if (wasReleased && oldMouseSelectedCard == hitIdx && hitIdx != -1)
			{
				PlayCard(stack, anim, oldMouseSelectedCard);
			}
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
	if (rotation != 0.0f) {
		maxWidth *= 0.4f;
	}

	const glm::quat transRot = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));


	const glm::vec3 camUp = transRot * cam.GetRealUp();
	const glm::vec3 camFront = transRot * cam.GetFront();
	const glm::vec3 right = transRot * cam.GetRight();
	glm::vec3 baseTranslate = transRot * cam.pos + (camFront + camUp * -1.0f);
	float camYaw = cam.GetYaw() + 90.0f - rotation;
	const glm::quat frontFaceingRotation = glm::rotate(glm::mat4(1.0f), glm::radians(-40.0f), right) * glm::rotate(glm::mat4(1.0f), glm::radians(camYaw), glm::vec3(0.0f, -1.0f, 0.0f));
	
	const size_t numCards = cards.size();
	const float distBetween = std::min(std::max(maxWidth * 2.0f / (float)cards.size(), g_cardMinDiff), maxDiff);

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
			c.position = right * curScale * g_cardScale + camUp * 0.2f * c.hoverCounter + baseTranslate;
			c.rotation = frontFaceingRotation;
			//c.transform = glm::translate(glm::mat4(1.0f), right * curScale * g_cardScale + camUp * 0.2f * c.hoverCounter) * std;
			curScale += distSigma1;
		}
		int idx = numCardsInSigma1;
		const float bigEnd = -xOff + smallWidth + bigWidth;
		while (curScale < bigEnd && idx < numCards)
		{
			auto& c = cards.at(idx);
			c.position = right * curScale * g_cardScale + camUp * 0.2f * c.hoverCounter + baseTranslate;
			c.rotation = frontFaceingRotation;
			curScale += g_cardMinDiff;
			idx++;
		}

		for (int i = idx; i < numCards; i++)
		{
			auto& c = cards.at(i);
			c.position = right * curScale * g_cardScale + camUp * 0.2f * c.hoverCounter + baseTranslate;
			c.rotation = frontFaceingRotation;
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
			c.position = right * scale + camUp * 0.2f * c.hoverCounter + baseTranslate;
			c.rotation = frontFaceingRotation;
		}
	}

	needRegen = false;
}
void CardHand::Draw(const Camera& cam)
{
	static float idx = 0.0f;
	static bool countDown = false;
	//if (needRegen)
	GenTransformations(cam);
    
    if (countDown) idx = std::max(idx - 0.02f, 0.0f);
    else idx = std::min(idx + 0.02f, 1.0f);

	const glm::quat transRot = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::vec3 camUp = transRot * cam.GetRealUp();
	const glm::vec3 camFront = transRot * cam.GetFront();
	const glm::vec3 right = transRot * cam.GetRight();
	const glm::vec3 baseTranslate = transRot * cam.pos + (camFront + camUp * -1.0f);

    if(this->handID == GetGameState()->playerInTurn) RendererAddEffect(CARD_EFFECT_BLUR, baseTranslate, glm::quat(), 1.0f + idx * 0.1f, 1.0f+ idx * 0.1f, 0x8000FF00);

	if (idx == 0.0f) countDown = false;
	else if (idx == 1.0f) countDown = true;
	
	for (int i = 0; i < cards.size(); i++) {
		auto& c = cards.at(i);

		if (!c.visible) {
			continue;	// don't draw temporary cards
		}
		if (i == highlightedCardIdx)
		{
			float scale = g_cardScale * (1.3f + 0.1f * idx);
			RendererAddEffect(CARD_EFFECT::CARD_EFFECT_BLUR, c.position, c.rotation, scale, scale - 0.1f * g_cardScale, 0xFF00D7FF);
		}
		RendererAddCard(c.back, c.front, c.position, c.rotation, g_cardScale, g_cardScale);
	}
	
}








AnimatedCard::AnimatedCard(const CardStack& stack, CARD_ID front, const glm::vec3& pos, const glm::quat& rot, float dur, CARD_ANIMATIONS a, int cardID, int handID) {
	back = CARD_ID_BLANK; this->front = front; posAnim.AddState(pos, AnimationType::ANIMATION_LINEAR, dur); rotAnim.AddState(rot, AnimationType::ANIMATION_LINEAR, dur); this->cardID = cardID; this->handID = handID;
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
	if (posAnim.states.empty() || rotAnim.states.empty()) return;// first state should be set already
	
	glm::vec3 endPos = glm::vec3(0.0f, CardStack::dy * stack.cards.size(), 0.0f);

	float endYaw = (rand() % 360) * 1.0f;	// 0� -> 360�
	float endPitch = -90.0f;
	float endRoll = 0.0f;

	glm::vec3 middlePos = (endPos + posAnim.states.at(0).transform) / 2.0f + glm::vec3(0.0f, 0.4f, 0.0f);
	glm::quat middleRot = glm::rotate(glm::mat4(1.0f), glm::radians(endPitch / 2.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * 
		glm::rotate(glm::mat4(1.0f), glm::radians(endYaw / 2.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::quat endRot = glm::rotate(glm::mat4(1.0f), glm::radians(endPitch), glm::vec3(1.0f, 0.0f, 0.0f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(endYaw), glm::vec3(0.0f, 0.0f, 1.0f));



	posAnim.AddState(middlePos, AnimationType::ANIMATION_LINEAR, 0.15f);
	rotAnim.AddState(middleRot, AnimationType::ANIMATION_LINEAR, 0.15f);
	posAnim.AddState(endPos, AnimationType::ANIMATION_LINEAR, 0.3f);
	rotAnim.AddState(endRot, AnimationType::ANIMATION_LINEAR, 0.3f);
}
void AnimatedCard::AddDeckAnimation()
{
	type = CARD_ANIMATIONS::ANIM_FETCH_CARD;
	if (posAnim.states.empty() || rotAnim.states.empty()) return;
	glm::quat rot = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	posAnim.AddState(glm::vec3(CardDeck::dx, CardDeck::topY, 0.0f), AnimationType::ANIMATION_LINEAR, 0.0f);
	rotAnim.AddState(rot, AnimationType::ANIMATION_LINEAR, 0.0f);
	std::swap(posAnim.states.at(0), posAnim.states.at(1));
	std::swap(rotAnim.states.at(0), rotAnim.states.at(1));

	posAnim.states.at(1).duration = 0.4f;
	rotAnim.states.at(1).duration = 0.4f;
}



void CardsInAnimation::AddAnim(const CardStack& stack, const CardInfo& info, int handID, CARD_ANIMATIONS id)
{
	this->inputsAllowed = false;
	const float inv_scale = 1.0f / g_cardScale;
	list.emplace_back(stack, info.front, info.position, info.rotation, 0.0f, id, info.transitionID, handID);
}
void CardsInAnimation::Update(std::vector<CardHand>& hands, CardStack& stack, float dt)
{
	if (list.size() == 0) {
		inputsAllowed = true;
		return;
	}

	bool anyTrue = false;
	for (auto& a : list)
	{
		glm::vec3 curPos = a.posAnim.GetTransform(dt, a.done);
		glm::quat curRot = a.rotAnim.GetTransform(dt, a.done);
		RendererAddCard(a.back, a.front, curPos, curRot, g_cardScale, g_cardScale);
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
				OnFinish(hands, stack, a.posAnim.currentTransform, a.rotAnim.currentTransform, a.front, a.type, a.handID, a.cardID);
			}
		}
		list = std::move(newList);
	}
	if (list.size() == 0) inputsAllowed = true;
}

void CardsInAnimation::OnFinish(std::vector<CardHand>& hands, CardStack& stack, const glm::vec3& position, const glm::quat& rotation, CARD_ID id, CARD_ANIMATIONS type, int handID, int transitionID)
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
		stack.AddToStack(id, position, rotation);
	}
}


#define CardTranslationRow(color, last)  CardData(CARD_VAL_0, color),CardData(CARD_VAL_1, color),CardData(CARD_VAL_2, color),CardData(CARD_VAL_3, color),CardData(CARD_VAL_4, color),CardData(CARD_VAL_5, color),\
										 CardData(CARD_VAL_6, color),CardData(CARD_VAL_7, color),CardData(CARD_VAL_8, color),CardData(CARD_VAL_9, color),CardData(CARD_PAUSE, color),CardData(CARD_FLIP, color),CardData(CARD_PULL_2, color),CardData(last, CARD_COLOR_BLACK)

static constexpr CardData translationTable[56] = {
	CardTranslationRow(CARD_COLOR_RED, CARD_CHOOSE_COLOR),
	CardTranslationRow(CARD_COLOR_YELLOW, CARD_PULL_4),
	CardTranslationRow(CARD_COLOR_GREEN, CARD_UNKNOWN),
	CardTranslationRow(CARD_COLOR_BLUE, CARD_UNKNOWN),
};
CardData RenderCardToCardData(CARD_ID id, COLOR_ID optionalColor)
{
	if (id >= NUM_AVAILABLE_CARDS) return CardData(CARD_UNKNOWN, CARD_COLOR_UNKOWN);

	CardData out = translationTable[id];
	if (out.color == CARD_COLOR_BLACK) {
		if (out.face == CARD_UNKNOWN) out.color = CARD_COLOR_UNKOWN;
		else out.color = (CardColor)optionalColor;
	}

	return out;
}

void CardDataToRenderCard(CardData in, CARD_ID& outID, COLOR_ID& outColor)
{
	for (int i = 0; i < sizeof(translationTable); i++)
	{
		if (translationTable[i].face == in.face)
		{
			if (in.face == CARD_CHOOSE_COLOR || in.face == CARD_PULL_4)
			{
				outID = (CARD_ID)i;
				outColor = (COLOR_ID)in.color;
				return;
			}
			else
			{
				if (translationTable[i].color == in.color) {
					outID = (CARD_ID)i;
					outColor = (COLOR_ID)translationTable[i].color;
					return;
				}
			}
		}
	}

	outID = CARD_ID::CARD_ID_BLANK;
	outColor = COLOR_ID::COLOR_INVALID;
}

