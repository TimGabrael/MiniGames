#ifndef BASE_RENDER_H
#define BASE_RENDER_H
#include "GLCompat.h"
#include "Scene.h"


struct UBO
{
	glm::mat4 projection;
	glm::mat4 view;
	glm::vec3 camPos;
};
struct UBOParams
{
	glm::vec4 lightDir;
	float exposure = 4.5f;
	float gamma = 2.2f;
	float prefilteredCubeMipLevels = 1.0f;
	float scaleIBLAmbient = 1.0f;
	float debugViewInputs = 0.0f;
	float debugViewEquation = 0.0f;
};


struct BaseRender* BR_Alloc();
void BR_Free(struct BaseRender** r);

void BR_Blur(struct BaseRender* r, GLuint endFbo, int fboX, int fboY, GLuint tex, float blurRadius, GLuint intermediateFBO, int intermediateSizeX, int intermediateSizeY, GLuint intermediateTexture);
void BR_Bloom(struct BaseRender* r, GLuint endFbo, int fboX, int fboY, GLuint intermediateFbo, int interX, int interY, GLuint intermediateTexture, GLuint tex, float blurRadius, float bloomIntensity, int texMipLevel, int endMipLevel);
void BR_Copy(struct BaseRender* r, GLuint tex, int mipMapLevel);
void BR_CopyDual(struct BaseRender* r, GLuint tex1, int mipMapLevel1, GLuint tex2, int mipMapLevel2);
void BR_Upsample(struct BaseRender* r, GLuint tex, int mipMapLevel);
void BR_AmbientOcclusion(struct BaseRender* r, GLuint depthTexture, float depthTexSizeX, float depthTexSizeY);


#endif // BASE_RENDER_H
