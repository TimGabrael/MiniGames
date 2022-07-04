#include "Animator.h"
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "logging.h"

void Animation::AddState(const glm::vec3& pos, float yaw, float pitch, float roll, AnimationType animType, float dur)
{
	glm::mat4 mat = glm::translate(glm::mat4(1.0f), pos)
		* glm::rotate(glm::mat4(1.0f), glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f))
		* glm::rotate(glm::mat4(1.0f), glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f))
		* glm::rotate(glm::mat4(1.0f), glm::radians(roll), glm::vec3(0.0f, 0.0f, 1.0f));
	states.emplace_back(mat, animType, dur);
}

glm::mat4 Animation::GetTransform(float deltaTime, bool& finished)
{
	finished = false;
	float oldDur = curDuration;
	curDuration += deltaTime;

	AnimationState* first = nullptr;
	AnimationState* second = nullptr;
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



	glm::mat4 out(1.0f);
	if (first && second)
	{
		const float normalizedOffset = (curDuration - curTime) / second->duration;

		out = first->transform * (1.0f - normalizedOffset) + normalizedOffset * second->transform;
	}
	else if (curTime < curDuration)
	{
		finished = true;
		currentTransform = states.at(states.size() - 1).transform;
		return states.at(states.size() - 1).transform;
	}

	currentTransform = out;
	return out;
}

uint32_t InterpolateInteger(uint32_t i1, uint32_t i2, float idx)
{
	return i1 * (1.0f - idx) + i2 * idx;
}
float InterpolateFloat(float x1, float x2, float idx)
{
	return x1 * (1.0f - idx) + x2 * idx;
}
uint32_t InterpolateColor(uint32_t c1, uint32_t c2, float idx)
{
	uint32_t a1 = (c1 >> 24) & 0xFF; uint32_t a2 = (c2 >> 24) & 0xFF;
	uint32_t b1 = (c1 >> 16) & 0xFF; uint32_t b2 = (c2 >> 16) & 0xFF;
	uint32_t g1 = (c1 >> 8) & 0xFF; uint32_t g2 = (c2 >> 8) & 0xFF;
	uint32_t r1 = c1 & 0xFF; uint32_t r2 = c2 & 0xFF;
	
	uint32_t ri = InterpolateInteger(r1, r2, idx);
	uint32_t gi = InterpolateInteger(g1, g2, idx);
	uint32_t bi = InterpolateInteger(b1, b2, idx);
	uint32_t ai = InterpolateInteger(a1, a2, idx);

	return (ai << 24) | (bi << 16) | (gi << 8) | ri;
}