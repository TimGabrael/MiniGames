#pragma once


#if defined(ANDROID) or defined(EMSCRIPTEN)
#include <GLES3/gl3.h>
#include <GLES3/gl3platform.h>
#else
#include "glad/glad.h"
#endif
#include <glm/glm.hpp>
#define MAX_BLOOM_MIPMAPS 8
#define MAX_NUM_LIGHTS 8

void EndFrameAndResetData();

void glUseProgramWrapper(GLuint program);
void glDrawElementsWrapper(GLenum mode, GLsizei count, GLenum type, const void* indices);
void glDrawArraysWrapper(GLenum mode, GLint first, GLsizei count);
void glDrawElementsInstancedWrapper(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instancecount);
void glDrawArraysInstancedWrapper(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);

// THESE ARE NEEDED, AS QT/ANDROID_NATIVE_GLUE MAY FORCE A DIFFRENT SCREEN FRAMEBUFFER
// IN THE CASE OF QT IT SHOWED THAT THE SCREEN FB IS 1 USUALLY
// BUT THAT MAY ( MAY ) DIFFER ON EACH STARTUP WHO KNOWS WHAT QT IS UP TO :(
void SetScreenFramebuffer(GLuint fb);
GLuint GetScreenFramebuffer();

// THE FRAMEBUFFER ALL OBJECTS ARE SUPPOSED TO BE DRAWN TO, may differ from DefaultFramebuffer for example in Compositing/Bloom
void SetMainFramebuffer(GLuint fb);
GLuint GetMainFramebuffer();








struct PointLightData
{
	glm::vec3 pos;
	float constant;
	float linear;
	float quadratic;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
};
struct DirectionalLightData
{
	glm::vec3 dir;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	bool hasShadow;
};
struct LightMapperData	// maps the light to a region in a shadow map
{
	glm::mat4 viewProj;
	glm::vec2 start;
	glm::vec2 end;
};
struct LightUniformData
{
	LightMapperData mapped;
	PointLightData pointLights[MAX_NUM_LIGHTS];
	DirectionalLightData dirLights[MAX_NUM_LIGHTS];
	int numPointLights;
	int numDirLights;
};

struct ScenePointLight
{
	PointLightData data;
	uint16_t group;
};
struct SceneDirLight
{
	DirectionalLightData data;
	glm::mat4 viewProj;
	glm::vec2 shadowAreaStart;
	glm::vec2 shadowAreaEnd;
	uint16_t group;
};


const char* GetVertexShaderBase();
// Predefined Functions: 
// void SetShadowOutputVariable(vec4 worldPos)
//

const char* GetFragmentShaderBase();
// Predefined Functions: 
// float CalculateShadowValue(LightMapper mapper, vec4 fragWorldPos)
// vec3 CalculatePointLightColor(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 matDiffuseCol, vec3 matSpecCol, float shininess)
// vec3 CalculateDirectionalLightColor(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 matDiffuseCol, vec3 matSpecCol, float shininess)
// 
// vec3 CalculateLightsColor(vec4 fragWorldPos, vec3 normal, vec3 viewDir, vec3 matDiffuseCol, vec3 matSpecCol, float shininess)
//
