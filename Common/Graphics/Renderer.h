#pragma once
#include "Scene.h"
#include "Camera.h"
#include "Helper.h"




struct CameraData
{
	glm::mat4 projection;
	glm::mat4 view;
	glm::vec3 camPos;
};

struct CurrentLightInformation
{
	ScenePointLight curPointLights[MAX_NUM_LIGHTS];
	SceneDirLight curDirLights[MAX_NUM_LIGHTS];
	int numCurPointLights = 0;
	int numCurDirLights = 0;
	glm::vec4 reflectionPlane = { 0, 0, 0, 0 };
	bool isReflected = false;
};
struct IntermediateTextureData
{
	static IntermediateTextureData Create(int sx, int sy, uint8_t precision, bool hasR, bool hasG, bool hasB, bool hasA);
	void CleanUp();
	GLuint fbo;
	GLuint texture;
	int sizeX;
	int sizeY;
	uint8_t precision;
	uint8_t rgbaFlags;
};


void InitializeRendererBackendData();
void CleanUpRendererBackendData();

GLuint GetWhiteTexture2D();
GLuint GetBlackTexture2D();
const IntermediateTextureData* GetIntermediateTexture(int requiredX, int requiredY, uint8_t requiredPrecision, bool reqR, bool reqG, bool reqB, bool reqA);


struct PostProcessingFBO
{
	GLuint fbo = 0;
	GLuint texture = 0;
	GLuint maybeDepth = 0;	// might not be initialized, depending on the state of the SceneRenderData
};
struct SceneRenderData
{
	void Create(int width, int height, int shadowWidth, int shadowHeight, uint8_t msaaQuality, bool useAmbientOcclusion, bool useBloom);
	void Recreate(int width, int height, int shadowWidth, int shadowHeight);
	void CleanUp();

	// framebuffer all objects are rendered to
	GLuint GetFramebuffer() const;
	glm::ivec2 GetFramebufferSize() const;

	void MakeMainFramebuffer();

	SingleFBO msaaFBO;
	SingleFBO aoFBO;
	DepthFBO shadowFBO;
	PostProcessingFBO ppFBO;
	BloomFBO bloomFBO;
	int baseWidth = 0;
	int baseHeight = 0;
	int shadowWidth = 0;
	int shadowHeight = 0;
	uint8_t _internal_msaa_quality = 0;
	uint8_t _internal_flags = 0;
};



void BeginScene(PScene scene);
void EndScene();
void RenderAmbientOcclusion(PScene scene, StandardRenderPassData* data, const SceneRenderData* frameData, float screeSizeX, float screenSizeY);// will set the ao texture in data or 0 if no occlusion available
void RenderSceneGeometry(PScene scene, const StandardRenderPassData* data);
void RenderSceneShadow(PScene scene, const StandardRenderPassData* data);
void RenderSceneCascadeShadow(PScene scene, const SceneRenderData* renderData, const Camera* cam, OrthographicCamera* orthoCam, DirectionalLightData* dirLight, const glm::vec2& regionStart, const glm::vec2& regionEnd, float endRelativeDist);
void RenderSceneReflectedOnPlane(PScene scene, const ReflectPlanePassData* data);
void RenderSceneStandard(PScene scene, const StandardRenderPassData* data);
void RenderPostProcessingBloom(const struct BloomFBO* bloomData, GLuint finalFBO, int finalSizeX, int finalSizeY, GLuint ppFBOTexture, int ppSizeX, int ppSizeY);

void RenderPostProcessing(const SceneRenderData* rendererData, GLuint finalFBO, int finalSizeX, int finalSizeY);

const CurrentLightInformation* GetCurrentLightInformation();
