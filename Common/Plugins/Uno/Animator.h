#pragma once

#include <vector>
#include <glm/glm.hpp>


enum AnimationType 
{
	ANIMATION_LINEAR,
	ANIMATION_EASE,
	ANIMATION_EASE_IN,
	ANIMATION_EASE_OUT,
};

struct AnimationState
{
	AnimationState(const glm::mat4& t, AnimationType ty, float dur) : transform(t), type(ty), duration(dur) {}
	glm::mat4 transform;
	AnimationType type;
	float duration;
};
struct Animation
{
	void AddState(const glm::mat4& anim, AnimationType animType, float dur) { states.emplace_back(anim, animType, dur); }
	void AddState(const glm::vec3& pos, float yaw, float pitch, float roll, AnimationType animType, float dur);


	glm::mat4 GetTransform(float deltaTime, bool& finished);

	glm::mat4 currentTransform;	// gets set in GetTransform
	std::vector<AnimationState> states;
	float curDuration = 0.0f;
};
