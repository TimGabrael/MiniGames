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