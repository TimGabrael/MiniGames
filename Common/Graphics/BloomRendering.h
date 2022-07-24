#pragma once
#include "GLCompat.h"

void InitializeBloomPipeline();
void CleanUpBloomPipeline();

void ResizeBloomInternals(int sx, int sy);

void BlurTextureToFramebuffer(GLuint endFbo, int fboX, int fboY, GLuint tex, float blurRadius);