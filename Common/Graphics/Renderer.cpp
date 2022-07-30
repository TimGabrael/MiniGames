#include "Renderer.h"
#include "Helper.h"
#include "logging.h"
#include "BloomRendering.h"



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


static constexpr int RENDER_ALLOC_STEPS = 10;
static void UpdateTemporary(const StandardRenderPassData* data)
{
	memcpy(&g_render->mainData, data, sizeof(StandardRenderPassData));
	g_render->mainData.lightData = g_render->lightDataUniform;
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
			if (attenuation > 0.1f)
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
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, width, height);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ppFBO.texture, 0);
		

		if (_internal_msaa_quality == 0)
		{
			glGenTextures(1, &ppFBO.maybeDepth);
			glBindTexture(GL_TEXTURE_2D, ppFBO.maybeDepth);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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
void RenderAmbientOcclusion(PScene scene, const StandardRenderPassData* data, const SceneRenderData* frameData)
{
	UpdateTemporary(data);
	
	glBindFramebuffer(GL_FRAMEBUFFER, frameData->aoFBO.fbo);
	glViewport(0, 0, frameData->baseWidth, frameData->baseHeight);
	glClearDepthf(1.0f);
	glClear(GL_DEPTH_BUFFER_BIT);
	RenderSceneGeometry(scene, data);

	// TODO: do the actual computation

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
		BoundingBox* bbox = &o->obj->base.bbox;
		glm::vec3 middle = (bbox->leftTopFront + bbox->rightBottomBack) / 2.0f;
		o->DrawFunc(o->obj, (void*)&g_render->mainData);
	}
}
void RenderSceneShadow(PScene scene, const StandardRenderPassData* data)
{
	UpdateTemporary(data);
	SetDefaultOpaqueState();

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.0f, 4.0f);
	int num;
	glm::mat4 camViewProj = *g_render->mainData.camProj * *g_render->mainData.camView;
	SC_FillRenderList(scene, g_render->objs, &camViewProj, &num, TYPE_FUNCTION_GEOMETRY, SCENE_OBJECT_CAST_SHADOW);
	
	for (int i = 0; i < num; i++)
	{
		ObjectRenderStruct* o = &g_render->objs[i];
		BoundingBox* bbox = &o->obj->base.bbox;
		glm::vec3 middle = (bbox->leftTopFront + bbox->rightBottomBack) / 2.0f;
		o->DrawFunc(o->obj, (void*)&g_render->mainData);
	}
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
	RenderPostProcessingBloom(&rendererData->bloomFBO, GetScreenFramebuffer(), rendererData->baseWidth, rendererData->baseHeight, rendererData->ppFBO.texture, rendererData->baseWidth, rendererData->baseHeight);
}


const CurrentLightInformation* GetCurrentLightInformation()
{
	return &g_render->cur;
}