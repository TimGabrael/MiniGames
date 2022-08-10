#pragma once
#include "GLCompat.h"

void InitializeBloomPipeline();
void CleanUpBloomPipeline();


void BlurTextureToFramebuffer(GLuint endFbo, int fboX, int fboY, GLuint tex, float blurRadius, GLuint intermediateFBO, int intermediateSizeX, int intermediateSizeY, GLuint intermediateTexture);
void BloomTextureToFramebuffer(GLuint endFbo, int fboX, int fboY, GLuint intermediateFbo, int interX, int interY, GLuint intermediateTexture, GLuint tex, float blurRadius, float bloomIntensity, int texMipLevel, int endMipLevel);
void CopyTextureToFramebuffer(GLuint tex, int mipMapLevel);
void CopyTexturesToFramebuffer(GLuint tex1, int mipMapLevel1, GLuint tex2, int mipMapLevel2);	// uses ACES tonemapping to map (0.0f -> inf) -> (0.0f, 1.0f)
void UpsampleTextureToFramebuffer(GLuint tex, int mipMapLevel);