#pragma once

#include "glad/glad.h"
#include "tiny_gltf.h"
#include "stb_truetype.h"
#include "stb_image_resize.h"
#include <vector>
#include <glm/glm.hpp>

void InitializeOpenGL();


GLuint CreateProgram(const char* vertexShader, const char* fragmentShader);

bool LoadModel(tinygltf::Model& model, const char* filename);

GLuint LoadCubemap(const std::vector<const char*>& faces);

GLuint GenerateBRDF_LUT(int dim);


void DrawSkybox(GLuint skybox, const glm::mat4& viewMat, const glm::mat4& projMat);



struct Camera
{
	void SetRotation(float yaw, float pitch, float roll);
	void SetPerspective(float fov, float aspect, float znear, float zfar);

	void UpdateViewMatrix();	// to update ViewMatrix from current Camera state

	void UpdateFromMouseMovement(float dx, float dy);
	


	glm::mat4 perspective;
	glm::mat4 view;
	glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);
private:
	glm::vec3 front = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	float yaw = 0.0f;
	float pitch = 0.0f;
	float roll = 0.0f;
};