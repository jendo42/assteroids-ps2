#include "resources.h"

static const VU_VECTOR g_rocketData[] = {
	{ 0.0f, 1.0f, 0.0f, 1.0f },
	{ 0.5f, -1.0f, 0.0f, 1.0f },
	{ 0.0f, 0.0f, 0.0f, 1.0f },
	{ -0.5f, -1.0f, 0.0f, 1.0f },
	{ 0.0f, 1.0f, 0.0f, 1.0f }
};

static const VU_VECTOR g_squareData[] = {
	{ -0.5f, -0.5f, 0.0f, 1.0f },
	{ -0.5f, +0.5f, 0.0f, 1.0f },
	{ +0.5f, +0.5f, 0.0f, 1.0f },
	{ +0.5f, -0.5f, 0.0f, 1.0f },
	{ -0.5f, -0.5f, 0.0f, 1.0f },
};

const mesh_t g_square = {
	.data = &g_squareData[0],
	.count = 5
};

const mesh_t g_rocket = {
	.data = &g_rocketData[0],
	.count = 5
};

const mesh_t g_line = {
	.data = &g_rocketData[0],
	.count = 2
};

const color_t g_color_black = {
	.r = 0,
	.g = 0,
	.b = 0,
	.a = 255,
	.q = 1.0f
};

const color_t g_color_white = {
	.r = 255,
	.g = 255,
	.b = 255,
	.a = 255,
	.q = 1.0f
};

const color_t g_color_red = {
	.r = 255,
	.g = 0,
	.b = 0,
	.a = 255,
	.q = 1.0f
};
