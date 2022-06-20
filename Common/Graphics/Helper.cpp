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


void InitializeOpenGL()
{
	gladLoadGL();
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
	glDeleteShader(vertexShader); glDeleteShader(fragmentShader);


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