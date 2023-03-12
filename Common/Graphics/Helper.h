#pragma once

#include "GLCompat.h"
#include "stb_image.h"
#include "stb_truetype.h"
#include "stb_image_resize.h"
#include <vector>
#include <glm/glm.hpp>
#include "Scene.h"

struct EnvironmentMaps {
    int width;
    int height;
    int irradiance_width;
    int irradiance_height;
    GLuint prefiltered;
    GLuint irradiance;
};

void InitializeOpenGL(void* assetManager);
void CleanUpOpenGL();


GLuint CreateProgram(const char* vertexShader, const char* fragmentShader);
GLuint CreateProgram(const char* vertexShader);	
GLuint CreateProgramExtended(const char* vertexShader, const char* fragmentShaderExtension, GLuint* lightUniform, GLuint* fboSizeUniformLoc, uint32_t shadowMapIdx, uint32_t globalAmbientOcclusionMapIdx);


GLuint LoadCubemap(const char* right, const char* left, const char* bottom, const char* top, const char* front, const char* back);
GLuint LoadCubemap(const char* file);

EnvironmentMaps LoadEnvironmentMaps(const char* file);
void UnloadEnvironmentMaps(EnvironmentMaps* env);

GLuint GenerateBRDF_LUT(int dim);


void DrawSkybox(GLuint skybox, const glm::mat4& viewMat, const glm::mat4& projMat);
void DrawSkybox(GLuint skybox, const glm::mat4& viewProjMat);


enum DEFAULT_SCENE_RENDER_TYPES
{
	SIMPLE_3D_RENDERABLE,
	PBR_RENDERABLE,
	REFLECTIVE_RENDERABLE,
	NUM_DEFAULT_RENDERABLES,
};

struct StreamArrayBuffer
{
	GLuint glBuf;
	void* mapped;
	uint32_t writeIdx;
	uint32_t size;
	void Init();
	void CleanUp();
	void Bind();
	void Append(const void* data, uint32_t dataSize, uint32_t sizeSteps);
};
struct SingleFBO
{
	GLuint fbo = 0;
	GLuint depth = 0;
	GLuint texture = 0;
};
struct DepthFBO
{
	GLuint fbo = 0;
	GLuint depth = 0;
};
struct BloomFBO
{
	GLuint* bloomFBOs1 = nullptr;
	GLuint* bloomFBOs2 = nullptr;
	GLuint bloomTexture1 = 0;
	GLuint bloomTexture2 = 0;
	int sizeX = 0;
	int sizeY = 0;
	int numBloomFbos = 0; // numBloomFbos == numMipMaps
	void Create(int sx, int sy);
	void Resize(int sx, int sy);
	void CleanUp();
};

PScene CreateAndInitializeSceneAsDefault();
SingleFBO CreateSingleFBO(int width, int height);
SingleFBO CreateSingleFBO(int width, int height, GLenum internalFormatColor, GLenum formatColor, GLenum internalFormatDepth, int numSamples);
DepthFBO CreateDepthFBO(int width, int height);
void RecreateSingleFBO(SingleFBO* fbo, int width, int height);
void DestroySingleFBO(SingleFBO* fbo);
void DestroyDepthFBO(DepthFBO* fbo);


