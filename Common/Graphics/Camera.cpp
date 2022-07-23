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

constexpr float MovementComponent::maxVelocity;			// required for android build
constexpr float MovementComponent::joystickTurnSpeed;	// required for android build



void MovementComponent::SetRotation(float yaw, float pitch, float roll)
{
	this->yaw = yaw; this->pitch = pitch; this->roll = roll;
}
void MovementComponent::UpdateFromMouseMovement(float dx, float dy)
{
	float sensitivity = 0.1f;
	dx *= sensitivity; dy *= sensitivity;

	yaw += dx; pitch += dy;
	if (pitch > 89.0f) pitch = 89.0f;
	else if (pitch < -89.0f) pitch = -89.0f;
}
void MovementComponent::UpdateTouch(int sceenWidth, int x, int y, int touchID, bool isUp)
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
		int idx = (x < sceenWidth / 2) ? 0 : 1;
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
void MovementComponent::UpdateTouchMove(int x, int y, int dx, int dy, int touchID)
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
void MovementComponent::SetMovementDirection(DIRECTION dir, bool isActive)
{
	this->keyboardMoveDirs[dir] = isActive;
	keyboardDirUsed = true;
	touchInputUsed = false;
}
void MovementComponent::Update()
{
	glm::vec3 moveVec = { 0.0f, 0.0f, 0.0f };
	glm::vec3 front = { cosf(glm::radians(yaw)) * cosf(glm::radians(pitch)), sinf(glm::radians(pitch)), sinf(glm::radians(yaw)) * cosf(glm::radians(pitch)) };
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
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
		}
	}
}


Camera Camera::GetReflected(const Camera* ref, const glm::vec4& reflectionPlane)
{
	Camera cam = *ref;
	glm::vec3 planeNormal = reflectionPlane;
	planeNormal = glm::normalize(planeNormal);
	cam.front = cam.front - 2 * glm::dot(cam.front, planeNormal) * planeNormal;
	cam.pos -= 2 * (glm::dot(cam.pos, planeNormal) + reflectionPlane.w) * planeNormal;
	cam.SetViewMatrix();
	return cam;
}

void Camera::SetRotation(float yaw, float pitch, float roll)
{
	this->yaw = yaw; this->pitch = pitch; this->roll = roll;
	front.x = cosf(glm::radians(yaw)) * cosf(glm::radians(pitch));
	front.y = sinf(glm::radians(pitch));
	front.z = sinf(glm::radians(yaw)) * cosf(glm::radians(pitch));
	
}
void Camera::SetPerspective(float fov, float aspect, float znear, float zfar)
{
	this->nearClipping = znear; this->farClipping = zfar;
	this->aspectRatio = aspect; this->fieldOfView = fov;
	perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
}
void Camera::Update(const MovementComponent* comp)
{
	pos = comp->pos;
	SetRotation(comp->yaw, comp->pitch, comp->roll);

	view = glm::lookAtRH(pos, pos + front, up);
}
void Camera::SetViewMatrix()
{
	view = glm::lookAtRH(pos, pos + front, up);
}
glm::vec3 Camera::GetFront() const
{
	return front;
}
glm::vec3 Camera::GetUp() const
{
	return up;
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
glm::vec3 Camera::GetRotation() const
{
	return { yaw, pitch, roll };
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

static glm::vec4 NDCToWorld(const glm::mat4& invMat, const glm::vec4& pos)
{
	glm::vec4 world = invMat * pos;
	world = world / world.w;
	return world;
}

OrthographicCamera OrthographicCamera::CreateMinimalFit(const Camera& cam, const glm::vec3& dir, float distance)
{
	OrthographicCamera out;
	
	glm::mat4 invMat = glm::inverse(cam.perspective * cam.view);

	

	glm::vec4 pts[8] = {
		NDCToWorld(invMat, {-1.0f, -1.0f, -1.0f, 1.0f}),
		NDCToWorld(invMat, { 1.0f, -1.0f, -1.0f, 1.0f}),
		NDCToWorld(invMat, { 1.0f,  1.0f, -1.0f, 1.0f}),
		NDCToWorld(invMat, {-1.0f,  1.0f, -1.0f, 1.0f}),
		
		NDCToWorld(invMat, {-1.0f, -1.0f,  1.0f, 1.0f}),
		NDCToWorld(invMat, { 1.0f, -1.0f,  1.0f, 1.0f}),
		NDCToWorld(invMat, { 1.0f,  1.0f,  1.0f, 1.0f}),
		NDCToWorld(invMat, {-1.0f,  1.0f,  1.0f, 1.0f}),
	};
	glm::vec3 center = { 0,0,0 };
	for(int i = 0; i < 8; i++)
	{
		center += glm::vec3(pts[i]);
	}
	center /= 8;
	out.pos = center - dir * distance;
	
	out.view = glm::lookAtRH(out.pos, center, glm::vec3(0.0f, 1.0f, 0.0f));

	for (int i = 0; i < 8; i++)
	{
		pts[i] = out.view * pts[i];
	}

	glm::vec3 min = pts[0];
	glm::vec3 max = pts[0];
	for (int i = 1; i < 8; i++)
	{
		if (pts[i].x < min.x) min.x = pts[i].x;
		if (max.x < pts[i].x) max.x = pts[i].x;
		if (pts[i].y < min.y) min.y = pts[i].y;
		if (max.y < pts[i].y) max.y = pts[i].y;
		if (pts[i].z < min.z) min.z = pts[i].z;
		if (max.z < pts[i].z) max.z = pts[i].z;
	}
	out.proj = glm::orthoLH(min.x, max.x, min.y, max.y, min.z, max.z);


	return out;
}