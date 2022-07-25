#include "Renderer.h"
#include "Helper.h"
#include "logging.h"
#include "BloomRendering.h"

static ObjectRenderStruct* objs = nullptr;
void BeginScene(PScene scene)
{
	objs = SC_GenerateRenderList(scene);
}
void EndScene()
{
	SC_FreeRenderList(objs);
}
void RenderSceneShadow(PScene scene, const StandardRenderPassData* data)
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	int num;
	glm::mat4 camViewProj = *data->camProj * *data->camView;
	SC_FillRenderList(scene, objs, &camViewProj, &num, TYPE_FUNCTION_GEOMETRY, SCENE_OBJECT_CAST_SHADOW);
	
	for (int i = 0; i < num; i++)
	{
		objs[i].DrawFunc(objs[i].obj, (void*)data);
	}

	//glCullFace(GL_BACK);
}

void RenderSceneReflectedOnPlane(PScene scene, const ReflectPlanePassData* data)
{
	glEnableClipDistance(0);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDisable(GL_BLEND);


	int num;
	glm::mat4 camViewProj = *data->base->camProj * *data->base->camView;
	SC_FillRenderList(scene, objs, &camViewProj, &num, TYPE_FUNCTION::TYPE_FUNCTION_CLIP_PLANE_OPAQUE, SCENE_OBJECT_FLAGS::SCENE_OBJECT_SURFACE_REFLECTED | SCENE_OBJECT_FLAGS::SCENE_OBJECT_OPAQUE);
	

	for (int i = 0; i < num; i++)
	{
		objs[i].DrawFunc(objs[i].obj, (void*)data);
	}
	DrawSkybox(data->base->skyBox, camViewProj);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	SC_FillRenderList(scene, objs, &camViewProj, &num, TYPE_FUNCTION::TYPE_FUNCTION_CLIP_PLANE_BLEND, SCENE_OBJECT_FLAGS::SCENE_OBJECT_SURFACE_REFLECTED | SCENE_OBJECT_FLAGS::SCENE_OBJECT_BLEND);
	for (int i = 0; i < num; i++)
	{
		objs[i].DrawFunc(objs[i].obj, (void*)data);
	}

	glDisableClipDistance(0);
}

void RenderSceneStandard(PScene scene, const StandardRenderPassData* data)
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDisable(GL_BLEND);

	int num;
	glm::mat4 camViewProj = *data->camProj * *data->camView;
	SC_FillRenderList(scene, objs, &camViewProj, &num, TYPE_FUNCTION::TYPE_FUNCTION_OPAQUE, SCENE_OBJECT_FLAGS::SCENE_OBJECT_OPAQUE);
	for (int i = 0; i < num; i++)
	{
		objs[i].DrawFunc(objs[i].obj, (void*)data);
	}
	DrawSkybox(data->skyBox, camViewProj);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	SC_FillRenderList(scene, objs, &camViewProj, &num, TYPE_FUNCTION::TYPE_FUNCTION_BLEND, SCENE_OBJECT_FLAGS::SCENE_OBJECT_BLEND);
	
	for (int i = 0; i < num; i++)
	{
		objs[i].DrawFunc(objs[i].obj, (void*)data);
	}
}
void RenderPostProcessingBloom(BloomFBO* bloomData, GLuint finalFBO, int finalSizeX, int finalSizeY)
{
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	float blurRadius = 4.0f;
	float intensity = 1.0f;
	if (bloomData->numBloomFbos == 0) return;
	glm::ivec2 fboSizes[20];
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