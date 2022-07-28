#pragma once
#include <glm/glm.hpp>
#include "GLCompat.h"

struct MovementComponent
{
	struct TouchJoystickData
	{
		int initialX;
		int initialY;
		float stateX;	// states are between [-1, 1]
		float stateY;
		int touchID = -1;
	};
	enum DIRECTION {
		FORWARD,
		LEFT,
		BACKWARD,
		RIGHT,
	};

	void SetRotation(float yaw, float pitch, float roll);

	// input functions
	void UpdateFromMouseMovement(float dx, float dy);
	void UpdateTouch(int sceenWidth, int x, int y, int touchID, bool isUp);
	void UpdateTouchMove(int x, int y, int dx, int dy, int touchID);
	void SetMovementDirection(DIRECTION dir, bool isActive);


	// Frame Function
	void Update();

	glm::vec3 mouseRay;
	glm::vec3 prevMouseRay;

	glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);
	float yaw = -90.0f;
	float pitch = 0.0f;
	float roll = 0.0f;

	float moveAngle = 0.0f;
	float velocity = 0.0f;

	TouchJoystickData touch[2];

	bool keyboardMoveDirs[4] = { false };	// forward, backward, left, right
	bool keyboardDirUsed = false;
	bool touchInputUsed = false;

	static constexpr float maxVelocity = 0.3f;
	static constexpr float joystickTurnSpeed = 2.0f;
};


struct Camera
{
	static Camera GetReflected(const Camera* ref, const glm::vec4& reflectionPlane);
	void SetRotation(float yaw, float pitch, float roll);
	void SetPerspective(float fov, float aspect, float znear, float zfar);

	void Update(const MovementComponent* comp);
	void SetViewMatrix();

	glm::vec3 GetFront() const;
	glm::vec3 GetUp() const; 
	glm::vec3 GetRealUp() const;	// independent of the up vector used in the transformations
	glm::vec3 GetRight() const;

	float GetYaw() const;
	glm::vec3 GetRotation() const;
	glm::vec2 GetFrustrumSquare(float distance) const;

	glm::vec3 ScreenToWorld(float x, float y) const;

	glm::mat4 perspective;
	glm::mat4 view;
	glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);

	glm::mat4 viewProj;
	GLuint uniform;

	int screenX, screenY;
private:

	glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

	float yaw = -90.0f;
	float pitch = 0.0f;
	float roll = 0.0f;


	float nearClipping = 0.0f;
	float farClipping = 0.0f;
	float aspectRatio = 0.0f;
	float fieldOfView = 0.0f;
};


struct OrthographicCamera
{
	static OrthographicCamera CreateMinimalFit(const Camera& cam, const glm::vec3& dir, float distance);

	glm::mat4 proj;
	glm::mat4 view;
	glm::vec3 pos;

	glm::mat4 viewProj;

	GLuint uniform;

};