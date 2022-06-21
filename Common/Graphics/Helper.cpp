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



static const char* cubemapVS = "#version 330 core\n\
layout (location = 0) in vec3  aPos;\n\
out vec3 TexCoords;\n\
\
uniform mat4 projection;\n\
uniform mat4 view;\n\
\
void main(){\
	TexCoords = aPos;\
	vec4 pos = projection * view * vec4(aPos, 1.0);\
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

void Camera::SetPosition(float x, float y, float z)
{
	pos.x = x; pos.y = y; pos.z = z;
}
void Camera::SetRotation(float yaw, float pitch, float roll)
{
	this->yaw = yaw; this->pitch = pitch; this->roll = roll;
}
void Camera::SetPerspective(float fov, float aspect, float znear, float zfar)
{
	perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
}
void Camera::UpdateViewMatrix()
{
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

	UpdateViewMatrix();
}