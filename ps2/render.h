#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <draw_types.h>
#include <libvux.h>

typedef struct mesh mesh_t;

struct mesh
{
	uint32_t count;
	const VU_VECTOR *data;
};

void assGsInit(bool interlace, bool ntsc);

void assGsBeginFrame();
uint32_t assGsEndFrame();
void assGsExecute();
void assGsWait();
void assGsWaitVSync();

float assGsTvAspectRatio();
float assGsFbAspectRatio();
uint32_t assGsWidth();
uint32_t assGsHeight();

void assGsClear(const color_t *color);
void assGsDrawMesh(const mesh_t *mesh, const color_t *color, const VU_MATRIX *m);
void assGsDrawQuad(const xyz_t *vec, const color_t *color);
void assGsDrawText(float x, float y, const char *str, const color_t *color, const VU_MATRIX *m);
void assDrawTest();

/// @brief Converts VU_VECTOR into GS fixed point format (xyz_t)
/// @param output - destination fixed point vector
/// @param center_x - center of the screen X (width / 2)
/// @param center_y - center of the screen Y (height / 2)
/// @param input - source float vector
void assToFixed(xyz_t *output, float center_x, float center_y, float origin_x, float origin_y, VU_VECTOR *input);
