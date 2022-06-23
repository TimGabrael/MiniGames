#include "Helper.h"
#include <iostream>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
#include "../logging.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "UiRendering.h"
#include "PbrRendering.h"

#define _USE_MATH_DEFINES
#include <math.h>




const char* brdf_lutVertexShader = "#version 330 core\n\
out vec2 UV;\
void main()\
{\
	UV = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);\
	gl_Position = vec4(UV * 2.0f - 1.0f, 0.0f, 1.0f);\
}";

const char* brdf_lutFragmentShader = "#version 330 core\n\
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






































static const char* cubemapVS = "#version 330 core\n\
layout (location = 0) in vec3  aPos;\n\
out vec3 TexCoords;\n\
\
uniform mat4 projection;\n\
uniform mat4 view;\n\
\
void main(){\
	TexCoords = aPos;\
	vec4 pos = projection * view * vec4(aPos, 0.0);\
	gl_Position = pos.xyww;\
}";

static const char* cubemapFS = "#version 330 core\n\
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
	{-1.0f,-1.0f,-1.0f},{-1.0f,-1.0f, 1.0f},{-1.0f, 1.0f, 1.0f},
	{1.0f, 1.0f,-1.0f},{-1.0f,-1.0f,-1.0f},{-1.0f, 1.0f,-1.0f},
	{1.0f,-1.0f, 1.0f},{-1.0f,-1.0f,-1.0f},{1.0f,-1.0f,-1.0f},
	{1.0f, 1.0f,-1.0f},{1.0f,-1.0f,-1.0f},{-1.0f,-1.0f,-1.0f},
	{-1.0f,-1.0f,-1.0f},{-1.0f, 1.0f, 1.0f},{-1.0f, 1.0f,-1.0f},
	{1.0f,-1.0f, 1.0f},{-1.0f,-1.0f, 1.0f},{-1.0f,-1.0f,-1.0f},
	{-1.0f, 1.0f, 1.0f},{-1.0f,-1.0f, 1.0f},{1.0f,-1.0f, 1.0f},
	{1.0f, 1.0f, 1.0f},{1.0f,-1.0f,-1.0f},{1.0f, 1.0f,-1.0f},
	{1.0f,-1.0f,-1.0f},{1.0f, 1.0f, 1.0f},{1.0f,-1.0f, 1.0f},
	{1.0f, 1.0f, 1.0f},{1.0f, 1.0f,-1.0f},{-1.0f, 1.0f,-1.0f},
	{1.0f, 1.0f, 1.0f},{-1.0f, 1.0f,-1.0f},{-1.0f, 1.0f, 1.0f},
	{1.0f, 1.0f, 1.0f},{-1.0f, 1.0f, 1.0f},{1.0f,-1.0f, 1.0f},
};


struct HelperGlobals
{
	GLuint cubemapProgram;
	GLuint cubeVertexBuffer;
	GLuint skyboxVAO;
	GLint projIdx = 0;
	GLint viewIdx = 0;
}g_helper;




void InitializeOpenGL()
{
	gladLoadGL();

	InitializePbrPipeline();
	InitializeUiPipeline();

	g_helper.cubemapProgram = CreateProgram(cubemapVS, cubemapFS);

	g_helper.projIdx = glGetUniformLocation(g_helper.cubemapProgram, "projection");
	g_helper.viewIdx = glGetUniformLocation(g_helper.cubemapProgram, "view");


	glGenVertexArrays(1, &g_helper.skyboxVAO);
	glGenBuffers(1, &g_helper.cubeVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, g_helper.cubeVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);


	glBindVertexArray(g_helper.skyboxVAO);

	glBindBuffer(GL_ARRAY_BUFFER, g_helper.cubeVertexBuffer);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (const void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLuint CreateProgram(const char* vertexShaderSrc, const char* fragmentShaderSrc)
{
	GLuint resultProgram = 0;
	GLuint vertexShader;
	GLuint fragmentShader;
	int  success;
	char infoLog[512];
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

bool LoadModel(tinygltf::Model& model, const char* filename)
{
	tinygltf::TinyGLTF loader;
	std::string error; std::string warning;
	bool res = false;
	if (memcmp(filename + strnlen_s(filename, 250) - 4, ".glb", 4) == 0)
	{
		res = loader.LoadBinaryFromFile(&model, &error, &warning, filename);
	}
	else
	{
		res = loader.LoadASCIIFromFile(&model, &error, &warning, filename);
	}

	if (!warning.empty()) LOG("WARNING WHILE LOAD GLTF FILE: %s warning: %s\n", filename, warning);
	if (!error.empty()) LOG("FAILED TO LOAD GLTF FILE: %s error: %s\n", filename, error);


	return res;
}


GLuint LoadCubemap(const std::vector<const char*>& faces)
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width = 0; int height = 0;int numChannels = 0;
	for (uint32_t i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces.at(i), &width, &height, &numChannels, 4);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			LOG("FAILED TO LOAD CUBEMAP: %s\n", faces.at(i));
		}
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
	GLuint framebuffer;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	GLuint brdfLut;
	glGenTextures(1, &brdfLut);
	glBindTexture(GL_TEXTURE_2D, brdfLut);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16, dim, dim, 0, GL_RGBA, GL_FLOAT, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);	// not quite sure what i should use here
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);	// not quite sure what i should use here


	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, brdfLut, 0);
	GLenum drawBuffers = GL_COLOR_ATTACHMENT0;
	glDrawBuffers(1, &drawBuffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		LOG("FAILED TO CREATE FRAMEBUFFER OBJECT\n");
		return 0;
	}
	GLuint shader = CreateProgram(brdf_lutVertexShader, brdf_lutFragmentShader);

	glUseProgram(shader);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glViewport(0, 0, dim, dim);




	glDrawArrays(GL_TRIANGLES, 0, 3);

	

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDeleteProgram(shader);
	glDeleteFramebuffers(1, &framebuffer);


	return brdfLut;
}






void DrawSkybox(GLuint skybox, const glm::mat4& viewMat, const glm::mat4& projMat)
{
	glDepthFunc(GL_LEQUAL);
	glUseProgram(g_helper.cubemapProgram);


	glUniformMatrix4fv(g_helper.viewIdx, 1, false, (const GLfloat*)&viewMat);
	glUniformMatrix4fv(g_helper.projIdx, 1, false, (const GLfloat*)&projMat);


	glBindVertexArray(g_helper.skyboxVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);
}


void Camera::SetRotation(float yaw, float pitch, float roll)
{
	this->yaw = yaw; this->pitch = pitch; this->roll = roll;
}
void Camera::SetPerspective(float fov, float aspect, float znear, float zfar)
{
	perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
}
void Camera::Update()
{
	if (keyboardDirUsed)
	{
		bool anyActive = false;
		for (int i = 0; i < 2; i++)
		{
			if (keyboardMoveDirs[i] != keyboardMoveDirs[i + 2]) {
				anyActive = true;
				break;
			}
		}
		if (anyActive) velocity = std::min(velocity + 0.1f, maxVelocity);
		else velocity = std::max(velocity - 0.1f, 0.0f);
		if (keyboardMoveDirs[DIRECTION::FORWARD])
		{
			if (keyboardMoveDirs[DIRECTION::LEFT]) moveAngle = -M_PI / 4.0f;
			else if (keyboardMoveDirs[DIRECTION::RIGHT]) moveAngle = M_PI / 4.0f;
			else moveAngle = 0.0f;
		}
		else if (keyboardMoveDirs[DIRECTION::BACKWARD])
		{
			if (keyboardMoveDirs[DIRECTION::LEFT]) moveAngle = -3.0f * M_PI / 4.0f;
			else if (keyboardMoveDirs[DIRECTION::RIGHT]) moveAngle = -5.0f * M_PI / 4.0f;
			else moveAngle = -M_PI;
		}
		else if (keyboardMoveDirs[DIRECTION::LEFT]) moveAngle = -M_PI / 2.0f;
		else moveAngle = M_PI / 2.0f;

	}
	glm::vec3 right = glm::cross(front, glm::normalize(up + front));
	
	glm::vec3 moveVec = sinf(moveAngle) * right * velocity + cosf(moveAngle) * front * velocity;
	pos += moveVec * velocity;

	view = glm::lookAt(pos, pos + front, up);
}

void Camera::UpdateFromMouseMovement(float dx, float dy)
{
	float sensitivity = 0.1f;
	dx *= sensitivity; dy *= sensitivity;

	yaw += dx; pitch += dy;
	if (pitch > 89.0f) pitch = 89.0f;
	else if (pitch < -89.0f) pitch = -89.0f;

	glm::vec3 fr;
	fr.x = cosf(glm::radians(yaw)) * cosf(glm::radians(pitch));
	fr.y = sinf(glm::radians(pitch));
	fr.z = sinf(glm::radians(yaw)) * cosf(glm::radians(pitch));
	front = glm::normalize(fr);
}

void Camera::SetMovementDirection(DIRECTION dir, bool isActive)
{
	this->keyboardMoveDirs[dir] = isActive;
	keyboardDirUsed = true;
}