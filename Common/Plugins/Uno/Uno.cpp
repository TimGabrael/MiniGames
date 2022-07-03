#include "Uno.h"
#include <string>
#include "Graphics/Helper.h"
#include "Graphics/Camera.h"
#include "logging.h" // REMINDER USE THIS !! USE THIS NOT <iostream> !!
#include "Graphics/PbrRendering.h"
#include "Graphics/UiRendering.h"
#include <chrono>
#include "Card.h"
#include "Graphics/Simple3DRendering.h"
#include "../InputStates.h"


#define _USE_MATH_DEFINES
#include <math.h>

PLUGIN_EXPORT_DEFINITION(UnoPlugin, "a3fV-6giK-10Eb-2rdT");

PLUGIN_INFO UnoPlugin::GetPluginInfos()
{
	PLUGIN_INFO plugInfo;
#ifdef _WIN32
	memcpy(plugInfo.ID, _Plugin_Export_ID_Value, strnlen_s(_Plugin_Export_ID_Value, 19));
#else
	memcpy(plugInfo.ID, _Plugin_Export_ID_Value, strnlen(_Plugin_Export_ID_Value, 19));
#endif
	plugInfo.previewResource = nullptr;
	return plugInfo;
}

void PrintGLMMatrix(const glm::mat4& m)
{
	LOG("%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n",
		m[0][0], m[1][0], m[2][0], m[3][0],
		m[0][1], m[1][1], m[2][1], m[3][1],
		m[0][2], m[1][2], m[2][2], m[3][2],
		m[0][3], m[1][3], m[2][3], m[3][3]);
}




glm::vec3 GetCamFront();glm::vec3 GetCamPos();glm::vec3 GetCamUp();glm::vec3 GetCamRight(); float GetCamYaw();
glm::vec3 GetMouseRay(); glm::vec2 GetCamClippingPlaneDimensions(float dist); bool IsMousePressed(); bool IsMouseReleased();
int GetMouseDeltaX();

static float g_cardScale = 0.3f;
static constexpr float g_cardMinDiff = 0.25f;
struct CardInfo
{
	CardInfo(CARD_ID back, CARD_ID front, const glm::mat4& mat, float hov) : back(back),front(front),transform(mat),hoverCounter(hov) {};
	CARD_ID back; CARD_ID front;
	glm::mat4 transform;
	float hoverCounter = 0.0f;
};
struct CardHand
{
	std::vector<CardInfo> cards;
	int highlightedCardIdx = -1;


	// used when the number of cards on hand are overflowing the visible capacity
	float maxWidth = 0.0f;
	float wideCardsStart = -1.0f;
	
	int mouseSelectedCard = -1;
	bool mouseAttached = false;

	bool needRegen = false;
	void Add(CARD_ID id)
	{
		if (wideCardsStart >= 0.0f) wideCardsStart += g_cardMinDiff * 0.5f;

		cards.emplace_back(CARD_ID::CARD_ID_BLANK, id, glm::mat4(1.0f), 0.0f);
		

		needRegen = true;
	}
	void PlayCard(int cardIdx)
	{
		LOG("PLAYING CARD AT INDEX: %d\n", cardIdx);
	}
	void Update()
	{
		needRegen = true;
		if (needRegen) GenTransformations();
		glm::vec3 cp = GetCamPos();
		glm::vec3 mRay = GetMouseRay();

		if (mouseAttached) {
			static constexpr float pixelStepSize = 0.01f;
			wideCardsStart += pixelStepSize * GetMouseDeltaX();
		}
		int oldMouseSelectedCard = mouseSelectedCard;
		bool wasReleased = false;
		if (IsMouseReleased()) {
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
				if (IsMousePressed()) {
					mouseSelectedCard = hitIdx;
					mouseAttached = true;
				}
				break;
			}
		}
		if (wasReleased && oldMouseSelectedCard == hitIdx)
		{
			PlayCard(oldMouseSelectedCard);
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
	void GenTransformations()
	{
		static constexpr float maxDiff = 0.8f;
		if (!needRegen || cards.empty()) return;
		const glm::vec2 frustrum = GetCamClippingPlaneDimensions(1.0f);
		maxWidth = frustrum.x * 1.2f;


		glm::vec3 camUp = GetCamUp();
		glm::vec3 baseTranslate = GetCamPos() + GetCamFront() + camUp * -1.f;
		glm::vec3 right = GetCamRight();
		float camYaw = GetCamYaw() + 90.0f;
		glm::mat4 frontFaceingRotation = glm::rotate(glm::mat4(1.0f), glm::radians(camYaw), glm::vec3(0.0f, -1.0f, 0.0f));
		glm::mat4 std = glm::translate(glm::mat4(1.0f), baseTranslate) * glm::rotate(glm::mat4(1.0f), glm::radians(-40.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * frontFaceingRotation * glm::scale(glm::mat4(1.0f), glm::vec3(g_cardScale));
		
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
	void Draw()
	{
		ClearCards();
		if (needRegen) GenTransformations();
		for (auto& c : cards) {
			RendererAddCard(c.back, c.front, c.transform);
		}
	}
};



struct UnoGlobals
{
	Camera playerCam;
	GLuint skybox;
	S3DCombinedBuffer platform;
	CardHand client;
	MouseState ms;

}g_objs;
glm::vec3 GetCamFront() { return g_objs.playerCam.GetFront(); } glm::vec3 GetCamPos() { return g_objs.playerCam.pos; }
glm::vec3 GetCamUp() { return g_objs.playerCam.GetRealUp(); } glm::vec3 GetCamRight() { return g_objs.playerCam.GetRight(); }
float GetCamYaw() { return g_objs.playerCam.GetYaw(); }
glm::vec3 GetMouseRay() { return g_objs.playerCam.mouseRay; }
glm::vec2 GetCamClippingPlaneDimensions(float dist) { return g_objs.playerCam.GetFrustrumSquare(dist); }
bool IsMousePressed() { return g_objs.ms.butns[MouseState::BUTTONS::BTN_LEFT].Pressed(); }
bool IsMouseReleased() { return g_objs.ms.butns[MouseState::BUTTONS::BTN_LEFT].Released(); }
int GetMouseDeltaX() { return g_objs.ms.dx; }



//#define ALLOW_FREEMOVEMENT
void UnoPlugin::Init(void* backendData, PLATFORM_ID id)
{
	initialized = true;
	this->backendData = backendData;
	InitializeOpenGL(backendData);
	InitializeCardPipeline(backendData);

	static constexpr float cubeSize = 4.0f;
	SVertex3D platformVertices[4] = {
		{{-cubeSize, 0.0f, -cubeSize}, {0.0f, 0.0f}, 0xFF402040},
		{{ cubeSize, 0.0f, -cubeSize}, {0.0f, 0.0f}, 0xFF406040},
		{{ cubeSize, 0.0f,  cubeSize}, {0.0f, 0.0f}, 0xFF402040},
		{{-cubeSize, 0.0f,  cubeSize}, {0.0f, 0.0f}, 0xFF406040},
	};
	uint32_t platformIndices[] = {
		0,3,2,2,1,0,
	};
	g_objs.platform = S3DGenerateBuffer(platformVertices, sizeof(platformVertices)/sizeof(SVertex3D), platformIndices, sizeof(platformIndices)/sizeof(uint32_t));




	g_objs.playerCam.pos = { 0.0f, 1.6f, 2.0f };
	g_objs.playerCam.SetRotation(-90.0f, -40.0f, 0.0f);


	g_objs.skybox = LoadCubemap(
		"Assets/CitySkybox/right.jpg",
		"Assets/CitySkybox/left.jpg",
		"Assets/CitySkybox/top.jpg",
		"Assets/CitySkybox/bottom.jpg",
		"Assets/CitySkybox/front.jpg",
		"Assets/CitySkybox/back.jpg");


	g_objs.client.Add(CARD_ID::CARD_ID_ADD_4);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
}

void UnoPlugin::Resize(void* backendData)
{
	if(sizeY && sizeX)
		g_objs.playerCam.SetPerspective(90.0f, (float)this->sizeX / (float)this->sizeY, 0.1f, 256.0f);
	g_objs.playerCam.screenX = sizeX;
	g_objs.playerCam.screenY = sizeY;
}
void UnoPlugin::Render(void* backendData)
{
	if (!(sizeX && sizeY)) return;
	
	static auto prev = std::chrono::high_resolution_clock::now();
	auto now = std::chrono::high_resolution_clock::now();

	float dt = std::chrono::duration<float>(now - prev).count();
	prev = now;

	glViewport(0, 0, framebufferX, framebufferY);
	glClearColor(1.0f, 0.4f, 0.4f, 1.0f);
	glClearDepthf(1.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDepthFunc(GL_LESS);
	g_objs.playerCam.Update();

	g_objs.client.Update();




	g_objs.client.Draw();
	glDisable(GL_BLEND);

	DrawSimple3D(g_objs.platform, g_objs.playerCam.perspective, g_objs.playerCam.view);

	DrawSkybox(g_objs.skybox, g_objs.playerCam.view, g_objs.playerCam.perspective);



	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);



	DrawCards(g_objs.playerCam.perspective, g_objs.playerCam.view);
	DrawUI();

	glEnable(GL_DEPTH_TEST);

	g_objs.ms.FrameEnd();
}



void UnoPlugin::MouseCallback(const PB_MouseData* mData)
{
#ifdef ALLOW_FREEMOVEMENT
	if (mData->lPressed && (mData->dx || mData->dy))
	{
		g_objs.playerCam.UpdateFromMouseMovement(-mData->dx, mData->dy);
	}
#endif
	g_objs.ms.SetFromPBState(mData);
	auto& ray = g_objs.playerCam.mouseRay;
	ray = g_objs.playerCam.ScreenToWorld(mData->xPos, mData->yPos);
}
void UnoPlugin::KeyDownCallback(Key k, bool isRepeat)
{
#ifdef ALLOW_FREEMOVEMENT
	if (!isRepeat)
	{
		if (k == Key::Key_W)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::FORWARD, true);
		if (k == Key::Key_A)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::LEFT, true);
		if (k == Key::Key_S)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::BACKWARD, true);
		if (k == Key::Key_D)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::RIGHT, true);
	}
#endif
	if (k == Key::Key_0) g_objs.client.Add((CARD_ID)(rand() % (int)CARD_ID::CARD_ID_YELLOW_SWAP));
}
void UnoPlugin::KeyUpCallback(Key k, bool isRepeat)
{
#ifdef ALLOW_FREEMOVEMENT
	if (!isRepeat)
	{
		if (k == Key::Key_W)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::FORWARD, false);
		if (k == Key::Key_A)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::LEFT, false);
		if (k == Key::Key_S)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::BACKWARD, false);
		if (k == Key::Key_D)g_objs.playerCam.SetMovementDirection(Camera::DIRECTION::RIGHT, false);
	}
#endif
}

void UnoPlugin::TouchDownCallback(int x, int y, int touchID)
{
#ifdef ALLOW_FREEMOVEMENT
	g_objs.playerCam.UpdateTouch(x, y, touchID, false);
#endif
	auto& ray = g_objs.playerCam.mouseRay;
	ray = g_objs.playerCam.ScreenToWorld(x, y);
}
void UnoPlugin::TouchUpCallback(int x, int y, int touchID)
{
#ifdef ALLOW_FREEMOVEMENT
	g_objs.playerCam.UpdateTouch(x, y, touchID, true);
#endif
}
void UnoPlugin::TouchMoveCallback(int x, int y, int dx, int dy, int touchID)
{
#ifdef ALLOW_FREEMOVEMENT
	g_objs.playerCam.UpdateTouchMove(x, y, dx, dy, touchID);
#endif
}
void UnoPlugin::CleanUp()
{
	glDeleteTextures(1, &g_objs.skybox);
	CleanUpOpenGL();


	// ADD GLTF CLEANUP!!!
}