#include "GLCompat.h"
#include "logging.h"
#define LOG_DRAW_CALLS false



static GLuint currentlyBoundShaderProgram = 0;
static GLuint screenFramebuffer = 0;
static GLuint mainFramebuffer = 0;
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


void SetScreenFramebuffer(GLuint fb)
{
	screenFramebuffer = fb;
}
GLuint GetScreenFramebuffer()
{
	return screenFramebuffer;
}

void SetMainFramebuffer(GLuint fb)
{
	mainFramebuffer = fb;
}
GLuint GetMainFramebuffer()
{
	return mainFramebuffer;
}





const char* GetVertexShaderBase()
{
	return
"#version 300 es\n\
#define MAX_NUM_LIGHTS 8\n\
struct PointLight\
{\
	vec3 pos;\
	float constant;\
	float linear;\
	float quadratic;\
	vec3 ambient;\
	vec3 diffuse;\
	vec3 specular;\
};\
struct DirectionalLight\
{\
	vec3 dir;\
	vec3 ambient;\
	vec3 diffuse;\
	vec3 specular;\
	bool hasShadow;\
};\
struct LightMapper\
{\
	mat4 viewProj;\
	vec2 start;\
	vec2 end;\
};\
uniform LightData\
{\
	LightMapper mapped;\
	PointLight pointLights[MAX_NUM_LIGHTS];\
	DirectionalLight dirLights[MAX_NUM_LIGHTS];\
	int numPointLights;\
	int numDirLights;\
}_lightData;\
out vec4 shadowFragLightPos;\
void SetShadowOutputVariable(vec4 worldPos)\
{\
	shadowFragLightPos = _lightData.mapped.viewProj * vec4(worldPos.xyz, 1.0f);\
}\n\
";
}









//uniform sampler2DShadow shadowMap;\
//
const char* GetFragmentShaderBase()
{
	return
		"#version 300 es\n\
precision highp float;\n\
precision highp sampler2DShadow;\n\
#define MAX_NUM_LIGHTS 8\n\
struct PointLight\
{\
	vec3 pos;\
	float constant;\
	float linear;\
	float quadratic;\
	vec3 ambient;\
	vec3 diffuse;\
	vec3 specular;\
};\
struct DirectionalLight\
{\
	vec3 dir;\
	vec3 ambient;\
	vec3 diffuse;\
	vec3 specular;\
	bool hasShadow;\
};\
struct LightMapper\
{\
	mat4 viewProj;\
	vec2 start;\
	vec2 end;\
};\
uniform LightData\
{\
	LightMapper mapped;\
	PointLight pointLights[MAX_NUM_LIGHTS];\
	DirectionalLight dirLights[MAX_NUM_LIGHTS];\
	int numPointLights;\
	int numDirLights;\
}_lightData;\
in vec4 shadowFragLightPos;\
uniform sampler2DShadow shadowMap;\
float CalculateShadowValue(LightMapper mapper, vec4 fragLightPos)\
{\
	vec2 ts = vec2(1.0f) / vec2(textureSize(shadowMap, 0));\
	vec3 projCoords = fragLightPos.xyz / fragLightPos.w;\
	projCoords = (projCoords * 0.5 + 0.5) * vec3(mapper.end, 1.0f) + vec3(mapper.start, 0.0f);\
	float shadow = 0.0f;\
	for(int x = -1; x <= 1; ++x)\
	{\
		for(int y = -1; y <= 1; ++y)\
		{\
			vec3 testPos = projCoords + vec3(ts.x * float(x), ts.y * float(y), 0.0f);\
			if(testPos.x < mapper.start.x || testPos.y < mapper.start.y || testPos.x > mapper.end.x || testPos.y > mapper.end.y) shadow += 1.0f;\
			shadow += texture(shadowMap, testPos.xyz);\
		}\
	}\
	shadow /= 9.0f;\
	return shadow;\
}\
vec3 CalculatePointLightColor(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 matDiffuseCol, vec3 matSpecCol, float shininess)\
{\
	vec3 lightDir = normalize(light.pos - fragPos);\
	float diff = max(dot(normal, lightDir), 0.0f);\
	vec3 reflectDir = reflect(-lightDir, normal);\
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);\
	float distance = length(light.pos - fragPos);\
	float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));\
	vec3 ambient = light.ambient * matDiffuseCol;\
	vec3 diffuse = light.diffuse * diff * matDiffuseCol;\
	vec3 specular = light.specular * spec * matSpecCol;\
	ambient *= attenuation;\
	diffuse *= attenuation;\
	specular *= attenuation;\
	return (ambient + diffuse + specular);\
}\
vec3 CalculateDirectionalLightColor(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 matDiffuseCol, vec3 matSpecCol, float shininess)\
{\
	vec3 lightDir = normalize(-light.dir);\
	float diff = max(dot(normal, lightDir), 0.0);\
	vec3 reflectDir = reflect(-lightDir, normal);\
	float spec = pow(max(dot(viewDir, reflectDir), 0.0f), shininess);\
	vec3 ambient = light.ambient * matDiffuseCol;\
	vec3 diffuse = light.diffuse * diff * matDiffuseCol;\
	vec3 specular = light.specular * spec * matSpecCol;\
	return (ambient + diffuse + specular);\
}\
vec3 CalculateLightsColor(vec4 fragWorldPos, vec3 normal, vec3 viewDir, vec3 matDiffuseCol, vec3 matSpecCol, float shininess)\
{\
	vec3 result = vec3(0.0f);\
	for(int i = 0; i < _lightData.numDirLights; i++)\
	{\
		float shadow = 1.0f;\
		if(_lightData.dirLights[i].hasShadow) shadow = CalculateShadowValue(_lightData.mapped, shadowFragLightPos);\
		result += CalculateDirectionalLightColor(_lightData.dirLights[i], normal, viewDir, matDiffuseCol, matSpecCol, shininess) * shadow;\
	}\
	for(int i = 0; i < _lightData.numPointLights; i++)\
	{\
		result += CalculatePointLightColor(_lightData.pointLights[i], normal, fragWorldPos.xyz, viewDir, matDiffuseCol, matSpecCol, shininess);\
	}\
	return result;\
}\
";

}