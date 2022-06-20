#pragma once

#include "glad/glad.h"
#include "tiny_gltf.h"
#include "stb_truetype.h"
#include "stb_image_resize.h"


void InitializeOpenGL();


GLuint CreateProgram(const char* vertexShader, const char* fragmentShader);

bool LoadModel(tinygltf::Model& model, const char* filename);

