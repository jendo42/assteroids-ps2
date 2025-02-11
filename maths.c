#include <stdlib.h>
#include <libvux.h>
#include "maths.h"

float assRand()
{
	return (float)rand() / RAND_MAX;
}

float assRandRange(float min, float max)
{
	return assLerp(min, max, assRand());
}

void assCircularRand(VU_VECTOR *out, float radius)
{
	float a = assRandRange(0.0f, M_PI * 2.0f);
	out->x = cosf(a) * radius;
	out->y = sinf(a) * radius;
}

float assLerp(float v0, float v1, float t)
{
	return (1 - t) * v0 + t * v1;
}

float assLengthVec2(VU_VECTOR *vect)
{
	return sqrtf(vect->x * vect->x + vect->y * vect->y);
}

void assNormalizeVec2(VU_VECTOR *vect)
{
	float len = assLengthVec2(vect);
	vect->x /= len;
	vect->y /= len;
}

void assRotateVec2(VU_VECTOR *vec, float angle)
{
	float x = cosf(angle) * vec->x - sinf(angle) * vec->y;
	float y = sinf(angle) * vec->x + cosf(angle) * vec->y;
	vec->x = x;
	vec->y = y;
}

float assClamp(float x, float min, float max)
{
	return fminf(fmaxf(x, min), max);
}

void assOrthoProjRH(VU_MATRIX *output, float left, float right, float top, float bottom, float near, float far)
{
	Vu0ResetMatrix(output);

	// scale
	output->m[0][0] = 2.0f / (right - left);
	output->m[1][1] = 2.0f / (top - bottom);
	output->m[2][2] = -2.0f / (far - near);

	// translation
	output->m[3][0] = -(right + left) / (right - left);
	output->m[3][1] = -(top + bottom) / (top - bottom);
	output->m[3][2] = -(far + near) / (far - near);
}

float assTestCircle(VU_VECTOR *a, VU_VECTOR *b)
{
	VU_VECTOR diff = {
		.x = b->x - a->x,
		.y = b->y - a->y,
		.z = b->z - a->z
	};
	
	diff.x *= diff.x;
	diff.y *= diff.y;
	diff.z *= diff.z;

	float distSq = diff.x + diff.y;
	float radSumSq = a->z + b->z;
	radSumSq *= radSumSq;
	return distSq - radSumSq;
}
