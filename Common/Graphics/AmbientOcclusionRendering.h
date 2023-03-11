#pragma once
#include "GLCompat.h"


void InitializeAmbientOcclusionPipeline();
void CleanUpAmbientOcclusionPipeline();


void RenderAmbientOcclusionQuad(GLuint depthTexture, float depthTexSizeX, float depthTexSizeY);

