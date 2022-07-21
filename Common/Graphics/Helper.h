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

PScene CreateAndInitializeSceneAsDefault();
SingleFBO CreateSingleFBO(int width, int height);
void RecreateSingleFBO(SingleFBO* fbo, int width, int height);
void DestroySingleFBO(SingleFBO* fbo);