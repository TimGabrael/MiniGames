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

struct BloomPipelineData
{
	GLuint program;
    GLuint bloomIntermediateFBO;
    GLuint bloomIntermediateTexture;
    int intermediateSizeX;
    int intermediateSizeY;
    BlurUniforms unis;
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
	g_bloom.program = CreateProgram(bloomVertexShader, blurFragmentShader);
    g_bloom.unis.blurRadiusLoc = glGetUniformLocation(g_bloom.program, "blurRadius");
    g_bloom.unis.blurAxisLoc = glGetUniformLocation(g_bloom.program, "axis");
    
    g_bloom.bloomIntermediateTexture = 0;
    g_bloom.bloomIntermediateFBO = 0;

    GenerateBloomIntermediates(1, 1);
}
void CleanUpBloomPipeline()
{
    glDeleteProgram(g_bloom.program);
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
    glUseProgramWrapper(g_bloom.program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);


    glUniform1f(g_bloom.unis.blurRadiusLoc, blurRadius);
    glUniform1i(g_bloom.unis.blurAxisLoc, (int)BLUR_AXIS::X_AXIS);
    glDrawArraysWrapper(GL_TRIANGLES, 0, 3);
    
    glBindFramebuffer(GL_FRAMEBUFFER, endFbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindTexture(GL_TEXTURE_2D, g_bloom.bloomIntermediateTexture);
    glUniform1i(g_bloom.unis.blurAxisLoc, (int)BLUR_AXIS::Y_AXIS);
    
    glDrawArraysWrapper(GL_TRIANGLES, 0, 3);
}