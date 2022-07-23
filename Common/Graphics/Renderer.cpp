#include "Renderer.h"
#include "Helper.h"
#include "logging.h"


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