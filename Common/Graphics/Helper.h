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

GLuint LoadCubemap(const char* right, const char* left, const char* bottom, const char* top, const char* front, const char* back);
GLuint LoadCubemap(const char* file);

GLuint GenerateBRDF_LUT(int dim);


void DrawSkybox(GLuint skybox, const glm::mat4& viewMat, const glm::mat4& projMat);



struct Camera
{
	enum DIRECTION {
		FORWARD,
		LEFT,
		BACKWARD,
		RIGHT,
	};
	void SetRotation(float yaw, float pitch, float roll);
	void SetPerspective(float fov, float aspect, float znear, float zfar);

	void Update();

	void UpdateFromMouseMovement(float dx, float dy);

	void SetMovementDirection(DIRECTION dir, bool isActive);


	glm::mat4 perspective;
	glm::mat4 view;
	glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);
private:
	glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	float moveAngle = 0.0f;
	float velocity = 0.0f;

	bool keyboardMoveDirs[4];	// forward, backward, left, right
	bool keyboardDirUsed = false;
	float yaw = -90.0f;
	float pitch = 0.0f;
	float roll = 0.0f;
	static constexpr float maxVelocity = 0.4f;
};