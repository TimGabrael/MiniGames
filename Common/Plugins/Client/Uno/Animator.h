#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


enum AnimationType 
{
	ANIMATION_LINEAR,
	ANIMATION_EASE,
	ANIMATION_EASE_IN,
	ANIMATION_EASE_OUT,
};
template<typename T>
struct AnimationState
{
	AnimationState(const T& t, AnimationType ty, float dur) : transform(t), type(ty), duration(dur) {}
	T transform;

	
	AnimationType type;
	float duration;
};
template<typename T>
struct Animation
{
	void AddState(const T& anim, AnimationType animType, float dur) { states.emplace_back(anim, animType, dur); }

	T GetTransform(float deltaTime, bool& finished)
	{
		finished = false;
		float oldDur = curDuration;
		curDuration += deltaTime;

		AnimationState<T>* first = nullptr;
		AnimationState<T>* second = nullptr;
		float curTime = 0.0f;
		for (int i = 0; i < states.size(); i++)
		{
			float d = states.at(i).duration;
			if (!second && curTime <= curDuration && curDuration < curTime + d)
			{
				second = &states.at(i);
				if (i > 0) first = &states.at(i - 1);
				else first = &states.at(0);
			}
			if (second) break;
			curTime += d;
		}

		if (first && second)
		{
			const float normalizedOffset = (curDuration - curTime) / second->duration;

			currentTransform = first->transform * (1.0f - normalizedOffset) + normalizedOffset * second->transform;
		}
		else if (curTime < curDuration)
		{
			finished = true;
			currentTransform = states.at(states.size() - 1).transform;
		}
		return currentTransform;
	}

	T currentTransform;	// gets set in GetTransform
	std::vector<AnimationState<T>> states;
	float curDuration = 0.0f;
};

uint32_t InterpolateInteger(uint32_t i1, uint32_t i2, float idx);
float InterpolateFloat(float x1, float x2, float idx);
uint32_t InterpolateColor(uint32_t c1, uint32_t c2, float idx);

