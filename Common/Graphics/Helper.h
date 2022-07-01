#pragma once

#include "GLCompat.h"
#include "stb_image.h"
#include "stb_truetype.h"
#include "stb_image_resize.h"
#include <vector>
#include <glm/glm.hpp>

void InitializeOpenGL(void* assetManager);
void CleanUpOpenGL();


GLuint CreateProgram(const char* vertexShader, const char* fragmentShader);


GLuint LoadCubemap(const char* right, const char* left, const char* bottom, const char* top, const char* front, const char* back);
GLuint LoadCubemap(const char* file);

GLuint GenerateBRDF_LUT(int dim);


void DrawSkybox(GLuint skybox, const glm::mat4& viewMat, const glm::mat4& projMat);


