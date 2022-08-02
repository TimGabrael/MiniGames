#include "GLCompat.h"
#include "logging.h"
#define LOG_DRAW_CALLS false



static GLuint currentlyBoundShaderProgram = 0;
static GLuint screenFramebuffer = 0;
static GLuint mainFramebuffer = 0;
static glm::ivec2 screenFramebufferSize = { 0, 0 };
static glm::ivec2 mainFramebufferSize = { 0, 0 };

static bool depthTestEnabled = false;
static bool writeToDepthEnabled = true;
static bool blendTestEnabled = false;
static GLenum activeDepthFunc = -1;
static GLenum activeBlendFuncSrc = -1;
static GLenum activeBlendFuncDst = -1;

#if LOG_DRAW_CALLS
static int numDrawCallsMadeThisFrame = 0;
#endif

void EndFrameAndResetData()
{
#if LOG_DRAW_CALLS
	LOG("NUM_DRAW_CALLS: %d\n", numDrawCallsMadeThisFrame);
	numDrawCallsMadeThisFrame = 0;
#endif
	currentlyBoundShaderProgram = 0;
	glDepthMask(GL_TRUE);
	writeToDepthEnabled = true;
}

void glUseProgramWrapper(GLuint program)
{
	if (currentlyBoundShaderProgram != program)
	{
		glUseProgram(program);
		currentlyBoundShaderProgram = program;
	}
}
void glDrawElementsWrapper(GLenum mode, GLsizei count, GLenum type, const void* indices)
{
#if LOG_DRAW_CALLS
	numDrawCallsMadeThisFrame = numDrawCallsMadeThisFrame + 1;
#endif
	glDrawElements(mode, count, type, indices);
}
void glDrawArraysWrapper(GLenum mode, GLint first, GLsizei count)
{
#if LOG_DRAW_CALLS
	numDrawCallsMadeThisFrame = numDrawCallsMadeThisFrame + 1;
#endif
	glDrawArrays(mode, first, count);
}
void glDrawElementsInstancedWrapper(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount)
{
#if LOG_DRAW_CALLS
	numDrawCallsMadeThisFrame = numDrawCallsMadeThisFrame + 1;
#endif
	glDrawElementsInstanced(mode, count, type, indices, instancecount);
}
void glDrawArraysInstancedWrapper(GLenum mode, GLint first, GLsizei count, GLsizei instancecount)
{
#if LOG_DRAW_CALLS
	numDrawCallsMadeThisFrame = numDrawCallsMadeThisFrame + 1;
#endif
	glDrawArraysInstanced(mode, first, count, instancecount);
}


void SetScreenFramebuffer(GLuint fb, const glm::ivec2& size)
{
	screenFramebuffer = fb;
	screenFramebufferSize = size;
}
GLuint GetScreenFramebuffer()
{
	return screenFramebuffer;
}
glm::ivec2 GetScreenFramebufferSize()
{
	return screenFramebufferSize;
}

void SetMainFramebuffer(GLuint fb, const glm::ivec2& size)
{
	mainFramebuffer = fb;
	mainFramebufferSize = size;
}
GLuint GetMainFramebuffer()
{
	return mainFramebuffer;
}
glm::ivec2 GetMainFramebufferSize()
{
	return mainFramebufferSize;
}




void SetOpenGLWeakState(bool depthTest, bool blendTest)
{
	if (depthTestEnabled != depthTest)
	{
		if (depthTest) glEnable(GL_DEPTH_TEST);
		else glDisable(GL_DEPTH_TEST);
		depthTestEnabled = depthTest;
	}
	if (blendTestEnabled != blendTest)
	{
		if (blendTest) glEnable(GL_BLEND);
		else glDisable(GL_BLEND);
		blendTestEnabled = blendTest;
	}
	
}
void SetOpenGLDepthWrite(bool enable)
{
	if (writeToDepthEnabled != enable)
	{
		glDepthMask(enable);
		writeToDepthEnabled = enable;
	}
}
void glDepthFuncWrapper(GLenum func)
{
	if (func != activeDepthFunc)
	{
		glDepthFunc(func);
		activeDepthFunc = func;
	}
}
void glBlendFuncWrapper(GLenum sfactor, GLenum dfactor)
{
	if (sfactor != activeBlendFuncSrc || dfactor != activeBlendFuncDst)
	{
		glBlendFunc(sfactor, dfactor);
		activeBlendFuncSrc = sfactor;
		activeBlendFuncDst = dfactor;
	}
}
void SetDefaultBlendState()
{
	SetOpenGLWeakState(true, true);
	SetOpenGLDepthWrite(false);
	glDepthFuncWrapper(GL_LEQUAL);
	glBlendFuncWrapper(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
void SetDefaultOpaqueState()
{
	SetOpenGLWeakState(true, false);
	SetOpenGLDepthWrite(true);
	glDepthFuncWrapper(GL_LEQUAL);
}




// vec3 convert_xyz_to_cube_uv(vec3 xyz) output: int index, float u, float v


const char* GetFragmentShaderBase()
{
	return
		"#version 300 es\n\
precision highp float;\n\
precision highp sampler2DShadow;\n\
#define MAX_NUM_LIGHTS 8\n\
struct LightMapper\
{\
	mat4 viewProj;\
	vec2 start;\
	vec2 end;\
};\
struct CubemapLightMapperData\
{\
	vec4 startEnd[6];\
};\
struct PointLight\
{\
	vec3 pos;\
	float constant;\
	float linear;\
	float quadratic;\
	vec3 ambient;\
	vec3 diffuse;\
	vec3 specular;\
	CubemapLightMapperData mapper;\
	bool hasShadow;\
};\
struct DirectionalLight\
{\
	vec3 dir;\
	vec3 ambient;\
	vec3 diffuse;\
	vec3 specular;\
	LightMapper mapper;\
	bool hasShadow;\
};\
layout (std140) uniform LightData\
{\
	PointLight pointLights[MAX_NUM_LIGHTS];\
	DirectionalLight dirLights[MAX_NUM_LIGHTS];\
	int numPointLights;\
	int numDirLights;\
}_lightData;\
uniform sampler2DShadow shadowMap;\
uniform sampler2D _my_internalAOMap;\
uniform vec2 currentFBOSize;\
float CalculateShadowValue(LightMapper mapper, vec4 fragWorldPos)\
{\
	vec2 ts = vec2(1.0f) / vec2(textureSize(shadowMap, 0));\
	vec4 fragLightPos = mapper.viewProj * fragWorldPos;\
	vec3 projCoords = fragLightPos.xyz / fragLightPos.w;\
	projCoords = (projCoords * 0.5 + 0.5) * vec3(mapper.end, 1.0f) + vec3(mapper.start, 0.0f);\
	float shadow = 0.0f;\
	for(int x = -1; x <= 1; ++x)\
	{\
		for(int y = -1; y <= 1; ++y)\
		{\
			vec3 testPos = projCoords + vec3(ts.x * float(x), ts.y * float(y), 0.0f);\
			if(testPos.x < mapper.start.x || testPos.y < mapper.start.y || testPos.x > mapper.end.x || testPos.y > mapper.end.y) shadow += 1.0f;\
			else shadow += texture(shadowMap, testPos.xyz);\
		}\
	}\
	shadow /= 9.0f;\
	return shadow;\
}\
vec3 convert_xyz_to_cube_uv(vec3 xyz)\
{\
	vec3 indexUVout = vec3(0.0f);\
	float absX = abs(xyz.x);\
	float absY = abs(xyz.y);\
	float absZ = abs(xyz.z);\
	int isXPositive = xyz.x > 0.0f ? 1 : 0;\
	int isYPositive = xyz.y > 0.0f ? 1 : 0;\
	int isZPositive = xyz.z > 0.0f ? 1 : 0;\
	float maxAxis = 0.0f;\
	float uc = 0.0f;\
	float vc = 0.0f;\
	if (isXPositive != 0 && absX >= absY && absX >= absZ) {\
		maxAxis = absX;\
		uc = -xyz.z;\
		vc = xyz.y;\
		indexUVout.x = 0.0f;\
	}\
	if (isXPositive == 0 && absX >= absY && absX >= absZ) {\
		maxAxis = absX;\
		uc = xyz.z;\
		vc = xyz.y;\
		indexUVout.x = 1.0f;\
	}\
	if (isYPositive != 0 && absY >= absX && absY >= absZ) {\
		maxAxis = absY;\
		uc = xyz.x;\
		vc = -xyz.z;\
		indexUVout.x = 2.0f;\
	}\
	if (isYPositive == 0 && absY >= absX && absY >= absZ) {\
		maxAxis = absY;\
		uc = xyz.x;\
		vc = xyz.z;\
		indexUVout.x = 3.0f;\
	}\
	if (isZPositive != 0 && absZ >= absX && absZ >= absY) {\
		maxAxis = absZ;\
		uc = xyz.x;\
		vc = xyz.y;\
		indexUVout.x = 4.0f;\
	}\
	if (isZPositive == 0 && absZ >= absX && absZ >= absY) {\
		maxAxis = absZ;\
		uc = -xyz.x;\
		vc = xyz.y;\
		indexUVout.x = 5.0f;\
	}\
	indexUVout.y = 0.5f * (uc / maxAxis + 1.0f);\
	indexUVout.z = 0.5f * (vc / maxAxis + 1.0f);\
	return indexUVout;\
}\n\
float CalculateCubeShadowValue(CubemapLightMapperData mapper, vec3 fragToLight, float fragPosWorld)\
{\
	vec2 ts = vec2(1.0f) / vec2(textureSize(shadowMap, 0));\
	vec3 indexUV = convert_xyz_to_cube_uv(fragToLight);\n\
	return 1.0f;\
}\
vec3 CalculatePointLightColor(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 matDiffuseCol, vec3 matSpecCol, float shininess, float shadowValue)\
{\
	vec3 lightDir = normalize(light.pos - fragPos);\
	float diff = max(dot(normal, lightDir), 0.0f);\
	vec3 reflectDir = reflect(-lightDir, normal);\
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);\
	float distance = length(light.pos - fragPos);\
	float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));\
	vec3 ambient = light.ambient * matDiffuseCol;\
	vec3 diffuse = light.diffuse * diff * matDiffuseCol * shadowValue;\
	vec3 specular = light.specular * spec * matSpecCol * shadowValue;\
	ambient *= attenuation;\
	diffuse *= attenuation;\
	specular *= attenuation;\
	return (ambient + diffuse + specular);\
}\
vec3 CalculateDirectionalLightColor(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 matDiffuseCol, vec3 matSpecCol, float shininess, float shadowValue)\
{\
	vec3 lightDir = normalize(-light.dir);\
	float diff = max(dot(normal, lightDir), 0.0f);\
	vec3 reflectDir = reflect(-lightDir, normal);\
	float spec = pow(max(dot(viewDir, reflectDir), 0.0f), shininess);\
	vec3 ambient = light.ambient * matDiffuseCol;\
	vec3 diffuse = light.diffuse * diff * matDiffuseCol * shadowValue;\
	vec3 specular = light.specular * spec * matSpecCol * shadowValue;\
	return (ambient + diffuse + specular);\
}\
vec3 CalculateLightsColor(vec4 fragWorldPos, vec3 normal, vec3 viewDir, vec3 matDiffuseCol, vec3 matSpecCol, float shininess)\
{\
	vec3 result = vec3(0.0f);\
	float occludedValue = texture(_my_internalAOMap, (gl_FragCoord.xy - vec2(0.5)) / currentFBOSize).r;\
	matDiffuseCol *= occludedValue;\
	for(int i = 0; i < _lightData.numDirLights; i++)\
	{\
		float shadow = 1.0f;\
		if(_lightData.dirLights[i].hasShadow) shadow = CalculateShadowValue(_lightData.dirLights[i].mapper, fragWorldPos);\
		result += CalculateDirectionalLightColor(_lightData.dirLights[i], normal, viewDir, matDiffuseCol, matSpecCol, shininess, shadow);\
	}\
	for(int i = 0; i < _lightData.numPointLights; i++)\
	{\
		result += CalculatePointLightColor(_lightData.pointLights[i], normal, fragWorldPos.xyz, viewDir, matDiffuseCol, matSpecCol, shininess, 1.0f);\
	}\n\
	return result;\
}\
";

}