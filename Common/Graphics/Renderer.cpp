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
};

static RendererBackendData* g_render = nullptr;
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
}
void CleanUpRendererBackendData()
{
	if (g_render)
	{
		glDeleteBuffers(1, &g_render->lightDataUniform);
		if (g_render->dirLightTemp) delete[] g_render->dirLightTemp;
		if (g_render->pointLightTemp) delete[] g_render->pointLightTemp;
		delete g_render;
		g_render = nullptr;
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
void RenderSceneShadow(PScene scene, const StandardRenderPassData* data)
{
	UpdateTemporary(data);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

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
		UpdateLightInformation(&middle, o->obj->base.lightGroups, nullptr);
		o->DrawFunc(o->obj, (void*)&g_render->mainData);
	}
	glDisable(GL_POLYGON_OFFSET_FILL);
}

void RenderSceneReflectedOnPlane(PScene scene, const ReflectPlanePassData* data)
{
	UpdateTemporary(data->base);
	// ReflectVectorAtPlane(&g_render->mainData.light.dir, data->planeEquation);
	// for (int i = 0; i < g_render->mainData.numPointLights; i++)
	// {
	// 	MirrorVectorAtPlane(&g_render->mainData.pointLights[i].point, data->planeEquation);
	// }



	ReflectPlanePassData reflectPassData;
	reflectPassData.base = &g_render->mainData;
	reflectPassData.planeEquation = data->planeEquation;

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDisable(GL_BLEND);


	int num;
	glm::mat4 camViewProj = *g_render->mainData.camProj * *g_render->mainData.camView;
	SC_FillRenderList(scene, g_render->objs, &camViewProj, &num, TYPE_FUNCTION::TYPE_FUNCTION_CLIP_PLANE_OPAQUE, SCENE_OBJECT_FLAGS::SCENE_OBJECT_SURFACE_REFLECTED | SCENE_OBJECT_FLAGS::SCENE_OBJECT_OPAQUE);
	

	for (int i = 0; i < num; i++)
	{
		ObjectRenderStruct* o = &g_render->objs[i];
		BoundingBox* bbox = &o->obj->base.bbox;
		glm::vec3 middle = (bbox->leftTopFront + bbox->rightBottomBack) / 2.0f;
		UpdateLightInformation(&middle, o->obj->base.lightGroups, data->planeEquation);
		o->DrawFunc(o->obj, (void*)&reflectPassData);
	}
	DrawSkybox(data->base->skyBox, camViewProj);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	SC_FillRenderList(scene, g_render->objs, &camViewProj, &num, TYPE_FUNCTION::TYPE_FUNCTION_CLIP_PLANE_BLEND, SCENE_OBJECT_FLAGS::SCENE_OBJECT_SURFACE_REFLECTED | SCENE_OBJECT_FLAGS::SCENE_OBJECT_BLEND);
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

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDisable(GL_BLEND);

	int num;
	glm::mat4 camViewProj = *g_render->mainData.camProj * *g_render->mainData.camView;
	SC_FillRenderList(scene, g_render->objs, &camViewProj, &num, TYPE_FUNCTION::TYPE_FUNCTION_OPAQUE, SCENE_OBJECT_FLAGS::SCENE_OBJECT_OPAQUE);
	for (int i = 0; i < num; i++)
	{
		ObjectRenderStruct* o = &g_render->objs[i];
		BoundingBox* bbox = &o->obj->base.bbox;
		glm::vec3 middle = (bbox->leftTopFront + bbox->rightBottomBack) / 2.0f;
		UpdateLightInformation(&middle, o->obj->base.lightGroups, nullptr);
		o->DrawFunc(o->obj, (void*)&g_render->mainData);
	}
	DrawSkybox(data->skyBox, camViewProj);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	SC_FillRenderList(scene, g_render->objs, &camViewProj, &num, TYPE_FUNCTION::TYPE_FUNCTION_BLEND, SCENE_OBJECT_FLAGS::SCENE_OBJECT_BLEND);
	
	for (int i = 0; i < num; i++)
	{
		ObjectRenderStruct* o = &g_render->objs[i];
		BoundingBox* bbox = &o->obj->base.bbox;
		glm::vec3 middle = (bbox->leftTopFront + bbox->rightBottomBack) / 2.0f;
		UpdateLightInformation(&middle, o->obj->base.lightGroups, nullptr);
		o->DrawFunc(o->obj, (void*)&g_render->mainData);
	}
}
void RenderPostProcessingBloom(BloomFBO* bloomData, GLuint finalFBO, int finalSizeX, int finalSizeY)
{
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	float blurRadius = 4.0f;
	float intensity = 1.0f;
	if (bloomData->numBloomFbos == 0) return;
	glm::ivec2 fboSizes[MAX_BLOOM_MIPMAPS];
	BloomTextureToFramebuffer(bloomData->bloomFBOs1[0], bloomData->sizeX, bloomData->sizeY, bloomData->bloomFBOs2[0], bloomData->sizeX, bloomData->sizeY, bloomData->bloomTexture2, bloomData->defaultTexture, blurRadius, intensity, 0, 0);
	
	fboSizes[0] = { bloomData->sizeX, bloomData->sizeY };
	int curSizeX = bloomData->sizeX;
	int curSizeY = bloomData->sizeY;
	for (int i = 1; i < bloomData->numBloomFbos; i++)
	{
		curSizeX = std::max(curSizeX >> 1, 1); curSizeY = std::max(curSizeY >> 1, 1);
		fboSizes[i] = { curSizeX, curSizeY };
		BloomTextureToFramebuffer(bloomData->bloomFBOs1[i], curSizeX, curSizeY, bloomData->bloomFBOs2[i], curSizeX, curSizeY, bloomData->bloomTexture2, bloomData->bloomTexture1, blurRadius, 0.0f, i - 1, i);
	}
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	for (int i = bloomData->numBloomFbos - 2; i >= 0; i--)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, bloomData->bloomFBOs1[i]);
		glViewport(0, 0, fboSizes[i].x, fboSizes[i].y);
	
		UpsampleTextureToFramebuffer(bloomData->bloomTexture1, i + 1);
	}
	
	glDisable(GL_BLEND);
	
	glBindFramebuffer(GL_FRAMEBUFFER, finalFBO);
	glViewport(0, 0, finalSizeX, finalSizeY);

	CopyTexturesToFramebuffer(bloomData->bloomTexture1, 0, bloomData->defaultTexture, 0);
}


const CurrentLightInformation* GetCurrentLightInformation()
{
	return &g_render->cur;
}