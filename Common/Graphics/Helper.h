#pragma once

#include "GLCompat.h"
#include "stb_image.h"
#include "stb_truetype.h"
#include "stb_image_resize.h"
#include <vector>
#include <glm/glm.hpp>
#include "Scene.h"

void InitializeOpenGL(void* assetManager);
void CleanUpOpenGL();


GLuint CreateProgram(const char* vertexShader, const char* fragmentShader);
GLuint CreateProgram(const char* vertexShader);	
GLuint CreateProgramExtended(const char* vertexShader, const char* fragmentShaderExtension, GLuint* lightUniform, uint32_t shadowMapIdx);


GLuint LoadCubemap(const char* right, const char* left, const char* bottom, const char* top, const char* front, const char* back);
GLuint LoadCubemap(const char* file);

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
	GLuint fbo;
	GLuint depth;
	GLuint texture;
};
struct DepthFBO
{
	GLuint fbo;
	GLuint depth;
};
struct BloomFBO
{
	GLuint defaultFBO;
	GLuint* bloomFBOs1;
	GLuint* bloomFBOs2;
	GLuint bloomTexture1;
	GLuint bloomTexture2;
	GLuint defaultTexture;
	GLuint defaultDepth;
	int sizeX;
	int sizeY;
	int numBloomFbos; // numBloomFbos == numMipMaps
	void Create(int drawFboSx, int drawFboSy, int sx, int sy);
	void Resize(int drawFboSx, int drawFboSy, int sx, int sy);
	void CleanUp();
};

PScene CreateAndInitializeSceneAsDefault();
SingleFBO CreateSingleFBO(int width, int height);
DepthFBO CreateDepthFBO(int width, int height);
void RecreateSingleFBO(SingleFBO* fbo, int width, int height);
void DestroySingleFBO(SingleFBO* fbo);
void DestroyDepthFBO(DepthFBO* fbo);

