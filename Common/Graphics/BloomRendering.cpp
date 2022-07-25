#include "BloomRendering.h"
#include "Helper.h"
#include "logging.h"

// (SOMEWHAT) SAME AS: brdf_lutVertexShader
static const char* bloomVertexShader = "#version 300 es\n\
out vec2 UV;\
out vec2 pos;\
void main()\
{\
	UV = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);\
    pos = UV * 2.0f - 1.0f;\
	gl_Position = vec4(pos, 0.0f, 1.0f);\
}";
static const char* blurFragmentShader = "#version 300 es\n\
precision highp float;\n\
in vec2 UV;\
in vec2 pos;\
uniform sampler2D tex;\
uniform float blurRadius;\
uniform int axis;\
out vec4 outCol;\
void main()\
{\
    vec2 textureSize = vec2(textureSize(tex, 0));\
    float x,y,rr=blurRadius*blurRadius,d,w,w0;\
    vec2 p = 0.5 * (vec2(1.0, 1.0) + pos);\
    vec4 col = vec4(0.0, 0.0, 0.0, 0.0);\
    w0 = 0.5135 / pow(blurRadius, 0.96);\n\
    if (axis == 0) for (d = 1.0 / textureSize.x, x = -blurRadius, p.x += x * d; x <= blurRadius; x++, p.x += d) { w = w0 * exp((-x * x) / (2.0 * rr)); col += texture2D(tex, p) * w; }\n\
    if (axis == 1) for (d = 1.0 / textureSize.y, y = -blurRadius, p.y += y * d; y <= blurRadius; y++, p.y += d) { w = w0 * exp((-y * y) / (2.0 * rr)); col += texture2D(tex, p) * w; }\n\
    outCol = col;\
}";
static const char* bloomFragmentShader = "#version 300 es\n\
precision highp float;\n\
in vec2 UV;\
in vec2 pos;\
uniform sampler2D tex;\
uniform float blurRadius;\
uniform int axis;\
uniform float intensity;\
uniform float mipLevel;\
out vec4 outCol;\
void main()\
{\
    vec2 textureSize = vec2(textureSize(tex, int(mipLevel)));\
    float x,y,rr=blurRadius*blurRadius,d,w,w0;\
    vec2 p = 0.5 * (vec2(1.0, 1.0) + pos);\
    vec4 col = vec4(0.0, 0.0, 0.0, 0.0);\
    w0 = 0.5135 / pow(blurRadius, 0.96);\n\
    if (axis == 0) for (d = 1.0 / textureSize.x, x = -blurRadius, p.x += x * d; x <= blurRadius; x++, p.x += d) { w = w0 * exp((-x * x) / (2.0 * rr));\
    vec3 addCol = textureLod(tex, p, mipLevel).xyz;\
    vec3 remCol = vec3(1.0f) * intensity;\n\
    addCol = addCol - remCol;\
    addCol = vec3(max(addCol.x, 0.0f), max(addCol.y, 0.0f), max(addCol.z, 0.0f));\n\
    col += vec4(addCol, 1.0f) * w;\
 }\n\
    if (axis == 1) for (d = 1.0 / textureSize.y, y = -blurRadius, p.y += y * d; y <= blurRadius; y++, p.y += d) { w = w0 * exp((-y * y) / (2.0 * rr));\
    vec3 addCol = textureLod(tex, p, mipLevel).xyz;\
    vec3 remCol = vec3(1.0f) * intensity;\n\
    addCol = addCol - remCol;\
    addCol = vec3(max(addCol.x, 0.0f), max(addCol.y, 0.0f), max(addCol.z, 0.0f));\n\
    col += vec4(addCol, 1.0f) * w;\
 }\n\
    outCol = col;\
}";
static constexpr char* copyFragmentShader = "#version 300 es\n\
precision highp float;\n\
in vec2 UV;\
in vec2 pos;\
uniform sampler2D tex;\
uniform float mipLevel;\
out vec4 outCol;\
void main()\
{\
    outCol = textureLod(tex, UV, mipLevel);\
}";
static constexpr char* upsamplingFragmentShader = "#version 300 es\n\
precision highp float;\n\
in vec2 UV;\
in vec2 pos;\
uniform sampler2D tex;\
uniform float mipLevel;\
out vec4 outCol;\
void main()\
{\
    vec2 ts = vec2(1.0f) / vec2(textureSize(tex, int(mipLevel)));\
    vec3 c1 = textureLod(tex, UV + vec2(-ts.x, -ts.y), mipLevel).rgb;\
    vec3 c2 = 2.0f * textureLod(tex, UV + vec2(0.0f, -ts.y), mipLevel).rgb;\
    vec3 c3 = textureLod(tex, UV + vec2(ts.x, -ts.y), mipLevel).rgb;\
    vec3 c4 = 2.0f * textureLod(tex, UV + vec2(-ts.x, 0.0f), mipLevel).rgb;\
    vec3 c5 = 4.0f * textureLod(tex, UV + vec2(0.0f, 0.0f), mipLevel).rgb;\
    vec3 c6 = 2.0f * textureLod(tex, UV + vec2(ts.x, 0.0f), mipLevel).rgb;\
    vec3 c7 = textureLod(tex, UV + vec2(-ts.x, ts.y), mipLevel).rgb;\
    vec3 c8 = 2.0f * textureLod(tex, UV + vec2(0.0f, ts.y), mipLevel).rgb;\
    vec3 c9 = textureLod(tex, UV + vec2(ts.x, ts.y), mipLevel).rgb;\
    vec3 accum = 1.0f / 16.0f * (c1 + c2 + c3 + c4 + c5 + c6 + c7 + c8 + c9);\
    outCol = vec4(accum, 1.0f);\
}";
static constexpr char* copyDualFragmentShader = "#version 300 es\n\
precision highp float;\n\
in vec2 UV;\
in vec2 pos;\
uniform sampler2D tex1;\
uniform sampler2D tex2;\
uniform float mipLevel1;\
uniform float mipLevel2;\
out vec4 outCol;\
void main()\
{\
    outCol = textureLod(tex1, UV, mipLevel1) + textureLod(tex2, UV, mipLevel2);\
}";

enum class BLUR_AXIS
{
    X_AXIS,
    Y_AXIS,
};
struct BlurUniforms
{
    GLuint blurRadiusLoc;
    GLuint blurAxisLoc;
};
struct BloomUniforms
{
    BlurUniforms blurUnis;
    GLuint intensityLoc;
    GLuint mipLevelLoc;
};
struct CopyUniforms
{
    GLuint mipLevelLoc;
};
struct CopyDualUniforms
{
    GLuint mipLevel1Loc;
    GLuint mipLevel2Loc;
};
struct UpsamplingUniforms
{
    GLuint mipLevelLoc;
};

struct BloomPipelineData
{
	GLuint blurProgram;
    GLuint bloomProgram;
    GLuint copyProgram;
    GLuint copyDualProgram;
    GLuint upsamplingProgram;
    GLuint bloomIntermediateFBO;
    GLuint bloomIntermediateTexture;
    int intermediateSizeX;
    int intermediateSizeY;
    BlurUniforms blurUnis;
    BloomUniforms bloomUnis;
    CopyUniforms copyUnis;
    CopyDualUniforms copyDualUnis;
    UpsamplingUniforms upsamplingUnis;
}g_bloom;

static void GenerateBloomIntermediates(int sx, int sy)
{
    if (g_bloom.bloomIntermediateFBO)
    {
        glDeleteFramebuffers(1, &g_bloom.bloomIntermediateFBO);
        glDeleteTextures(1, &g_bloom.bloomIntermediateTexture);
    }
    glGenFramebuffers(1, &g_bloom.bloomIntermediateFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, g_bloom.bloomIntermediateFBO);

    glGenTextures(1, &g_bloom.bloomIntermediateTexture);
    glBindTexture(GL_TEXTURE_2D, g_bloom.bloomIntermediateTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, sx, sy, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_bloom.bloomIntermediateTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG("FAILED  TO CREATE FRAMEBUFFER\n");
        std::cout << "hier\n";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, GetDefaultFramebuffer());
    g_bloom.intermediateSizeX = sx;
    g_bloom.intermediateSizeY = sy;
}

void InitializeBloomPipeline()
{
	g_bloom.blurProgram = CreateProgram(bloomVertexShader, blurFragmentShader);
    g_bloom.bloomProgram = CreateProgram(bloomVertexShader, bloomFragmentShader);
    g_bloom.copyProgram = CreateProgram(bloomVertexShader, copyFragmentShader);
    g_bloom.copyDualProgram = CreateProgram(bloomVertexShader, copyDualFragmentShader);
    g_bloom.upsamplingProgram = CreateProgram(bloomVertexShader, upsamplingFragmentShader);
    g_bloom.blurUnis.blurRadiusLoc = glGetUniformLocation(g_bloom.blurProgram, "blurRadius");
    g_bloom.blurUnis.blurAxisLoc = glGetUniformLocation(g_bloom.blurProgram, "axis");
    
    g_bloom.bloomUnis.blurUnis.blurRadiusLoc = glGetUniformLocation(g_bloom.bloomProgram, "blurRadius");
    g_bloom.bloomUnis.blurUnis.blurAxisLoc = glGetUniformLocation(g_bloom.bloomProgram, "axis");
    g_bloom.bloomUnis.intensityLoc = glGetUniformLocation(g_bloom.bloomProgram, "intensity");
    g_bloom.bloomUnis.mipLevelLoc = glGetUniformLocation(g_bloom.bloomProgram, "mipLevel");

    g_bloom.copyUnis.mipLevelLoc = glGetUniformLocation(g_bloom.copyProgram, "mipLevel");

    g_bloom.copyDualUnis.mipLevel1Loc = glGetUniformLocation(g_bloom.copyDualProgram, "mipLevel1");
    g_bloom.copyDualUnis.mipLevel2Loc = glGetUniformLocation(g_bloom.copyDualProgram, "mipLevel2");
    glUseProgram(g_bloom.copyDualProgram);
    GLuint idx = glGetUniformLocation(g_bloom.copyDualProgram, "tex1");
    glUniform1i(idx, 0);
    idx = glGetUniformLocation(g_bloom.copyDualProgram, "tex2");
    glUniform1i(idx, 1);

    g_bloom.upsamplingUnis.mipLevelLoc = glGetUniformLocation(g_bloom.upsamplingProgram, "mipLevel");

    g_bloom.bloomIntermediateTexture = 0;
    g_bloom.bloomIntermediateFBO = 0;

    GenerateBloomIntermediates(1, 1);
}
void CleanUpBloomPipeline()
{
    glDeleteProgram(g_bloom.blurProgram);
    glDeleteFramebuffers(1, &g_bloom.bloomIntermediateFBO);
    glDeleteTextures(1, &g_bloom.bloomIntermediateTexture);
}

void ResizeBloomInternals(int sx, int sy)
{
    GenerateBloomIntermediates(sx, sy);
}

void BlurTextureToFramebuffer(GLuint endFbo, int fboX, int fboY, GLuint tex, float blurRadius)
{
    glBindFramebuffer(GL_FRAMEBUFFER, g_bloom.bloomIntermediateFBO);
    glViewport(0, 0, fboX, fboY);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glUseProgramWrapper(g_bloom.blurProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);


    glUniform1f(g_bloom.blurUnis.blurRadiusLoc, blurRadius);
    glUniform1i(g_bloom.blurUnis.blurAxisLoc, (int)BLUR_AXIS::X_AXIS);
    glDrawArraysWrapper(GL_TRIANGLES, 0, 3);
    
    glBindFramebuffer(GL_FRAMEBUFFER, endFbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindTexture(GL_TEXTURE_2D, g_bloom.bloomIntermediateTexture);
    glUniform1i(g_bloom.blurUnis.blurAxisLoc, (int)BLUR_AXIS::Y_AXIS);
    
    glDrawArraysWrapper(GL_TRIANGLES, 0, 3);
}
void BloomTextureToFramebuffer(GLuint endFbo, int fboX, int fboY, GLuint intermediateFbo, int interX, int interY, GLuint intermediateTexture, GLuint tex, float blurRadius, float bloomIntensity, int texMipLevel, int endMipLevel)
{
    glBindFramebuffer(GL_FRAMEBUFFER, intermediateFbo);
    glViewport(0, 0, interX, interY);
    glUseProgramWrapper(g_bloom.bloomProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);


    glUniform1f(g_bloom.bloomUnis.blurUnis.blurRadiusLoc, blurRadius);
    glUniform1i(g_bloom.bloomUnis.blurUnis.blurAxisLoc, (int)BLUR_AXIS::X_AXIS);
    glUniform1f(g_bloom.bloomUnis.intensityLoc, bloomIntensity);
    glUniform1f(g_bloom.bloomUnis.mipLevelLoc, texMipLevel);

    glDrawArraysWrapper(GL_TRIANGLES, 0, 3);

    glBindFramebuffer(GL_FRAMEBUFFER, endFbo);
    glViewport(0, 0, fboX, fboY);

    glBindTexture(GL_TEXTURE_2D, intermediateTexture);
    glUniform1f(g_bloom.bloomUnis.intensityLoc, 0.0f);
    glUniform1f(g_bloom.bloomUnis.mipLevelLoc, endMipLevel);
    glUniform1i(g_bloom.bloomUnis.blurUnis.blurAxisLoc, (int)BLUR_AXIS::Y_AXIS);

    glDrawArraysWrapper(GL_TRIANGLES, 0, 3);
}
void CopyTextureToFramebuffer(GLuint tex, int mipMapLevel)
{
    glUseProgram(g_bloom.copyProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);

    glUniform1f(g_bloom.copyUnis.mipLevelLoc, mipMapLevel);

    glDrawArraysWrapper(GL_TRIANGLES, 0, 3);
}
void CopyTexturesToFramebuffer(GLuint tex1, int mipMapLevel1, GLuint tex2, int mipMapLevel2)
{
    glUseProgram(g_bloom.copyDualProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex1);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex2);

    glUniform1f(g_bloom.copyDualUnis.mipLevel1Loc, mipMapLevel1);
    glUniform1f(g_bloom.copyDualUnis.mipLevel2Loc, mipMapLevel2);

    glDrawArraysWrapper(GL_TRIANGLES, 0, 3);
}
void UpsampleTextureToFramebuffer(GLuint tex, int mipMapLevel)
{
    glUseProgram(g_bloom.upsamplingProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);

    glUniform1f(g_bloom.upsamplingUnis.mipLevelLoc, mipMapLevel);

    glDrawArraysWrapper(GL_TRIANGLES, 0, 3);
}