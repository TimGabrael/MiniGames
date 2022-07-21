#include "ReflectiveSurfaceRendering.h"
#include "Helper.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Renderer.h"

const char* vertexShader = "#version 300 es\n\
#extension GL_EXT_clip_cull_distance : enable\
\n\
const vec2 pos[6] = {\
	vec2(-0.5f, -0.5f),\
	vec2( 0.5f, -0.5f),\
	vec2( 0.5f,  0.5f),\
	vec2( 0.5f,  0.5f),\
	vec2(-0.5f,  0.5f),\
	vec2(-0.5f, -0.5f),\
};\
uniform mat4 projection;\n\
uniform mat4 view;\n\
uniform mat4 model;\n\
uniform vec4 clipPlane;\n\
out vec4 clipSpace;\
out vec4 texCoord;\
void main(){\
	vec4 worldPos = model * vec4(pos[gl_VertexID], 0.0f, 1.0f);\
	clipSpace = projection * view * worldPos;\
	gl_Position = clipSpace;\
	gl_ClipDistance[0] = dot(worldPos, clipPlane);\
}\
";
static const char* fragmentShader = "#version 300 es\n\
precision highp float;\n\
\n\
uniform Material {\n\
	vec4 tintColor;\n\
	float moveFactor;\n\
	float distortionFactor;\n\
	int type;\n\
} material;\n\
in vec4 clipSpace;\
in vec4 texCoord;\
uniform sampler2D reflectTexture;\
uniform sampler2D refractTexture;\
uniform sampler2D dvdu;\
out vec4 outCol;\
void main(){\
	vec2 refract = clipSpace.xy/clipSpace.w * 0.5f + 0.5f;\
	vec2 reflect = vec2(ndc.x, -ndc.y);\
	outCol = texture(reflectTexture, reflect);\
	outCol = outCol + texture(refractTexture, refract);\
}\
";

enum RS_TEXTURES
{
	RS_TEXTURE_REFLECT,
	RS_TEXTURE_REFRACT,
	RS_TEXTURE_DVDU,
	NUM_RS_TEXTURES,
};


struct ReflectiveSurfacePipelineObjects
{
	GLuint program;
	GLint projLoc;
	GLint viewLoc;
	GLint modelLoc;
	GLint clipPlaneLoc;
	GLint materialLoc;

}g_reflect;


struct ReflectiveSurfaceMaterial
{
	GLuint reflectionTexture;
	GLuint refractionTexture;
	GLuint dudv;
	GLuint dataUniform;
};

struct ReflectiveSurfaceSceneObject
{
	BaseSceneObject base;
	glm::mat4* modelTransform;
	ReflectiveSurfaceMaterial* material;
};
static_assert(sizeof(ReflectiveSurfaceSceneObject) <= sizeof(SceneObject), "INVALID OBJECT SIZE");


void InitializeReflectiveSurfacePipeline()
{
	g_reflect.program = CreateProgram(vertexShader, fragmentShader);

	glUseProgramWrapper(g_reflect.program);
	GLint index = glGetUniformLocation(g_reflect.program, "reflectTexture");
	glUniform1i(index, RS_TEXTURE_REFLECT);
	index = glGetUniformLocation(g_reflect.program, "refractTexture");
	glUniform1i(index, RS_TEXTURE_REFRACT);
	index = glGetUniformLocation(g_reflect.program, "dvdu");
	glUniform1i(index, RS_TEXTURE_DVDU);

	g_reflect.projLoc = glGetUniformLocation(g_reflect.program, "projection");
	g_reflect.viewLoc = glGetUniformLocation(g_reflect.program, "view");
	g_reflect.modelLoc = glGetUniformLocation(g_reflect.program, "model");
	g_reflect.clipPlaneLoc = glGetUniformLocation(g_reflect.program, "clipPlane");
	g_reflect.materialLoc = glGetUniformLocation(g_reflect.program, "material");
	glUniformBlockBinding(g_reflect.program, g_reflect.materialLoc, g_reflect.materialLoc);

	
}

SceneObject* AddReflectiveSurface(PScene scene, const glm::vec3* pos, const glm::vec3* normal, float scaleX, float scaleY, const ReflectiveSurfaceMaterialData* data, const ReflectiveSurfaceTextures* texData)
{
	ReflectiveSurfaceSceneObject* obj = (ReflectiveSurfaceSceneObject*)SC_AddSceneObject(scene, REFLECTIVE_RENDERABLE);
	
	obj->base.flags = SCENE_OBJECT_FLAGS::SCENE_OBJECT_OPAQUE | SCENE_OBJECT_FLAGS::SCENE_OBJECT_CAST_SHADOW | SCENE_OBJECT_FLAGS::SCENE_OBJECT_REFLECTED | SCENE_OBJECT_FLAGS::SCENE_OBJECT_SURFACE_REFLECTED;




	obj->material = (ReflectiveSurfaceMaterial*)SC_AddMaterial(scene, REFLECTIVE_RENDERABLE, sizeof(ReflectiveSurfaceMaterial));
	obj->modelTransform = (glm::mat4*)SC_AddTransform(scene, REFLECTIVE_RENDERABLE, sizeof(glm::mat4));

	const glm::vec3 norm = glm::normalize(*normal);
	const float product = glm::dot(norm, glm::vec3(0.0f, 0.0f, 1.0f));
	const glm::vec3 rotVec = glm::cross(norm, glm::vec3(0.0f, 0.0f, 1.0f));
	const float angle = acosf(product);

	*obj->modelTransform = glm::rotate(glm::mat4(1.0f), angle, rotVec) * glm::scale(glm::mat4(1.0f), glm::vec3(scaleX, scaleY, 1.0f));

	if (texData)
	{
		obj->material->dudv = texData->dudv;
		obj->material->reflectionTexture = texData->reflect;
		obj->material->refractionTexture = texData->refract;
	}

	glGenBuffers(1, &obj->material->dataUniform);
	glBindBuffer(GL_UNIFORM_BUFFER, obj->material->dataUniform);

	glBufferData(GL_UNIFORM_BUFFER, sizeof(ReflectiveSurfaceMaterialData), data, GL_STATIC_DRAW);

	return (SceneObject*)obj;
}

void ReflectiveSurfaceSetTextureData(SceneObject* objd, const ReflectiveSurfaceTextures* texData)
{
	ReflectiveSurfaceSceneObject* obj = (ReflectiveSurfaceSceneObject*)objd;
	obj->material->dudv = texData->dudv;
	obj->material->reflectionTexture = texData->reflect;
	obj->material->refractionTexture = texData->refract;
}

void DrawReflectiveSurface(ReflectiveSurfaceSceneObject* obj, const StandardRenderPassData* stdData, const glm::vec4* clipPlane)
{
	glUseProgramWrapper(g_reflect.program);
	glBindBufferBase(GL_UNIFORM_BUFFER, g_reflect.materialLoc, obj->material->dataUniform);
	glActiveTexture(GL_TEXTURE0 + RS_TEXTURE_REFLECT);
	glBindTexture(GL_TEXTURE_2D, obj->material->reflectionTexture);
	glActiveTexture(GL_TEXTURE0 + RS_TEXTURE_REFRACT);
	glBindTexture(GL_TEXTURE_2D, obj->material->reflectionTexture);
	glActiveTexture(GL_TEXTURE0 + RS_TEXTURE_DVDU);
	glBindTexture(GL_TEXTURE_2D, obj->material->dudv);

	glUniformMatrix4fv(g_reflect.projLoc, 1, GL_FALSE, (const GLfloat*)stdData->camProj);
	glUniformMatrix4fv(g_reflect.viewLoc, 1, GL_FALSE, (const GLfloat*)stdData->camView);
	glUniformMatrix4fv(g_reflect.modelLoc, 1, GL_FALSE, (const GLfloat*)obj->modelTransform);
	if(clipPlane)
		glUniform4fv(g_reflect.clipPlaneLoc, 1, (const GLfloat*)clipPlane);



	glDrawArrays(GL_TRIANGLES, 0, 6);

}

void DrawReflectStandard(SceneObject* obj, void* data)
{
	const StandardRenderPassData* d = (const StandardRenderPassData*)data;
	DrawReflectiveSurface((ReflectiveSurfaceSceneObject*)obj, d, nullptr);
}
void DrawReflectPlane(SceneObject* obj, void* data)
{
	const ReflectPlanePassData* planeData = (const ReflectPlanePassData*)data;
	DrawReflectiveSurface((ReflectiveSurfaceSceneObject*)obj, &planeData->base, planeData->planeEquation);
}
PFUNCDRAWSCENEOBJECT ReflectiveSurfaceGetDrawFunction(TYPE_FUNCTION f)
{
	if (f == TYPE_FUNCTION::TYPE_FUNCTION_OPAQUE) return DrawReflectStandard;
	else if (f == TYPE_FUNCTION::TYPE_FUNCTION_CLIP_PLANE_OPAQUE) return DrawReflectPlane;
	return nullptr;
}