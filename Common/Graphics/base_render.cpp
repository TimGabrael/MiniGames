#include "base_render.h"
#include "Graphics/GLCompat.h"
#include "shader.h"
#include <stdlib.h>
#include <random>

#define NOISE_DATA_SIZE 4
struct BaseRender {
    Shader s;
    GLuint noiseTexture;

};

enum class BLUR_AXIS
{
    X_AXIS,
    Y_AXIS,
};

struct BaseRender* BR_Alloc() {
    BaseRender* base = (BaseRender*)malloc(sizeof(BaseRender));
    SH_Create(&base->s);

    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;
    glm::vec3* noiseData = new glm::vec3[NOISE_DATA_SIZE * NOISE_DATA_SIZE];
    for (int y = 0; y < NOISE_DATA_SIZE; y++) {
        for (int x = 0; x < NOISE_DATA_SIZE; x++) {
            noiseData[y * NOISE_DATA_SIZE + x] = glm::vec3(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f, 0.0f);
        }
    }
    glGenTextures(1, &base->noiseTexture);
    glBindTexture(GL_TEXTURE_2D, base->noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, NOISE_DATA_SIZE, NOISE_DATA_SIZE, 0, GL_RGB, GL_FLOAT, noiseData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    delete[] noiseData;

    return base;
}
void BR_Free(struct BaseRender** base) {
    if(base && *base) {
        BaseRender* r = *base;
        glDeleteTextures(1, &r->noiseTexture);
        SH_CleanUp(&r->s);
        free(r);
        *base = nullptr;
    }
}


void BR_Blur(struct BaseRender* r, GLuint endFbo, int fboX, int fboY, GLuint tex, float blurRadius, GLuint intermediateFBO, int intermediateSizeX, int intermediateSizeY, GLuint intermediateTexture) {
    glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);
    glViewport(0, 0, fboX, fboY);
    glClear(GL_COLOR_BUFFER_BIT);
    SetOpenGLWeakState(false, false);
    glUseProgramWrapper(r->s.blur);
    SHB_blur_tex(&r->s, tex);

    SHB_blur_blurRadius(&r->s, blurRadius);
    SHB_blur_axis(&r->s, (int)BLUR_AXIS::X_AXIS);
    float uvs[2] = {1.0f, 1.0f};
    SHB_blur_texUV(&r->s, uvs);
    glDrawArraysWrapper(GL_TRIANGLES, 0, 3);
    uvs[0] = (float)fboX / (float)intermediateSizeX; uvs[1] = (float)fboY / (float)intermediateSizeY;
    SHB_blur_texUV(&r->s, uvs);
    glBindFramebuffer(GL_FRAMEBUFFER, endFbo);
    glClear(GL_COLOR_BUFFER_BIT);
    SHB_blur_tex(&r->s, intermediateTexture);
    SHB_blur_axis(&r->s, (int)BLUR_AXIS::Y_AXIS);
    glDrawArraysWrapper(GL_TRIANGLES, 0, 3);
}
void BR_Bloom(struct BaseRender* r, GLuint endFbo, int fboX, int fboY, GLuint intermediateFbo, int interX, int interY, GLuint intermediateTexture, GLuint tex, float blurRadius, float bloomIntensity, int texMipLevel, int endMipLevel) {
    glBindFramebuffer(GL_FRAMEBUFFER, intermediateTexture);
    glViewport(0, 0, interX, interY);
    glUseProgramWrapper(r->s.bloom);
    SHB_bloom_tex(&r->s, tex);
    SHB_bloom_blurRadius(&r->s, blurRadius);
    SHB_bloom_axis(&r->s, (int)BLUR_AXIS::X_AXIS);
    SHB_bloom_intensity(&r->s, bloomIntensity);
    SHB_bloom_mipLevel(&r->s, (GLfloat)texMipLevel);
    glDrawArraysWrapper(GL_TRIANGLES, 0, 3);

    glBindFramebuffer(GL_FRAMEBUFFER, endFbo);
    glViewport(0, 0, fboX, fboY);
    SHB_bloom_tex(&r->s, intermediateTexture);
    SHB_bloom_intensity(&r->s, 0.0f);
    SHB_bloom_mipLevel(&r->s, (GLfloat)endMipLevel);
    SHB_bloom_axis(&r->s, (int)BLUR_AXIS::Y_AXIS);
    glDrawArraysWrapper(GL_TRIANGLES, 0, 3);
}
void BR_Copy(struct BaseRender* r, GLuint tex, int mipMapLevel) {
    glUseProgramWrapper(r->s.copy);
    SHB_copy_tex(&r->s, tex);
    SHB_copy_mipLevel(&r->s, (GLfloat)mipMapLevel);
    glDrawArraysWrapper(GL_TRIANGLES, 0, 3);
}
void BR_CopyDual(struct BaseRender* r, GLuint tex1, int mipMapLevel1, GLuint tex2, int mipMapLevel2) {
    glUseProgramWrapper(r->s.dual_copy);
    SHB_dual_copy_tex1(&r->s, tex1);
    SHB_dual_copy_tex2(&r->s, tex2);
    SHB_dual_copy_mipLevel1(&r->s, (GLfloat)mipMapLevel1);
    SHB_dual_copy_mipLevel2(&r->s, (GLfloat)mipMapLevel2);
    glDrawArraysWrapper(GL_TRIANGLES, 0, 3);
}
void BR_Upsample(struct BaseRender* r, GLuint tex, int mipMapLevel) {
    glUseProgramWrapper(r->s.upsample);
    SHB_upsample_tex(&r->s, tex);
    SHB_upsample_mipLevel(&r->s, (GLfloat)mipMapLevel);
    glDrawArraysWrapper(GL_TRIANGLES, 0, 3);
}
void BR_AmbientOcclusion(struct BaseRender* r, GLuint depthTexture, float depthTexSizeX, float depthTexSizeY) {
    glUseProgramWrapper(r->s.ao);
    float res[2] = {depthTexSizeX, depthTexSizeY};
    SHB_ao_screenResolution(&r->s, res);
    SHB_ao_gDepthMap(&r->s, depthTexture);
    SHB_ao_gRandTex(&r->s, r->noiseTexture);
    glDrawArraysWrapper(GL_TRIANGLES, 0, 6);
}


