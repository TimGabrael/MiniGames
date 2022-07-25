#include "Helper.h"



#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "../logging.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "UiRendering.h"
#include "PbrRendering.h"
#include "Simple3DRendering.h"
#include "ReflectiveSurfaceRendering.h"
#include "BloomRendering.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include "FileQuery.h"


static const char* brdf_lutVertexShader = "#version 300 es\n\
out vec2 UV;\
void main()\
{\
	UV = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);\
	gl_Position = vec4(UV * 2.0f - 1.0f, 0.0f, 1.0f);\
}";

static const char* brdf_lutFragmentShader = "#version 300 es\n\
precision highp float;\n\
in vec2 UV;\n\
out vec4 outColor;\n\
const uint NUM_SAMPLES = 1024u;\n\
\n\
const float PI = 3.1415926536;\n\
\n\
float random(vec2 co)\n\
{\n\
	float a = 12.9898;\n\
	float b = 78.233;\n\
	float c = 43758.5453;\n\
	float dt = dot(co.xy, vec2(a, b));\n\
	float sn = mod(dt, 3.14);\n\
	return fract(sin(sn) * c);\n\
}\n\
\n\
vec2 hammersley2d(uint i, uint N)\n\
{\n\
	uint bits = (i << 16u) | (i >> 16u);\n\
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);\n\
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);\n\
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);\n\
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);\n\
	float rdi = float(bits) * 2.3283064365386963e-10;\n\
	return vec2(float(i) / float(N), rdi);\n\
}\n\
\n\
vec3 importanceSample_GGX(vec2 Xi, float roughness, vec3 normal)\n\
{\n\
	float alpha = roughness * roughness;\n\
	float phi = 2.0 * PI * Xi.x + random(normal.xz) * 0.1;\n\
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha * alpha - 1.0) * Xi.y));\n\
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);\n\
	vec3 H = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);\n\
\n\
	vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);\n\
	vec3 tangentX = normalize(cross(up, normal));\n\
	vec3 tangentY = normalize(cross(normal, tangentX));\n\
	return normalize(tangentX * H.x + tangentY * H.y + normal * H.z);\n\
}\n\
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)\n\
{\n\
	float k = (roughness * roughness) / 2.0;\n\
	float GL = dotNL / (dotNL * (1.0 - k) + k);\n\
	float GV = dotNV / (dotNV * (1.0 - k) + k);\n\
	return GL * GV;\n\
}\n\
\n\
vec2 BRDF(float NoV, float roughness)\n\
{\n\
	const vec3 N = vec3(0.0, 0.0, 1.0);\n\
	vec3 V = vec3(sqrt(1.0 - NoV * NoV), 0.0, NoV);\n\
	vec2 LUT = vec2(0.0);\n\
	for (uint i = 0u; i < NUM_SAMPLES; i++) {\n\
		vec2 Xi = hammersley2d(i, NUM_SAMPLES);\n\
		vec3 H = importanceSample_GGX(Xi, roughness, N);\n\
		vec3 L = 2.0 * dot(V, H) * H - V;\n\
		float dotNL = max(dot(N, L), 0.0);\n\
		float dotNV = max(dot(N, V), 0.0);\n\
		float dotVH = max(dot(V, H), 0.0);\n\
		float dotNH = max(dot(H, N), 0.0);\n\
		if (dotNL > 0.0) {\n\
			float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);\n\
			float G_Vis = (G * dotVH) / (dotNH * dotNV);\n\
			float Fc = pow(1.0 - dotVH, 5.0);\n\
			LUT += vec2((1.0 - Fc) * G_Vis, Fc * G_Vis);\n\
		}\n\
	}\n\
	return LUT / float(NUM_SAMPLES);\n\
}\n\
void main()\n\
{\
	outColor = vec4(BRDF(UV.s, 1.0 - UV.t), 0.0, 1.0);\
}";
static const char* emptyFragmentShader = "#version 300 es\n\
precision highp float;\n\
void main()\
{\
}";





































static const char* cubemapVS = "#version 300 es\n\
layout (location = 0) in vec3  aPos;\n\
out vec3 TexCoords;\n\
\
uniform mat4 viewProj;\n\
\
void main(){\
	TexCoords = aPos;\
	vec4 pos = viewProj * vec4(aPos, 0.0);\n\
	gl_Position = pos.xyww;\
}";

static const char* cubemapFS = "#version 300 es\n\
precision highp float;\n\
out vec4 FragColor;\n\
\
in vec3 TexCoords;\n\
\
uniform samplerCube skybox;\n\
void main()\
{\
	FragColor = texture(skybox, TexCoords);\n\
}";

//  x------x
// /|     /|
// x------x|
// ||	  ||
// |x-----|x
// x------x
static glm::vec3 cubeVertices[] = {

	{-1.0f, 1.0f, 1.0f},{-1.0f,-1.0f, 1.0f},{-1.0f,-1.0f,-1.0f},
	{-1.0f, 1.0f,-1.0f},{-1.0f,-1.0f,-1.0f},{1.0f, 1.0f,-1.0f},
	{1.0f,-1.0f,-1.0f},{-1.0f,-1.0f,-1.0f},{1.0f,-1.0f, 1.0f},


	{-1.0f,-1.0f,-1.0f},{1.0f,-1.0f,-1.0f},{1.0f, 1.0f,-1.0f},
	{-1.0f, 1.0f,-1.0f},{-1.0f, 1.0f, 1.0f},{-1.0f,-1.0f,-1.0f},
	{-1.0f,-1.0f,-1.0f},{-1.0f,-1.0f, 1.0f},{1.0f,-1.0f, 1.0f},


	{1.0f,-1.0f, 1.0f},{-1.0f,-1.0f, 1.0f},{-1.0f, 1.0f, 1.0f},
	{1.0f, 1.0f,-1.0f},{1.0f,-1.0f,-1.0f},{1.0f, 1.0f, 1.0f},
	{1.0f,-1.0f, 1.0f},{1.0f, 1.0f, 1.0f},{1.0f,-1.0f,-1.0f},


	{-1.0f, 1.0f,-1.0f},{1.0f, 1.0f,-1.0f},{1.0f, 1.0f, 1.0f},
	{-1.0f, 1.0f, 1.0f},{-1.0f, 1.0f,-1.0f},{1.0f, 1.0f, 1.0f},
	{1.0f,-1.0f, 1.0f},{-1.0f, 1.0f, 1.0f},{1.0f, 1.0f, 1.0f},
};


struct HelperGlobals
{
	GLuint cubemapProgram;
	GLuint cubeVertexBuffer;
	GLuint skyboxVAO;
	GLint viewProjIdx = 0;
	void* assetManager = nullptr;
}g_helper;

stbi_uc* stbi_load_wrapper(const char* file, int* width, int* height, int* numChannels, int req_comp)
{
	FileContent content = LoadFileContent(g_helper.assetManager, file);
	if (!content.data)  return nullptr;
	stbi_uc* retVal = stbi_load_from_memory(content.data, content.size, width, height, numChannels, req_comp);
	delete[] content.data;
	return retVal;
}



void InitializeOpenGL(void* assetManager)
{
#if !defined(ANDROID) and !defined(EMSCRIPTEN)
	gladLoadGL();
#endif
	g_helper.assetManager = assetManager;
	
	InitializePbrPipeline(assetManager);
	InitializeUiPipeline();
	InitializeSimple3DPipeline();
	InitializeReflectiveSurfacePipeline();
	InitializeBloomPipeline();

	
	g_helper.cubemapProgram = CreateProgram(cubemapVS, cubemapFS);
	
	g_helper.viewProjIdx = glGetUniformLocation(g_helper.cubemapProgram, "viewProj");
	
	
	glGenVertexArrays(1, &g_helper.skyboxVAO);
	glGenBuffers(1, &g_helper.cubeVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, g_helper.cubeVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
	
	glUseProgramWrapper(g_helper.cubemapProgram);
	GLint curTexture = glGetUniformLocation(g_helper.cubemapProgram, "skybox");
	glUniform1i(curTexture, 0);

	glBindVertexArray(g_helper.skyboxVAO);
	
	glBindBuffer(GL_ARRAY_BUFFER, g_helper.cubeVertexBuffer);
	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (const void*)0);
	glEnableVertexAttribArray(0);
	
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgramWrapper(0);
}
void CleanUpOpenGL()
{
	CleanUpPbrPipeline();
	CleanUpUiPipeline();
	CleanUpSimple3DPipeline();
	glDeleteBuffers(1, &g_helper.cubeVertexBuffer);
	glDeleteProgram(g_helper.cubemapProgram);
	glDeleteVertexArrays(1, &g_helper.skyboxVAO);
}

GLuint CreateProgram(const char* vertexShaderSrc, const char* fragmentShaderSrc)
{
	GLuint resultProgram = 0;
	GLuint vertexShader;
	GLuint fragmentShader;
	int  success;
	char infoLog[512];
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSrc, NULL);
	glCompileShader(vertexShader);
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		LOG("ERROR::SHADER::VERTEX::COMPILATION_FAILED %s\n", infoLog);
		glDeleteShader(vertexShader);
		return 0;
	}

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSrc, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		LOG("ERROR::SHADER::VERTEX::COMPILATION_FAILED %s\n", infoLog);
		glDeleteShader(vertexShader); glDeleteShader(fragmentShader);
		return 0;
	}

	resultProgram = glCreateProgram();
	glAttachShader(resultProgram, vertexShader);
	glAttachShader(resultProgram, fragmentShader);
	glLinkProgram(resultProgram);

	glGetProgramiv(resultProgram, GL_LINK_STATUS, &success);

	glDeleteShader(vertexShader); glDeleteShader(fragmentShader);

	if (!success)
	{
		glGetProgramInfoLog(resultProgram, 512, NULL, infoLog);
		LOG("ERROR::SHADER::PROGRAM::LINKING_FAILED %s\n", infoLog);
		return 0;
	}
	return resultProgram;
}
GLuint CreateProgram(const char* vertexShader)
{
	return CreateProgram(vertexShader, emptyFragmentShader);
}



GLuint LoadCubemap(const char* right, const char* left, const char* top, const char* bottom, const char* front, const char* back)
{
	const char* faces[6] = { right, left, top, bottom, front, back };
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width = 0; int height = 0; int numChannels = 0;
	for (uint32_t i = 0; i < 6; i++)
	{
		unsigned char* data = stbi_load_wrapper(faces[i], &width, &height, &numChannels, 4);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			LOG("FAILED TO LOAD CUBEMAP: %s\n", faces[i]);
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}
GLuint LoadCubemap(const char* file)
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width = 0; int height = 0; int numChannels = 0;
	unsigned char* data = stbi_load_wrapper(file, &width, &height, &numChannels, 4);
	
	if (data)
	{
		stbi_image_free(data);
	}
	
	
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}


GLuint GenerateBRDF_LUT(int dim)
{
	glDisable(GL_DEPTH_TEST);
	GLuint framebuffer;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	GLuint brdfLut;
	glGenTextures(1, &brdfLut);
	glBindTexture(GL_TEXTURE_2D, brdfLut);
	
#ifdef __glad_h_
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, dim, dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
#else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dim, dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
#endif
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// not quite sure what i should use here
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	// not quite sure what i should use here
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	


	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLut, 0);
	GLenum drawBuffers = GL_COLOR_ATTACHMENT0;
	glDrawBuffers(1, &drawBuffers);
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		LOG("FAILED TO CREATE FRAMEBUFFER OBJECT, STATUS: %d\n", status);
		return 0;
	}
	GLuint shader = CreateProgram(brdf_lutVertexShader, brdf_lutFragmentShader);

	glUseProgramWrapper(shader);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glViewport(0, 0, dim, dim);



	glDrawArraysWrapper(GL_TRIANGLES, 0, 3);

	glBindFramebuffer(GL_FRAMEBUFFER, GetDefaultFramebuffer());


	glDeleteProgram(shader);
	glDeleteFramebuffers(1, &framebuffer);
	glEnable(GL_DEPTH_TEST);


	return brdfLut;
}






void DrawSkybox(GLuint skybox, const glm::mat4& viewMat, const glm::mat4& projMat)
{
	glDepthFunc(GL_LEQUAL);
	glUseProgramWrapper(g_helper.cubemapProgram);

	const glm::mat4 viewProjMat = projMat * viewMat;

	glUniformMatrix4fv(g_helper.viewProjIdx, 1, false, (const GLfloat*)&viewProjMat);


	glBindVertexArray(g_helper.skyboxVAO);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox);
	glDrawArraysWrapper(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);
}
void DrawSkybox(GLuint skybox, const glm::mat4& viewProjMat)
{
	glDepthFunc(GL_LEQUAL);
	glUseProgramWrapper(g_helper.cubemapProgram);

	glUniformMatrix4fv(g_helper.viewProjIdx, 1, false, (const GLfloat*)&viewProjMat);


	glBindVertexArray(g_helper.skyboxVAO);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox);
	glDrawArraysWrapper(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);
}






void StreamArrayBuffer::Init()
{
	glBuf = 0;
	writeIdx = 0;
	size = 0;
	mapped = nullptr;
	glGenBuffers(1, &glBuf);
	glBindBuffer(GL_ARRAY_BUFFER, glBuf);
}
void StreamArrayBuffer::CleanUp()
{
	if (mapped)
	{
		glBindBuffer(GL_ARRAY_BUFFER, glBuf);
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}
	glDeleteBuffers(1, &glBuf);
}
void StreamArrayBuffer::Bind()
{
	if (mapped) {
		glBindBuffer(GL_ARRAY_BUFFER, glBuf);
		glUnmapBuffer(GL_ARRAY_BUFFER);
		mapped = nullptr;
		writeIdx = 0;
	}
}
void StreamArrayBuffer::Append(const void* data, uint32_t dataSize, uint32_t sizeSteps)
{
	if (writeIdx + dataSize >= size)
	{
		const int increaseSize = (dataSize / sizeSteps + 1) * sizeSteps;
		const int newSize = std::max(size + increaseSize, writeIdx + increaseSize);
		if (mapped)
		{
			uint8_t* newData = new uint8_t[newSize];
			memcpy(newData, mapped, writeIdx);
			memcpy(newData + writeIdx, data, dataSize);

			glBindBuffer(GL_ARRAY_BUFFER, glBuf);
			glUnmapBuffer(GL_ARRAY_BUFFER);

			size = newSize;
			glBufferData(GL_ARRAY_BUFFER, size, newData, GL_DYNAMIC_DRAW);
			delete[] newData;
			mapped = glMapBufferRange(GL_ARRAY_BUFFER, 0, size, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
			return;
		}
		else
		{
			size = newSize;
			glBindBuffer(GL_ARRAY_BUFFER, glBuf);
			glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
			mapped = glMapBufferRange(GL_ARRAY_BUFFER, 0, size, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
		}
	}
	if (!mapped)
	{
		glBindBuffer(GL_ARRAY_BUFFER, glBuf);
		mapped = glMapBufferRange(GL_ARRAY_BUFFER, 0, size, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
	}

	memcpy((void*)((uintptr_t)mapped + writeIdx), data, dataSize);
	writeIdx += dataSize;
}



PFUNCDRAWSCENEOBJECT TEMPORARY_DRAW_RETRIEVER(TYPE_FUNCTION f)
{
	return nullptr;
}
PScene CreateAndInitializeSceneAsDefault()
{
	PScene scene = SC_CreateScene();
	const uint32_t indexS3D = SC_AddType(scene, S3DGetDrawFunctions);
	assert(indexS3D == 0);
	const uint32_t indexPBR = SC_AddType(scene, TEMPORARY_DRAW_RETRIEVER);		// TODO ADD GET DRAW FUNCTION 
	assert(indexPBR == 1);
	const uint32_t indexReflect = SC_AddType(scene, ReflectiveSurfaceGetDrawFunction);	// TODO ADD GET DRAW FUNCTION 
	assert(indexReflect == 2);

	return scene;
}

SingleFBO CreateSingleFBO(int width, int height)
{
	SingleFBO out;
	glGenFramebuffers(1, &out.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, out.fbo);


	glGenTextures(1, &out.texture);
	glBindTexture(GL_TEXTURE_2D, out.texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, out.texture, 0);



	glGenTextures(1, &out.depth);
	glBindTexture(GL_TEXTURE_2D, out.depth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, out.depth, 0);


	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		LOG("FAILED  TO CREATE FRAMEBUFFER\n");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, GetDefaultFramebuffer());

	return out;
}
DepthFBO CreateDepthFBO(int width, int height)
{
	DepthFBO out;
	glGenFramebuffers(1, &out.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, out.fbo);

	glGenTextures(1, &out.depth);
	glBindTexture(GL_TEXTURE_2D, out.depth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	GLfloat borderColor = 1.0f;
#ifdef ANDROID
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_EXT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER_EXT);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR_EXT, &borderColor);
#else
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &borderColor);
#endif
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, out.depth, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		LOG("FAILED  TO CREATE FRAMEBUFFER\n");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, GetDefaultFramebuffer());

	return out;
}
void RecreateSingleFBO(SingleFBO* fbo, int width, int height)
{
	DestroySingleFBO(fbo);
	*fbo = CreateSingleFBO(width, height);
}
void DestroySingleFBO(SingleFBO* fbo)
{
	glDeleteFramebuffers(1, &fbo->fbo);
	glDeleteTextures(1, &fbo->texture);
	glDeleteTextures(1, &fbo->depth);
}
void DestroyDepthFBO(DepthFBO* fbo)
{
	glDeleteFramebuffers(1, &fbo->fbo);
	glDeleteTextures(1, &fbo->depth);
}


void BloomFBO::Create(int drawFboSx, int drawFboSy, int sx, int sy)
{
	sizeX = sx;
	sizeY = sy;
	glGenFramebuffers(1, &defaultFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, defaultFBO);

	glGenTextures(1, &defaultDepth);
	glBindTexture(GL_TEXTURE_2D, defaultDepth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, drawFboSx, drawFboSy, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, defaultDepth, 0);

	glGenTextures(1, &defaultTexture);
	glBindTexture(GL_TEXTURE_2D, defaultTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, drawFboSx, drawFboSy, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, defaultTexture, 0);

	
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		LOG("FAILED  TO CREATE FRAMEBUFFER\n");
	}
	


	numBloomFbos = 1 + floor(log2(std::max(sx, sy)));
	int curX = sx; int curY = sy;
	for (int i = 1; i < numBloomFbos; i++)
	{
		curX = std::max((curX >> 1), 1);
		curY = std::max((curY >> 1), 1);
		if (curX < 16 && curY < 16 && false)
		{
			numBloomFbos = i;
			break;
		}
	}
	numBloomFbos = std::min(numBloomFbos, 8);	// NO MORE THEN 8 PLEASE!
	{
		this->bloomFBOs1 = new GLuint[numBloomFbos];
		glGenFramebuffers(numBloomFbos, bloomFBOs1);

		glGenTextures(1, &bloomTexture1);
		glBindTexture(GL_TEXTURE_2D, bloomTexture1);
		glTexStorage2D(GL_TEXTURE_2D, numBloomFbos, GL_RGBA16F, sx, sy);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		for (int i = 0; i < numBloomFbos; i++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, bloomFBOs1[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomTexture1, i);
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
				LOG("FAILED  TO CREATE FRAMEBUFFER\n");
			}
		}
	}
	{
		this->bloomFBOs2 = new GLuint[numBloomFbos];
		glGenFramebuffers(numBloomFbos, bloomFBOs2);

		glGenTextures(1, &bloomTexture2);
		glBindTexture(GL_TEXTURE_2D, bloomTexture2);
		glTexStorage2D(GL_TEXTURE_2D, numBloomFbos, GL_RGBA16F, sx, sy);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		for (int i = 0; i < numBloomFbos; i++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, bloomFBOs2[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomTexture2, i);
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
				LOG("FAILED  TO CREATE FRAMEBUFFER\n");
			}
		}
	}


	glBindFramebuffer(GL_FRAMEBUFFER, GetDefaultFramebuffer());

}
void BloomFBO::Resize(int drawFboSx, int drawFboSy, int sx, int sy)
{
	CleanUp();
	Create(drawFboSx, drawFboSy, sx, sy);
}
void BloomFBO::CleanUp()
{
	if (bloomFBOs1)
	{
		glDeleteFramebuffers(numBloomFbos, bloomFBOs1);
		delete[] bloomFBOs1;
		bloomFBOs1 = nullptr;
	}
	if (bloomFBOs2)
	{
		glDeleteFramebuffers(numBloomFbos, bloomFBOs2);
		delete[] bloomFBOs2;
		bloomFBOs2 = nullptr;
	}
	glDeleteFramebuffers(1, &defaultFBO);
	glDeleteTextures(1, &bloomTexture1);
	glDeleteTextures(1, &bloomTexture2);
	glDeleteTextures(1, &defaultTexture);
	glDeleteTextures(1, &defaultDepth);
}