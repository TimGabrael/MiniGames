#pragma once
#include <glm/glm.hpp>


struct Camera
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
	void SetPerspective(float fov, float aspect, float znear, float zfar);

	void Update();

	void UpdateFromMouseMovement(float dx, float dy);

	void UpdateTouch(int x, int y, int touchID, bool isUp);
	void UpdateTouchMove(int x, int y, int dx, int dy, int touchID);

	void SetMovementDirection(DIRECTION dir, bool isActive);

	glm::vec3 GetFront() const;

	glm::vec3 ScreenToWorld(float x, float y) const;

	glm::mat4 perspective;
	glm::mat4 view;
	glm::vec3 mouseRay;
	glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);
	int screenX, screenY;
private:
	glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	float moveAngle = 0.0f;
	float velocity = 0.0f;


	TouchJoystickData touch[2];

	bool keyboardMoveDirs[4];	// forward, backward, left, right
	bool keyboardDirUsed = false;
	bool touchInputUsed = false;
	float yaw = -90.0f;
	float pitch = 0.0f;
	float roll = 0.0f;
	static constexpr float maxVelocity = 0.3f;
	static constexpr float joystickTurnSpeed = 2.0f;
};