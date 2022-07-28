#include "ReflectiveSurfaceRendering.h"
#include "Helper.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Renderer.h"
#include "logging.h"

const char* vertexShader = "#version 300 es\n\
vec2 pos[6] = vec2[6](\
	vec2(-0.5f, -0.5f),\
	vec2( 0.5f, -0.5f),\
	vec2( 0.5f,  0.5f),\
	vec2( 0.5f,  0.5f),\
	vec2(-0.5f,  0.5f),\
	vec2(-0.5f, -0.5f)\
);\
uniform mat4 projection;\n\
uniform mat4 view;\n\
uniform mat4 model;\n\
uniform mat4 testLightUniform;\n\
uniform vec4 clipPlane;\n\
out vec4 clipSpace;\
out vec2 texCoord;\
out vec4 fragPosWorld;\
out float clipDist;\
void main(){\
	vec4 worldPos = model * vec4(pos[gl_VertexID], 0.0f, 1.0f);\
	fragPosWorld = worldPos;\
	clipSpace = projection * view * worldPos;\
	gl_Position = clipSpace;\
	clipDist = dot(worldPos, clipPlane);\
	texCoord = pos[gl_VertexID] + vec2(0.5f);\
}\
";
const char* vertexShaderExtension = "\n\
vec2 pos[6] = vec2[6](\
	vec2(-0.5f, -0.5f),\
	vec2( 0.5f, -0.5f),\
	vec2( 0.5f,  0.5f),\
	vec2( 0.5f,  0.5f),\
	vec2(-0.5f,  0.5f),\
	vec2(-0.5f, -0.5f)\
);\
uniform mat4 projection;\n\
uniform mat4 view;\n\
uniform mat4 model;\n\
uniform mat4 testLightUniform;\n\
uniform vec4 clipPlane;\n\
out vec4 clipSpace;\
out vec2 texCoord;\
out vec4 fragPosWorld;\
out float clipDist;\
out vec4 fragLightCoord;\
void main(){\
	vec4 worldPos = model * vec4(pos[gl_VertexID], 0.0f, 1.0f);\
	fragPosWorld = worldPos;\
	fragLightCoord = testLightUniform * worldPos;\
	clipSpace = projection * view * worldPos;\
	gl_Position = clipSpace;\
	clipDist = dot(worldPos, clipPlane);\
	texCoord = pos[gl_VertexID] + vec2(0.5f);\
}\
";
static const char* fragmentShaderExtension = "\n\
uniform Material {\n\
	vec4 tintColor;\n\
	float moveFactor;\n\
	float distortionFactor;\n\
	int type;\n\
} material;\n\
in vec4 clipSpace;\
in vec2 texCoord;\
in vec4 fragPosWorld;\
in float clipDist;\
in vec4 fragLightCoord;\
uniform sampler2D reflectTexture;\
uniform sampler2D refractTexture;\
uniform sampler2D dvdu;\
out vec4 outCol;\
float shadowCalculation()\
{\
	vec2 ts = vec2(1.0f) / vec2(textureSize(shadowMap, 0));\
	vec4 wPos = _lightData.mapped.viewProj * fragPosWorld;\
	vec3 projCoords = fragLightCoord.xyz / fragLightCoord.w;\
	projCoords = projCoords * 0.5f + 0.5f;\
	projCoords.z -= 0.002f;\
	float shadow = 0.0f;\
	for(int x = -1; x <= 1; ++x)\
	{\
		for(int y = -1; y <= 1; ++y)\
		{\
			vec3 testPos = projCoords + vec3(ts.x * float(x), ts.y * float(y), 0.0f);\
			if(testPos.x < 0.0f || testPos.y < 0.0f || testPos.x > 1.0f || testPos.y > 1.0f) shadow += 1.0f;\
			else shadow += texture(shadowMap, testPos.xyz);\
		}\
	}\
	shadow /= 9.0f;\
	return shadow;\
}\
void main(){\
	if(clipDist < 0.0f) discard;\
	vec2 refract = clipSpace.xy/clipSpace.w * 0.5f + 0.5f;\
	vec2 reflect = vec2(refract.x, -refract.y);\
	outCol = texture(reflectTexture, reflect);\n\
	float shadow = shadowCalculation();\n\
	outCol = (shadow + 0.2f) * (vec4(0.8f) * outCol + vec4(0.2f) * texture(refractTexture, texCoord)) * material.tintColor;\n\
}\
";

// THE texture(shadowMap, projCoords) already applys a little bit of antialiasing (2x2 Kernel)
// might be sufficient for my uses


enum RS_TEXTURES
{
	RS_TEXTURE_REFLECT,
	RS_TEXTURE_REFRACT,
	RS_TEXTURE_DVDU,
	RS_TEXTURE_SHADOWMAP,
	NUM_RS_TEXTURES,
};

struct ReflectiveSurfaceUniforms
{
	GLint projLoc;
	GLint viewLoc;
	GLint modelLoc;
	GLint clipPlaneLoc;
	GLint materialLoc;
	GLuint lightDataLoc;

	GLuint testUniLoc;
};


struct ReflectiveSurfacePipelineObjects
{
	GLuint geometryProgram;
	GLuint program;
	ReflectiveSurfaceUniforms unis;
	ReflectiveSurfaceUniforms geomUnis;

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
	g_reflect.geometryProgram = CreateProgram(vertexShader);

	g_reflect.program = CreateProgramExtended(vertexShaderExtension, fragmentShaderExtension, &g_reflect.unis.lightDataLoc, RS_TEXTURE_SHADOWMAP);

	glUseProgramWrapper(g_reflect.program);
	GLint index = glGetUniformLocation(g_reflect.program, "reflectTexture");
	glUniform1i(index, RS_TEXTURE_REFLECT);
	index = glGetUniformLocation(g_reflect.program, "refractTexture");
	glUniform1i(index, RS_TEXTURE_REFRACT);
	index = glGetUniformLocation(g_reflect.program, "dvdu");
	glUniform1i(index, RS_TEXTURE_DVDU);

	g_reflect.unis.projLoc = glGetUniformLocation(g_reflect.program, "projection");
	g_reflect.unis.viewLoc = glGetUniformLocation(g_reflect.program, "view");
	g_reflect.unis.modelLoc = glGetUniformLocation(g_reflect.program, "model");
	
	g_reflect.unis.testUniLoc = glGetUniformLocation(g_reflect.program, "testLightUniform");

	g_reflect.unis.clipPlaneLoc = glGetUniformLocation(g_reflect.program, "clipPlane");
	g_reflect.unis.materialLoc = glGetUniformBlockIndex(g_reflect.program, "Material");
	glUniformBlockBinding(g_reflect.program, g_reflect.unis.materialLoc, g_reflect.unis.materialLoc);

	g_reflect.geomUnis.projLoc = glGetUniformLocation(g_reflect.geometryProgram, "projection");
	g_reflect.geomUnis.viewLoc = glGetUniformLocation(g_reflect.geometryProgram, "view");
	g_reflect.geomUnis.modelLoc = glGetUniformLocation(g_reflect.geometryProgram, "model");

	g_reflect.geomUnis.testUniLoc = glGetUniformLocation(g_reflect.geometryProgram, "testLightUniform");

	g_reflect.geomUnis.clipPlaneLoc = glGetUniformLocation(g_reflect.geometryProgram, "clipPlane");
	g_reflect.geomUnis.materialLoc = glGetUniformBlockIndex(g_reflect.geometryProgram, "Material");
	glUniformBlockBinding(g_reflect.geometryProgram, g_reflect.geomUnis.materialLoc, g_reflect.geomUnis.materialLoc);

}
SceneObject* AddReflectiveSurface(PScene scene, const glm::vec3* pos, const glm::vec3* normal, float scaleX, float scaleY, const ReflectiveSurfaceMaterialData* data, const ReflectiveSurfaceTextures* texData)
{
	ReflectiveSurfaceSceneObject* obj = (ReflectiveSurfaceSceneObject*)SC_AddSceneObject(scene, REFLECTIVE_RENDERABLE);
	
	obj->base.flags = SCENE_OBJECT_FLAGS::SCENE_OBJECT_OPAQUE | SCENE_OBJECT_FLAGS::SCENE_OBJECT_CAST_SHADOW | SCENE_OBJECT_FLAGS::SCENE_OBJECT_REFLECTED | SCENE_OBJECT_FLAGS::SCENE_OBJECT_SURFACE_REFLECTED;

	obj->base.bbox.leftTopFront = { -0.5f, -0.5f, -0.1f };
	obj->base.bbox.rightBottomBack = { 0.5f, 0.5f, 0.1f };


	obj->material = (ReflectiveSurfaceMaterial*)SC_AddMaterial(scene, REFLECTIVE_RENDERABLE, sizeof(ReflectiveSurfaceMaterial));
	obj->modelTransform = (glm::mat4*)SC_AddTransform(scene, REFLECTIVE_RENDERABLE, sizeof(glm::mat4));

	const glm::vec3 norm = glm::normalize(*normal);
	const float product = glm::dot(norm, glm::vec3(0.0f, 0.0f, 1.0f));
	const glm::vec3 rotVec = glm::cross(norm, glm::vec3(0.0f, 0.0f, 1.0f));
	const float angle = acosf(product);
	

	if(angle != 0.0f)
	{
		*obj->modelTransform = glm::translate(glm::mat4(1.0f), *pos) * glm::rotate(glm::mat4(1.0f), -angle, rotVec) * glm::scale(glm::mat4(1.0f), glm::vec3(scaleX, scaleY, 1.0f));
	}
	else
	{
		*obj->modelTransform = glm::translate(glm::mat4(1.0f), *pos) * glm::scale(glm::mat4(1.0f), glm::vec3(scaleX, scaleY, 1.0f));
	}
	obj->base.bbox.leftTopFront = *obj->modelTransform * glm::vec4(obj->base.bbox.leftTopFront, 1.0f);
	obj->base.bbox.rightBottomBack = *obj->modelTransform * glm::vec4(obj->base.bbox.rightBottomBack, 1.0f);
	auto& min = obj->base.bbox.leftTopFront;
	auto& max = obj->base.bbox.rightBottomBack;
	if (min.x > max.x) { float temp = max.x; max.x = min.x; min.x = temp; }
	if (min.y > max.y) { float temp = max.y; max.y = min.y; min.y = temp; }
	if (min.z > max.z) { float temp = max.z; max.z = min.z; min.z = temp; }

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

static void DrawReflectiveSurface(ReflectiveSurfaceSceneObject* obj, const StandardRenderPassData* stdData, const glm::vec4* clipPlane, bool geometryOnly)
{
	ReflectiveSurfaceUniforms* unis = geometryOnly ? &g_reflect.geomUnis : &g_reflect.unis;
	glUseProgramWrapper(geometryOnly ? g_reflect.geometryProgram : g_reflect.program);
	if(!geometryOnly)
		glBindBufferBase(GL_UNIFORM_BUFFER, unis->materialLoc, obj->material->dataUniform);
	
	glActiveTexture(GL_TEXTURE0 + RS_TEXTURE_REFLECT);
	glBindTexture(GL_TEXTURE_2D, obj->material->reflectionTexture);
	glActiveTexture(GL_TEXTURE0 + RS_TEXTURE_REFRACT);
	glBindTexture(GL_TEXTURE_2D, obj->material->refractionTexture);
	glActiveTexture(GL_TEXTURE0 + RS_TEXTURE_DVDU);
	glBindTexture(GL_TEXTURE_2D, obj->material->dudv);
	glActiveTexture(GL_TEXTURE0 + RS_TEXTURE_SHADOWMAP);
	glBindTexture(GL_TEXTURE_2D, stdData->shadowMap);
	

	glUniformMatrix4fv(unis->projLoc, 1, GL_FALSE, (const GLfloat*)stdData->camProj);
	glUniformMatrix4fv(unis->viewLoc, 1, GL_FALSE, (const GLfloat*)stdData->camView);
	glUniformMatrix4fv(unis->modelLoc, 1, GL_FALSE, (const GLfloat*)obj->modelTransform);
	glBindBufferBase(GL_UNIFORM_BUFFER, unis->lightDataLoc, stdData->lightData);
	

	glm::vec3 pos = { 2.0f, 4.0f, 2.0f };
	glm::vec3 lightDir = glm::vec3(-1.0f / sqrt(3));
	glm::mat4 view = glm::lookAtLH(pos, pos - lightDir, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 proj = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, -10.0f, 10.0f);
	glm::mat4 viewProj = proj * view;
	glUniformMatrix4fv(unis->testUniLoc, 1, GL_FALSE, (const GLfloat*)&viewProj);
	if (clipPlane)
		glUniform4fv(unis->clipPlaneLoc, 1, (const GLfloat*)clipPlane);
	else
		glUniform4f(unis->clipPlaneLoc, 0.0f, 1.0f, 0.0f, 10000000.0f);


	glDrawArrays(GL_TRIANGLES, 0, 6);

}

static void DrawReflectGeoemtry(SceneObject* obj, void* data)
{
	const StandardRenderPassData* d = (const StandardRenderPassData*)data;
	DrawReflectiveSurface((ReflectiveSurfaceSceneObject*)obj, d, nullptr, true);
}
static void DrawReflectStandard(SceneObject* obj, void* data)
{
	const StandardRenderPassData* d = (const StandardRenderPassData*)data;
	DrawReflectiveSurface((ReflectiveSurfaceSceneObject*)obj, d, nullptr, false);
}
static void DrawReflectPlane(SceneObject* obj, void* data)
{
	const ReflectPlanePassData* planeData = (const ReflectPlanePassData*)data;
	DrawReflectiveSurface((ReflectiveSurfaceSceneObject*)obj, planeData->base, planeData->planeEquation, false);
}
PFUNCDRAWSCENEOBJECT ReflectiveSurfaceGetDrawFunction(TYPE_FUNCTION f)
{
	if (f == TYPE_FUNCTION::TYPE_FUNCTION_OPAQUE) return DrawReflectStandard;
	else if (f == TYPE_FUNCTION::TYPE_FUNCTION_CLIP_PLANE_OPAQUE) return DrawReflectPlane;
	else if (f == TYPE_FUNCTION::TYPE_FUNCTION_GEOMETRY) return DrawReflectGeoemtry;
	return nullptr;
}