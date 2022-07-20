#include "Renderer.h"
#include "Helper.h"
#include "logging.h"



void RenderSceneReflectedOnPlane(PScene scene, const Camera* cam, const glm::vec4* planeEquation, GLuint cameraUniform, GLuint skybox)
{
	glEnable(GL_CLIP_DISTANCE0);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDisable(GL_BLEND);

	Camera rCam = Camera::GetReflected(cam, *planeEquation);

	int num;
	glm::mat4 camViewProj = rCam.perspective * rCam.view;
	ObjectRenderStruct* objs = SC_GenerateRenderList(scene);
	SC_FillRenderList(scene, objs, &camViewProj, &num, TYPE_FUNCTION::TYPE_FUNCTION_CLIP_PLANE_OPAQUE, SCENE_OBJECT_FLAGS::SCENE_OBJECT_SURFACE_REFLECTED | SCENE_OBJECT_FLAGS::SCENE_OBJECT_OPAQUE);
	
	ReflectPlanePassData reflectPassData;
	StandardRenderPassData& standardPassData = reflectPassData.base;
	standardPassData.camView = &rCam.view;
	standardPassData.camProj = &rCam.perspective;
	standardPassData.cameraUniform = cameraUniform;
	standardPassData.skyBox = skybox;
	reflectPassData.planeEquation = planeEquation;

	for (int i = 0; i < num; i++)
	{
		objs[i].DrawFunc(objs[i].obj, &reflectPassData);
	}
	DrawSkybox(skybox, camViewProj);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	SC_FillRenderList(scene, objs, &camViewProj, &num, TYPE_FUNCTION::TYPE_FUNCTION_CLIP_PLANE_BLEND, SCENE_OBJECT_FLAGS::SCENE_OBJECT_SURFACE_REFLECTED | SCENE_OBJECT_FLAGS::SCENE_OBJECT_BLEND);
	for (int i = 0; i < num; i++)
	{
		objs[i].DrawFunc(objs[i].obj, &reflectPassData);
	}


	SC_FreeRenderList(objs);

	glDisable(GL_CLIP_DISTANCE0);
}

void RenderSceneStandard(PScene scene, const glm::mat4* camView, const glm::mat4* camProj, GLuint cameraUniform, GLuint skybox)
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDisable(GL_BLEND);

	StandardRenderPassData standardPassData;
	standardPassData.camView = camView;
	standardPassData.camProj = camProj;
	standardPassData.cameraUniform = cameraUniform;
	standardPassData.skyBox = skybox;

	int num;
	glm::mat4 camViewProj = *camProj * *camView;
	ObjectRenderStruct* objs = SC_GenerateRenderList(scene);
	SC_FillRenderList(scene, objs, &camViewProj, &num, TYPE_FUNCTION::TYPE_FUNCTION_OPAQUE, SCENE_OBJECT_FLAGS::SCENE_OBJECT_OPAQUE);
	for (int i = 0; i < num; i++)
	{
		objs[i].DrawFunc(objs[i].obj, &standardPassData);
	}
	DrawSkybox(skybox, camViewProj);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	SC_FillRenderList(scene, objs, &camViewProj, &num, TYPE_FUNCTION::TYPE_FUNCTION_BLEND, SCENE_OBJECT_FLAGS::SCENE_OBJECT_BLEND);
	
	for (int i = 0; i < num; i++)
	{
		objs[i].DrawFunc(objs[i].obj, &standardPassData);
	}
	SC_FreeRenderList(objs);
}