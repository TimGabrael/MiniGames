#include "Renderer.h"
#include "Helper.h"
#include "logging.h"

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
	ObjectRenderStruct* objs = SC_GenerateRenderListOpaque(scene, &camViewProj, &num);
	for (int i = 0; i < num; i++)
	{
		objs[i].DrawFunc(objs[i].obj, &standardPassData);
	}
	SC_FreeRenderListOpaque(objs);
	DrawSkybox(skybox, camViewProj);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	objs = SC_GenerateRenderListBlend(scene, &camViewProj, &num);
	for (int i = 0; i < num; i++)
	{
		objs[i].DrawFunc(objs[i].obj, &standardPassData);
	}

}