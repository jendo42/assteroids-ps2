#pragma once
#include <stdint.h>

typedef struct matrix VU_MATRIX;
typedef struct vector VU_VECTOR;
typedef struct mesh mesh_t;
typedef struct color color_t;

struct vector
{
	float x;
	float y;
	float z;
	float w;
};

struct color
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
	float q;
};

struct matrix
{
	float m[4][4];
};

struct mesh
{
	uint32_t count;
	const VU_VECTOR* data;
};

void Vu0ResetMatrix(VU_MATRIX* m);
void Vu0MulMatrix(VU_MATRIX* m0, VU_MATRIX* m1, VU_MATRIX* out);
void Vu0ApplyMatrix(VU_MATRIX* m, VU_VECTOR* v0, VU_VECTOR* out);
void Vu0CopyMatrix(VU_MATRIX* dest, VU_MATRIX* src);
void VuxRotMatrixZ(VU_MATRIX* m, float z);
