#include "Renderer.h"
#include "Helper.h"
#include "logging.h"
#include "BloomRendering.h"
#include "AmbientOcclusionRendering.h"
#include <random>
#include <cmath>


enum INTERMEDIATE_TEXTURE_FLAGS
{
	INTERMEDIATE_TEXTURE_RED_FLAG = 1,
	INTERMEDIATE_TEXTURE_GREEN_FLAG = 1 << 1,
	INTERMEDIATE_TEXTURE_BLUE_FLAG = 1 << 2,
	INTERMEDIATE_TEXTURE_ALPHA_FLAG = 1 << 3,
};
IntermediateTextureData IntermediateTextureData::Create(int requiredX, int requiredY, uint8_t requiredPrecision, bool reqR, bool reqG, bool reqB, bool reqA)
{
	IntermediateTextureData out;
	out.rgbaFlags = 0;
	if (reqR) out.rgbaFlags |= INTERMEDIATE_TEXTURE_RED_FLAG;
	if (reqG) out.rgbaFlags |= INTERMEDIATE_TEXTURE_GREEN_FLAG;
	if (reqB) out.rgbaFlags |= INTERMEDIATE_TEXTURE_BLUE_FLAG;
	if (reqA) out.rgbaFlags |= INTERMEDIATE_TEXTURE_ALPHA_FLAG;

	out.sizeX = requiredX;
	out.sizeY = requiredY;
	out.precision = requiredPrecision;
	

	GLenum internalFmt = out.precision > 8 ? GL_R32F : GL_R8;
	if ((out.rgbaFlags & 0xFF) != INTERMEDIATE_TEXTURE_RED_FLAG)
	{
		internalFmt = out.precision > 8 ? GL_RGBA32F : GL_RGBA8;
		out.precision = 32;
		out.rgbaFlags |= INTERMEDIATE_TEXTURE_RED_FLAG;
		out.rgbaFlags |= INTERMEDIATE_TEXTURE_GREEN_FLAG;
		out.rgbaFlags |= INTERMEDIATE_TEXTURE_BLUE_FLAG;
		out.rgbaFlags |= INTERMEDIATE_TEXTURE_ALPHA_FLAG;
	}

	glGenFramebuffers(1, &out.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, out.fbo);

	glGenTextures(1, &out.texture);
	glBindTexture(GL_TEXTURE_2D, out.texture);
	glTexStorage2D(GL_TEXTURE_2D, 1, internalFmt, requiredX, requiredY);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, out.texture, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		LOG("FAILED TO CREATE INTERMEDIATE FBO\n");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, GetScreenFramebuffer());
	return out;
}
void IntermediateTextureData::CleanUp()
{
	sizeX = -1; sizeY = -1; precision = 0; rgbaFlags = 0;
	glDeleteFramebuffers(1, &fbo);
	glDeleteTextures(1, &texture);
}

struct RendererBackendData
{
	StandardRenderPassData mainData;
	ScenePointLight** pointLightTemp = nullptr;
	SceneDirLight** dirLightTemp = nullptr;
	ObjectRenderStruct* objs = nullptr;
	int capacityPointLights = 0;
	int capacityDirLights = 0;

	CurrentLightInformation cur;

	GLuint lightDataUniform;

	GLuint whiteTexture;
	GLuint blackTexture;

	IntermediateTextureData intermediate; // usually for blur
};
static RendererBackendData* g_render = nullptr;

GLuint GetWhiteTexture2D()
{
	return g_render->whiteTexture;
}
GLuint GetBlackTexture2D()
{
	return g_render->blackTexture;
}
const IntermediateTextureData* GetIntermediateTexture(int requiredX, int requiredY, uint8_t requiredPrecision, bool reqR, bool reqG, bool reqB, bool reqA)
{
	uint8_t flag = 0;
	if (reqR) flag |= INTERMEDIATE_TEXTURE_RED_FLAG;
	if (reqG) flag |= INTERMEDIATE_TEXTURE_GREEN_FLAG;
	if (reqB) flag |= INTERMEDIATE_TEXTURE_BLUE_FLAG;
	if (reqA) flag |= INTERMEDIATE_TEXTURE_ALPHA_FLAG;

	if (g_render->intermediate.sizeX < requiredX || g_render->intermediate.sizeY < requiredY || g_render->intermediate.precision < requiredPrecision || (g_render->intermediate.rgbaFlags & flag) != flag)
	{
		if (g_render->intermediate.sizeX >= 0 && g_render->intermediate.sizeY >= 0) g_render->intermediate.CleanUp();
		g_render->intermediate = IntermediateTextureData::Create(requiredX, requiredY, requiredPrecision, reqR, reqG, reqB, reqA);
	}
	return &g_render->intermediate;
}


static constexpr int RENDER_ALLOC_STEPS = 10;
static void UpdateTemporary(const StandardRenderPassData* data)
{
	memcpy(&g_render->mainData, data, sizeof(StandardRenderPassData));
	g_render->mainData.lightData = g_render->lightDataUniform;
	if (data->ambientOcclusionMap == 0) g_render->mainData.ambientOcclusionMap = g_render->whiteTexture;
	if (data->shadowMap == 0) g_render->mainData.shadowMap = g_render->whiteTexture;
}
static void ReflectVectorAtPlane(glm::vec3* reflecting, const glm::vec4* plane)
{
	assert(reflecting != nullptr && plane != nullptr);
	glm::vec3 planeNormal = glm::normalize(glm::vec3(*plane));
	*reflecting = *reflecting - 2.0f * glm::dot(*reflecting, planeNormal) * planeNormal;
}
static void MirrorVectorAtPlane(glm::vec3* mirrorPos, const glm::vec4* plane)
{
	assert(mirrorPos != nullptr && plane != nullptr);
	glm::vec3 planeNormal = glm::normalize(glm::vec3(*plane));
	*mirrorPos = *mirrorPos - 2 * (glm::dot(*mirrorPos, planeNormal) + plane->w) * planeNormal;
}
static void UpdateLightUniformFromCurrent()
{
	CurrentLightInformation* c = &g_render->cur;
	LightUniformData u;
	memset(&u, 0, sizeof(LightUniformData));
	u.numDirLights = c->numCurDirLights;
	u.numPointLights = c->numCurPointLights;
	bool shadowFound = false;
	for (int i = 0; i < c->numCurDirLights; i++)
	{
		u.dirLights[i] = c->curDirLights[i].data;
	}
	for (int i = 0; i < c->numCurPointLights; i++)
	{
		u.pointLights[i] = c->curPointLights[i].data;
	}
	glBindBuffer(GL_UNIFORM_BUFFER, g_render->lightDataUniform);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(LightUniformData), &u, GL_DYNAMIC_DRAW);
}

static void UpdateLightInformation(const glm::vec3* middle, uint16_t lightGroups, const glm::vec4* reflectPlane)
{
	CurrentLightInformation* cl = &g_render->cur;
	CurrentLightInformation lInfo;
	memset(&lInfo, 0, sizeof(CurrentLightInformation));
	int numFittingLights = 0;
	SceneDirLight* curDirLight = g_render->dirLightTemp[0];
	int idx = 0;
	while (curDirLight && idx < MAX_NUM_LIGHTS && numFittingLights < MAX_NUM_LIGHTS)
	{
		if (curDirLight->group & lightGroups)
		{
			lInfo.curDirLights[lInfo.numCurDirLights] = *curDirLight;
			if (reflectPlane)
			{
				ReflectVectorAtPlane(&lInfo.curDirLights[lInfo.numCurDirLights].data.dir, reflectPlane);
			}
			lInfo.numCurDirLights++;
			numFittingLights++;
		}
		idx++;
		curDirLight = g_render->dirLightTemp[idx];
	}
	ScenePointLight* curPointLight = g_render->pointLightTemp[0];
	idx = 0;
	while (curPointLight && idx < MAX_NUM_LIGHTS && numFittingLights < MAX_NUM_LIGHTS)
	{
		if (curPointLight->group & lightGroups)
		{
			float dist = glm::length(*middle - curPointLight->data.pos);
			float attenuation = 1.0f / (curPointLight->data.constant + sqrtf(dist) * curPointLight->data.linear + dist * curPointLight->data.quadratic);
			//if (attenuation > 0.1f)
			{
				lInfo.curPointLights[lInfo.numCurPointLights] = *curPointLight;
				if (reflectPlane)
				{
					MirrorVectorAtPlane(&lInfo.curPointLights[lInfo.numCurPointLights].data.pos, reflectPlane);
				}
				lInfo.numCurPointLights += 1;
				numFittingLights++;
			}
		}
		idx++;
		curPointLight = g_render->pointLightTemp[idx];
	}
	if (memcmp(cl, &lInfo, sizeof(CurrentLightInformation)) != 0)
	{
		memcpy(cl, &lInfo, sizeof(CurrentLightInformation));
		UpdateLightUniformFromCurrent();
	}
}





void InitializeRendererBackendData()
{
	if(!g_render) g_render = new RendererBackendData;
	glGenBuffers(1, &g_render->lightDataUniform);

	glGenTextures(1, &g_render->whiteTexture);
	glBindTexture(GL_TEXTURE_2D, g_render->whiteTexture);
	uint32_t color[2] = { 0xFFFFFFFF, 0xFFFFFFFF };
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, color);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glGenTextures(1, &g_render->blackTexture);
	glBindTexture(GL_TEXTURE_2D, g_render->blackTexture);
	color[0] = 0x0; color[1] = 0x0;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &color);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	g_render->intermediate.sizeX = -1; g_render->intermediate.sizeY = -1;

}
void CleanUpRendererBackendData()
{
	if (g_render)
	{
		glDeleteBuffers(1, &g_render->lightDataUniform);
		if (g_render->dirLightTemp) delete[] g_render->dirLightTemp;
		if (g_render->pointLightTemp) delete[] g_render->pointLightTemp;

		glDeleteTextures(1, &g_render->whiteTexture);
		glDeleteTextures(1, &g_render->blackTexture);

		delete g_render;
		g_render = nullptr;
	}
}












enum InternalSceneRenderDataInfo
{
	INTERNAL_SCENE_RENDER_DATA_HAS_BLOOM = 1,
	INTERNAL_SCENE_RENDER_DATA_HAS_AMBIENT_OCCLUSION = 1<<1,
	INTERNAL_SCENE_RENDER_DATA_HAS_AMBIENT_SHADOW = 1<<2,
};
void SceneRenderData::Create(int width, int height, int shadowWidth, int shadowHeight, uint8_t msaaQuality, bool useAmbientOcclusion, bool useBloom)
{
	GLenum requiredColorFormat = GL_RGBA8;
	_internal_msaa_quality = std::min(msaaQuality, (uint8_t)16);
#ifdef ANDROID
	_internal_msaa_quality = 0;
#endif
	baseWidth = width;
	baseHeight = height;
	this->shadowWidth = shadowWidth;
	this->shadowHeight = shadowHeight;
	if (useBloom) {
		requiredColorFormat = GL_RGBA16F;
		_internal_flags |= INTERNAL_SCENE_RENDER_DATA_HAS_BLOOM;
		bloomFBO.Create(std::max(width / 2, 1), std::max(height / 2, 1));
	}
	if (useAmbientOcclusion)
	{
		_internal_flags |= INTERNAL_SCENE_RENDER_DATA_HAS_AMBIENT_OCCLUSION;
		this->aoFBO = CreateSingleFBO(width, height, GL_R8, GL_RED, GL_DEPTH_COMPONENT32F, 0);
	}
	if (_internal_msaa_quality > 0)
	{
		msaaFBO = CreateSingleFBO(width, height, requiredColorFormat, GL_RGBA, GL_DEPTH_COMPONENT32F, _internal_msaa_quality);
	}
	if (shadowWidth > 0 && shadowHeight > 0) // negative number is NO SHADOWS AT ALL
	{
		shadowFBO = CreateDepthFBO(shadowWidth, shadowHeight);
	}

	if (_internal_flags & (INTERNAL_SCENE_RENDER_DATA_HAS_BLOOM | INTERNAL_SCENE_RENDER_DATA_HAS_AMBIENT_OCCLUSION)) // has Post Processing effect
	{
		// create Post-Processing-FBO
		glGenFramebuffers(1, &ppFBO.fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, ppFBO.fbo);
		glGenTextures(1, &ppFBO.texture);
		glBindTexture(GL_TEXTURE_2D, ppFBO.texture);
		glTexStorage2D(GL_TEXTURE_2D, 1, requiredColorFormat, width, height);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ppFBO.texture, 0);
		

		if (_internal_msaa_quality == 0)
		{
			glGenTextures(1, &ppFBO.maybeDepth);
			glBindTexture(GL_TEXTURE_2D, ppFBO.maybeDepth);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, ppFBO.maybeDepth, 0);
		}

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			LOG("FAILED  TO CREATE POST PROCESSING FBO\n");
		}

		glBindFramebuffer(GL_FRAMEBUFFER, GetScreenFramebuffer());
	}


}
void SceneRenderData::Recreate(int width, int height, int shadowWidth, int shadowHeight)
{
	CleanUp();
	Create(width, height, shadowWidth, shadowHeight, _internal_msaa_quality, _internal_flags & INTERNAL_SCENE_RENDER_DATA_HAS_AMBIENT_OCCLUSION, _internal_flags & INTERNAL_SCENE_RENDER_DATA_HAS_BLOOM);
}
void SceneRenderData::CleanUp()
{
	if (_internal_msaa_quality)
	{
		DestroySingleFBO(&msaaFBO);
	}
	if (_internal_flags & INTERNAL_SCENE_RENDER_DATA_HAS_AMBIENT_OCCLUSION)
	{
		DestroySingleFBO(&aoFBO);
	}
	if (_internal_flags & INTERNAL_SCENE_RENDER_DATA_HAS_BLOOM)
	{
		bloomFBO.CleanUp();
	}
	if (_internal_flags & (INTERNAL_SCENE_RENDER_DATA_HAS_BLOOM | INTERNAL_SCENE_RENDER_DATA_HAS_AMBIENT_OCCLUSION)) 	// has Post Processing effect
	{
		glDeleteFramebuffers(1, &ppFBO.fbo);
		glDeleteTextures(1, &ppFBO.texture);
		if (_internal_msaa_quality == 0)
		{
			glDeleteTextures(1, &ppFBO.maybeDepth);
		}
	}
	if (shadowWidth > 0 && shadowHeight > 0)
	{
		DestroyDepthFBO(&shadowFBO);
	}
}
GLuint SceneRenderData::GetFramebuffer() const
{
	if (_internal_msaa_quality > 0)
	{
		return msaaFBO.fbo;
	}
	else if (_internal_flags & (INTERNAL_SCENE_RENDER_DATA_HAS_BLOOM | INTERNAL_SCENE_RENDER_DATA_HAS_AMBIENT_OCCLUSION)) 	// has Post Processing effect
	{
		return ppFBO.fbo;
	}
	else
	{
		return GetMainFramebuffer();
	}
}
glm::ivec2 SceneRenderData::GetFramebufferSize() const
{
	if (_internal_msaa_quality > 0)
	{
		return { baseWidth, baseHeight };
	}
	else if (_internal_flags & (INTERNAL_SCENE_RENDER_DATA_HAS_BLOOM | INTERNAL_SCENE_RENDER_DATA_HAS_AMBIENT_OCCLUSION)) 	// has Post Processing effect
	{
		return { baseWidth, baseHeight };
	}
	else
	{
		return GetMainFramebufferSize();
	}
}
void SceneRenderData::MakeMainFramebuffer()
{
	if (_internal_msaa_quality > 0)
	{
		SetMainFramebuffer(msaaFBO.fbo, { baseWidth, baseHeight });
	}
	else if (_internal_flags & (INTERNAL_SCENE_RENDER_DATA_HAS_BLOOM | INTERNAL_SCENE_RENDER_DATA_HAS_AMBIENT_OCCLUSION)) 	// has Post Processing effect
	{
		SetMainFramebuffer(ppFBO.fbo, { baseWidth, baseHeight });
	}
	else
	{
		SetMainFramebuffer(GetScreenFramebuffer(), GetScreenFramebufferSize());
	}
}









void BeginScene(PScene scene)
{
	g_render->objs = SC_GenerateRenderList(scene);

	int numLights = SC_GetNumLights(scene);
	if (g_render->capacityDirLights <= numLights)
	{
		if (g_render->dirLightTemp) delete[] g_render->dirLightTemp;
		g_render->capacityDirLights = numLights + 10;
		g_render->dirLightTemp = new SceneDirLight*[g_render->capacityDirLights];
	}
	if (g_render->capacityPointLights <= numLights)
	{
		g_render->capacityPointLights = numLights + 10;
		if (g_render->pointLightTemp) delete[] g_render->pointLightTemp;
		g_render->pointLightTemp = new ScenePointLight*[g_render->capacityPointLights];
	}
	memset(g_render->dirLightTemp, 0, sizeof(SceneDirLight*) * g_render->capacityDirLights);
	memset(g_render->pointLightTemp, 0, sizeof(ScenePointLight*) * g_render->capacityPointLights);
	memset(&g_render->cur, 0, sizeof(CurrentLightInformation));
	SC_FillLights(scene, g_render->pointLightTemp, g_render->dirLightTemp);
}
void EndScene()
{
	SC_FreeRenderList(g_render->objs);
}

void RenderAmbientOcclusion(PScene scene, StandardRenderPassData* data, const SceneRenderData* frameData, float screenSizeX, float screenSizeY)
{
	GLuint mainFBO = frameData->GetFramebuffer();
	glm::ivec2 size = frameData->GetFramebufferSize();
	
	RenderSceneGeometry(scene, data);
	if (frameData->_internal_flags & INTERNAL_SCENE_RENDER_DATA_HAS_AMBIENT_OCCLUSION)
	{
		data->ambientOcclusionMap = frameData->aoFBO.texture;
		glm::ivec2 sao = { frameData->baseWidth, frameData->baseHeight };
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameData->aoFBO.fbo);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, mainFBO);
		glBlitFramebuffer(0, 0, size.x, size.y, 0, 0, sao.x, sao.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glViewport(0, 0, sao.x, sao.y);
		SetOpenGLWeakState(false, false);
		SetOpenGLDepthWrite(false);

		RenderAmbientOcclusionQuad(frameData->aoFBO.depth, screenSizeX, screenSizeY);

		const IntermediateTextureData* data = GetIntermediateTexture(screenSizeX, screenSizeY, 8, true, false, false, false);
		BlurTextureToFramebuffer(frameData->aoFBO.fbo, frameData->baseWidth, frameData->baseHeight, frameData->aoFBO.texture, 2.0f, data->fbo, data->texture);
		
	}
	else
	{
		data->ambientOcclusionMap = 0;
	}
}
void RenderSceneGeometry(PScene scene, const StandardRenderPassData* data)
{
	UpdateTemporary(data);
	SetDefaultOpaqueState();

	int num;
	glm::mat4 camViewProj = *g_render->mainData.camProj * *g_render->mainData.camView;
	SC_FillRenderList(scene, g_render->objs, &camViewProj, &num, TYPE_FUNCTION_GEOMETRY, SCENE_OBJECT_CAST_SHADOW);

	for (int i = 0; i < num; i++)
	{
		ObjectRenderStruct* o = &g_render->objs[i];
		o->DrawFunc(o->obj, (void*)&g_render->mainData);
	}
	SetOpenGLDepthWrite(true);
}
void RenderSceneShadow(PScene scene, const StandardRenderPassData* data)
{
	UpdateTemporary(data);
	SetDefaultOpaqueState();

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.0f, 4.0f);
	int num;
	glm::mat4 camViewProj = *g_render->mainData.camProj * *g_render->mainData.camView;
	SC_FillRenderList(scene, g_render->objs, &camViewProj, &num, TYPE_FUNCTION_SHADOW, SCENE_OBJECT_CAST_SHADOW);
	
	for (int i = 0; i < num; i++)
	{
		ObjectRenderStruct* o = &g_render->objs[i];
		BoundingBox* bbox = &o->obj->base.bbox;
		glm::vec3 middle = (bbox->leftTopFront + bbox->rightBottomBack) / 2.0f;
		o->DrawFunc(o->obj, (void*)&g_render->mainData);
	}
	glDisable(GL_POLYGON_OFFSET_FILL);
}
void RenderSceneCascadeShadow(PScene scene, const SceneRenderData* renderData, const Camera* cam, OrthographicCamera* orthoCam, DirectionalLightData* dirLight, const glm::vec2& regionStart, const glm::vec2& regionEnd, float endRelativeDist)
{
	g_render->mainData.camPos = &orthoCam->pos;
	g_render->mainData.camProj = &orthoCam->proj;
	g_render->mainData.camView = &orthoCam->view;
	g_render->mainData.camViewProj = &orthoCam->viewProj;
	g_render->mainData.cameraUniform = orthoCam->uniform;
	g_render->mainData.shadowMap = 0;
	g_render->mainData.ambientOcclusionMap = GetWhiteTexture2D();
	g_render->mainData.lightData = 0;
	SetDefaultOpaqueState();


	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.0f, 4.0f);
	int num;
	glm::mat4 camViewProj = *g_render->mainData.camProj * *g_render->mainData.camView;
	SC_FillRenderList(scene, g_render->objs, &camViewProj, &num, TYPE_FUNCTION_SHADOW, SCENE_OBJECT_CAST_SHADOW);
	
	const glm::vec2 stepSize = (regionEnd - regionStart) / 2.0f;

	const glm::ivec2 vpStart = glm::ivec2(regionStart.x * renderData->shadowWidth, regionStart.y * renderData->shadowHeight);
	const glm::ivec2 vpStep = glm::ivec2(stepSize.x * renderData->shadowWidth, stepSize.y * renderData->shadowHeight);
	g_render->mainData.renderSize = vpStep;

	float lastSplitDist = 0.0f;
	for (int i = 0; i < MAX_NUM_SHADOW_CASCADE_MAPS; i++)	// use 4 cascades by default
	{
		const int x = i % 2;
		const int y = i / 2;
		const int xStart = vpStart.x + x * vpStep.x;
		const int yStart = vpStart.y + y * vpStep.y;

		glViewport(xStart, yStart, vpStep.x, vpStep.y);
		
		
		cam->SetTightFit(orthoCam, dirLight->dir, lastSplitDist, lastSplitDist + endRelativeDist / 4.0f, &dirLight->cascadeSplits[i]);
		dirLight->mapper[i].start = regionStart + glm::vec2(x * stepSize.x, y * stepSize.y);
		dirLight->mapper[i].end = dirLight->mapper[i].start + stepSize;
		dirLight->mapper[i].viewProj = orthoCam->viewProj;

		for (int j = 0; j < num; j++)
		{
			ObjectRenderStruct* o = &g_render->objs[j];
			o->DrawFunc(o->obj, (void*)&g_render->mainData);
		}

		lastSplitDist += endRelativeDist / 4.0f;
	}
	dirLight->numCascades = 4;



	glDisable(GL_POLYGON_OFFSET_FILL);
}

void RenderSceneReflectedOnPlane(PScene scene, const ReflectPlanePassData* data)
{
	UpdateTemporary(data->base);
	SetDefaultOpaqueState();

	ReflectPlanePassData reflectPassData;
	reflectPassData.base = &g_render->mainData;
	reflectPassData.planeEquation = data->planeEquation;

	int num;
	glm::mat4 camViewProj = *g_render->mainData.camProj * *g_render->mainData.camView;
	SC_FillRenderList(scene, g_render->objs, &camViewProj, &num, TYPE_FUNCTION::TYPE_FUNCTION_CLIP_PLANE_OPAQUE, SCENE_OBJECT_FLAGS::SCENE_OBJECT_SURFACE_REFLECTED);
	SC_AppendRenderList(scene, g_render->objs, &camViewProj, &num, TYPE_FUNCTION::TYPE_FUNCTION_CLIP_PLANE_BLEND, SCENE_OBJECT_FLAGS::SCENE_OBJECT_SURFACE_REFLECTED, num);

	DrawSkybox(data->base->skyBox, camViewProj);
	for (int i = 0; i < num; i++)
	{
		ObjectRenderStruct* o = &g_render->objs[i];
		BoundingBox* bbox = &o->obj->base.bbox;
		glm::vec3 middle = (bbox->leftTopFront + bbox->rightBottomBack) / 2.0f;
		UpdateLightInformation(&middle, o->obj->base.lightGroups, data->planeEquation);
		o->DrawFunc(o->obj, (void*)&reflectPassData);
	}
	SetOpenGLDepthWrite(true);
}

void RenderSceneStandard(PScene scene, const StandardRenderPassData* data)
{
	UpdateTemporary(data);
	SetDefaultOpaqueState();

	int num;
	glm::mat4 camViewProj = *g_render->mainData.camProj * *g_render->mainData.camView;
	SC_FillRenderList(scene, g_render->objs, &camViewProj, &num, TYPE_FUNCTION::TYPE_FUNCTION_OPAQUE, SCENE_OBJECT_FLAGS::SCENE_OBJECT_OPAQUE);
	SC_AppendRenderList(scene, g_render->objs, &camViewProj, &num, TYPE_FUNCTION::TYPE_FUNCTION_BLEND, SCENE_OBJECT_FLAGS::SCENE_OBJECT_BLEND, num);

	DrawSkybox(data->skyBox, camViewProj);
	for (int i = 0; i < num; i++)
	{
		ObjectRenderStruct* o = &g_render->objs[i];
		BoundingBox* bbox = &o->obj->base.bbox;
		glm::vec3 middle = (bbox->leftTopFront + bbox->rightBottomBack) / 2.0f;
		UpdateLightInformation(&middle, o->obj->base.lightGroups, nullptr);
		o->DrawFunc(o->obj, (void*)&g_render->mainData);
	}
	SetOpenGLDepthWrite(true);
}
void RenderPostProcessingBloom(const BloomFBO* bloomData, GLuint finalFBO, int finalSizeX, int finalSizeY, GLuint ppFBOTexture, int ppSizeX, int ppSizeY)
{
	SetOpenGLWeakState(false, false);
	float blurRadius = 4.0f;
	float intensity = 1.0f;
	if (bloomData->numBloomFbos == 0) return;
	glm::ivec2 fboSizes[MAX_BLOOM_MIPMAPS];
	BloomTextureToFramebuffer(bloomData->bloomFBOs1[0], bloomData->sizeX, bloomData->sizeY, bloomData->bloomFBOs2[0], bloomData->sizeX, bloomData->sizeY, bloomData->bloomTexture2, ppFBOTexture, blurRadius, intensity, 0, 0);
	
	fboSizes[0] = { bloomData->sizeX, bloomData->sizeY };
	int curSizeX = bloomData->sizeX;
	int curSizeY = bloomData->sizeY;
	for (int i = 1; i < bloomData->numBloomFbos; i++)
	{
		curSizeX = std::max(curSizeX >> 1, 1); curSizeY = std::max(curSizeY >> 1, 1);
		fboSizes[i] = { curSizeX, curSizeY };
		BloomTextureToFramebuffer(bloomData->bloomFBOs1[i], curSizeX, curSizeY, bloomData->bloomFBOs2[i], curSizeX, curSizeY, bloomData->bloomTexture2, bloomData->bloomTexture1, blurRadius, 0.0f, i - 1, i);
	}
	SetOpenGLWeakState(false, true);
	glBlendFuncWrapper(GL_ONE, GL_ONE);
	for (int i = bloomData->numBloomFbos - 2; i >= 0; i--)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, bloomData->bloomFBOs1[i]);
		glViewport(0, 0, fboSizes[i].x, fboSizes[i].y);
	
		UpsampleTextureToFramebuffer(bloomData->bloomTexture1, i + 1);
	}
	
	SetOpenGLWeakState(false, false);
	
	glBindFramebuffer(GL_FRAMEBUFFER, finalFBO);
	glViewport(0, 0, finalSizeX, finalSizeY);
	CopyTexturesToFramebuffer(bloomData->bloomTexture1, 0, ppFBOTexture, 0);
}
void RenderPostProcessing(const SceneRenderData* rendererData, GLuint finalFBO, int finalSizeX, int finalSizeY)
{
	if (rendererData->_internal_msaa_quality > 0)
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rendererData->ppFBO.fbo);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, rendererData->msaaFBO.fbo);
		glBlitFramebuffer(0, 0, rendererData->baseWidth, rendererData->baseHeight, 0, 0, rendererData->baseWidth, rendererData->baseHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}
	else if(!(rendererData->_internal_flags & (INTERNAL_SCENE_RENDER_DATA_HAS_BLOOM | INTERNAL_SCENE_RENDER_DATA_HAS_AMBIENT_OCCLUSION)))	// has no Post Processing
	{
		return;
	}
	RenderPostProcessingBloom(&rendererData->bloomFBO, GetScreenFramebuffer(), finalSizeX, finalSizeY, rendererData->ppFBO.texture, rendererData->baseWidth, rendererData->baseHeight);
}


const CurrentLightInformation* GetCurrentLightInformation()
{
	return &g_render->cur;
}