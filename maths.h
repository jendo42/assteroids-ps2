#pragma once
#include <libvux.h>
#include <draw_types.h>

#define M_E 2.71828182845904523536
#define M_LOG2E 1.44269504088896340736
#define M_LOG10E 0.434294481903251827651
#define M_LN2 0.693147180559945309417
#define M_LN10 2.30258509299404568402
#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#define M_PI_4 0.785398163397448309616
#define M_1_PI 0.318309886183790671538
#define M_2_PI 0.636619772367581343076
#define M_2_SQRTPI 1.12837916709551257390
#define M_SQRT2	1.41421356237309504880
#define M_SQRT1_2 0.707106781186547524401

float assLerp(float v0, float v1, float t);
float assRand();
float assRandRange(float min, float max);
void assCircularRand(VU_VECTOR *out, float radius);
float assLengthVec2(VU_VECTOR *vect);
void assNormalizeVec2(VU_VECTOR *vect);
void assRotateVec2(VU_VECTOR *vec, float angle);
float assClamp(float x, float min, float max);
void assOrthoProjRH(VU_MATRIX *output, float left, float right, float top, float bottom, float near, float far);

/// @brief Converts VU_VECTOR into GS fixed point format (xyz_t)
/// @param output - destination fixed point vector
/// @param center_x - center of the screen X (width / 2)
/// @param center_y - center of the screen Y (height / 2)
/// @param input - source float vector
void assToFixed(xyz_t *output, float center_x, float center_y, float origin_x, float origin_y, VU_VECTOR *input);

float assTestCircle(VU_VECTOR *a, VU_VECTOR *b);
