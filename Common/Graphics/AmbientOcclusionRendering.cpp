#include "AmbientOcclusionRendering.h"
#include "Helper.h"
#include <random>
#include <string>
#include "logging.h"

static const char* ambientOcclusionVertexShader = "#version 300 es\n\
vec2 pos[6] = vec2[6](\
	vec2(-1.0f, -1.0f),\
	vec2( 1.0f, -1.0f),\
	vec2( 1.0f,  1.0f),\
	vec2( 1.0f,  1.0f),\
	vec2(-1.0f,  1.0f),\
	vec2(-1.0f, -1.0f)\
);\
out vec2 TexCoord;\
void main()\
{\
    vec2 Position = pos[gl_VertexID];\
	gl_Position = vec4(Position, 0.0f, 1.0);\
     TexCoord = (Position.xy + vec2(1.0)) / 2.0;\
}";
static const char* ambientOcclusionFragmentShader = "#version 300 es\n\
precision highp float;\n\
in vec2 TexCoord;\
out float FragColor;\
uniform mediump vec2 screenResolution;\
uniform sampler2D gDepthMap;\
uniform sampler2D gRandTex;\
vec3 NormalFromDepth(float depth, vec2 Coords)\
{\
    vec2 offset1 = vec2(0.0, 1.0 / screenResolution.y);\
    vec2 offset2 = vec2(1.0 / screenResolution.x, 0.0);\
    float depth1 = texture(gDepthMap, Coords + offset1).r;\
    float depth2 = texture(gDepthMap, Coords + offset2).r;\
    vec3 p1 = vec3(offset1, depth1-depth);\
    vec3 p2 = vec3(offset2, depth2-depth);\
    vec3 normal = cross(p1, p2);\
    normal.z = -normal.z;\
    return normalize(normal);\
}\
vec3 sample_sphere[16] = vec3[16](\
vec3(0.5381, 0.1856, -0.4319), vec3(0.1379, 0.2486, 0.4430),\
vec3(0.3371, 0.5679, -0.0057), vec3(-0.6999, -0.0451, -0.0019),\
vec3(0.0689, -0.1598, -0.8547), vec3(0.0560, 0.0069, -0.1843),\
vec3(-0.0146, 0.1402, 0.0762), vec3(0.0100, -0.1924, -0.0344),\
vec3(-0.3577, -0.5301, -0.4358), vec3(-0.3169, 0.1063, 0.0158),\
vec3(0.0103, -0.5869, 0.0046), vec3(-0.0897, -0.4940, 0.3287),\
vec3(0.7119, -0.0154, -0.0918), vec3(-0.0533, 0.0596, -0.5411),\
vec3(0.0352, -0.0631, 0.5460), vec3(-0.4776, 0.2847, -0.0271)\
);\
const float total_strength = 1.5;\
const float base = 0.75;\
const float area = 0.025;\
const float falloff = 0.000001;\
const float radius = 0.025;\
void main()\
{\
    float depth = texture(gDepthMap, TexCoord).r;\
    vec3 random = normalize(texture(gRandTex, TexCoord * 64.0).rgb);\
    vec3 position = vec3(TexCoord, depth);\
    vec3 normal = NormalFromDepth(depth, TexCoord);\
    float radiusDepth = radius / depth;\
    float occlusion = 0.0;\
    for(int i = 0; i < 16; i++)\
    {\
        vec3 ray = radiusDepth * reflect(sample_sphere[i], random);\
        vec3 hemiRay = position + sign(dot(ray, normal)) * ray;\
        float occ_depth = texture(gDepthMap, clamp(hemiRay.xy, 0.0, 1.0)).r;\
        float difference = depth - occ_depth;\
        occlusion += step(falloff, difference) * (1.0 - smoothstep(falloff, area, difference));\n\
    }\
    float ao = 1.0 - total_strength * occlusion * (1.0 / 16.0);\
    FragColor = clamp(ao + base, 0.0, 1.0);\n\
}\
";

struct AmbientOcclusionPipelineData
{
    GLuint program;
    GLuint screenResolutionLoc;
    GLuint noiseTexture;
}g_AOpipeline;
float lerp(float v0, float v1, float t);    // currently defined in renderer.cpp
void InitializeAmbientOcclusionPipeline()
{
    g_AOpipeline.program = CreateProgram(ambientOcclusionVertexShader, ambientOcclusionFragmentShader);
    glUseProgram(g_AOpipeline.program);

    g_AOpipeline.screenResolutionLoc = glGetUniformLocation(g_AOpipeline.program, "screenResolution");

    GLuint idx = glGetUniformLocation(g_AOpipeline.program, "gDepthMap");
    glUniform1i(idx, 0);
    idx = glGetUniformLocation(g_AOpipeline.program, "gRandTex");
    glUniform1i(idx, 1);

    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;

    glm::vec3 noiseData[64 * 64];
    for (int y = 0; y < 64; y++)
    {
        for (int x = 0; x < 64; x++)
        {
            noiseData[y * 64 + x] = glm::vec3(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator));
        }
    }
    glGenTextures(1, &g_AOpipeline.noiseTexture);
    glBindTexture(GL_TEXTURE_2D, g_AOpipeline.noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, 64, 64, 0, GL_RGB, GL_FLOAT, noiseData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

}
void CleanUpAmbientOcclusionPipeline()
{
    glDeleteProgram(g_AOpipeline.program);
}

void RenderAmbientOcclusionQuad(GLuint depthTexture, float depthTexSizeX, float depthTexSizeY)
{
    glUseProgramWrapper(g_AOpipeline.program);
    glUniform2f(g_AOpipeline.screenResolutionLoc, depthTexSizeX, depthTexSizeY);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_AOpipeline.noiseTexture);

    glDrawArraysWrapper(GL_TRIANGLES, 0, 6);
}