#include "Animator.h"
#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "logging.h"


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