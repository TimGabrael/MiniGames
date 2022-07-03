#include "Camera.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#define _USE_MATH_DEFINES
#include <math.h>

#include "logging.h"

constexpr float Camera::maxVelocity;		// required for android build
constexpr float Camera::joystickTurnSpeed; // required for android build



void Camera::SetRotation(float yaw, float pitch, float roll)
{
	this->yaw = yaw; this->pitch = pitch; this->roll = roll;
	glm::vec3 fr;
	fr.x = cosf(glm::radians(yaw)) * cosf(glm::radians(pitch));
	fr.y = sinf(glm::radians(pitch));
	fr.z = sinf(glm::radians(yaw)) * cosf(glm::radians(pitch));
	front = glm::normalize(fr);
}
void Camera::SetPerspective(float fov, float aspect, float znear, float zfar)
{
	this->nearClipping = znear; this->farClipping = zfar;
	this->aspectRatio = aspect; this->fieldOfView = fov;
	perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
}
void Camera::Update()
{
	glm::vec3 moveVec = { 0.0f, 0.0f, 0.0f };
	glm::vec3 right = glm::normalize(glm::cross(front, up));
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

		glm::vec3 moveVec = sinf(moveAngle) * right + cosf(moveAngle) * front;
		pos += moveVec * velocity;
	}
	else if (touchInputUsed)
	{

		if (touch[0].touchID != -1)
		{
			glm::vec3 moveVec = touch[0].stateX * right + touch[0].stateY * front;
			pos += moveVec * maxVelocity;
		}
		if (touch[1].touchID != -1)
		{
			yaw += touch[1].stateX * joystickTurnSpeed;
			pitch += touch[1].stateY * joystickTurnSpeed;
			glm::vec3 fr;
			fr.x = cosf(glm::radians(yaw)) * cosf(glm::radians(pitch));
			fr.y = sinf(glm::radians(pitch));
			fr.z = sinf(glm::radians(yaw)) * cosf(glm::radians(pitch));
			front = glm::normalize(fr);
		}
	}

	view = glm::lookAtRH(pos, pos + front, up);
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
void Camera::UpdateTouch(int x, int y, int touchID, bool isUp)
{
	if (isUp)
	{
		for (int i = 0; i < 2; i++)
		{
			if (touch[i].touchID == touchID)
			{
				touch[i].touchID = -1;
				touch[i].stateX = 0.0f;
				touch[i].stateY = 0.0f;
				break;
			}
		}
		if ((touch[0].touchID == -1 && touch[1].touchID == -1))
		{
			touchInputUsed = false;
		}
	}
	else
	{
		int idx = (x < screenX / 2) ? 0 : 1;
		if (touch[idx].touchID == -1)
		{
			touch[idx].initialX = x;
			touch[idx].initialY = y;
			touch[idx].stateX = 0.0f;
			touch[idx].stateY = 0.0f;
			touch[idx].touchID = touchID;
			memset(keyboardMoveDirs, 0, 4);
			touchInputUsed = true;
		}
	}
}
void Camera::UpdateTouchMove(int x, int y, int dx, int dy, int touchID)
{
	static constexpr float MAX_RADIUS = 60.0f;
	TouchJoystickData* tid = nullptr;
	for (int i = 0; i < 2; i++) {
		if (touch[i].touchID == touchID) {
			tid = &touch[i];
			break;
		}
	}
	if (!tid) return;

	const float rx = (float)(x - tid->initialX);
	const float ry = (float)(tid->initialY - y);

	const float dist = sqrtf(rx * rx + ry * ry);
	if (dist == 0.0f) return;


	float relX = rx / dist;
	float relY = ry / dist;

	const float radClipped = std::min(MAX_RADIUS, dist);

	tid->stateX = relX * radClipped / MAX_RADIUS;
	tid->stateY = relY * radClipped / MAX_RADIUS;
	touchInputUsed = true;

}

void Camera::SetMovementDirection(DIRECTION dir, bool isActive)
{
	this->keyboardMoveDirs[dir] = isActive;
	keyboardDirUsed = true;
	touchInputUsed = false;
}
glm::vec3 Camera::GetFront() const
{
	return front;
}
glm::vec3 Camera::GetRealUp() const
{
	float upPitch = pitch + 90.0f;
	glm::vec3 realUp;
	realUp.x = cosf(glm::radians(yaw)) * cosf(glm::radians(upPitch));
	realUp.y = sinf(glm::radians(upPitch));
	realUp.z = sinf(glm::radians(yaw)) * cosf(glm::radians(upPitch));
	return realUp;
}
glm::vec3 Camera::GetRight() const
{
	return glm::normalize(glm::cross(front, up));
}
float Camera::GetYaw() const
{
	return yaw;
}
glm::vec2 Camera::GetFrustrumSquare(float distance) const
{
	float heightCut = 2.0f * tanf(glm::radians(fieldOfView) / 2.0f) * distance;
	return glm::vec2(heightCut * aspectRatio, heightCut);
}
glm::vec3 Camera::ScreenToWorld(float x, float y) const
{
	glm::vec4 res;

	float nx = (x / (float)screenX) * 2.0f - 1.0f;
	float ny = 1.0f - (y / (float)screenY) * 2.0f;

	res = glm::vec4(nx, ny, -1.0f, 1.0f);
	
	glm::mat4 invProj = glm::inverse(perspective);
	res = invProj * res;
	res.z = -1.0f; res.w = 0.0f;

	glm::mat4 invView = glm::inverse(view);
	res = glm::normalize(invView * res);

	return res;
}