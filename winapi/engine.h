#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "sprintf.h"
#include "libvux.h"

typedef struct buffer BUFFER;

struct buffer
{
	void* data;
	unsigned capacity;
	unsigned elementCount;
	unsigned elementSize;
};


#define LOG_DEBUG(f, ...) assDebug(f, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#include "maths.h"

void assInitBuffer(BUFFER* buffer, unsigned elementSize, unsigned capacity);
void assResizeBuffer(BUFFER* buffer, unsigned elementSize, unsigned elementCount);
float assGetTime();
void assDebug(const char* format, const char* func, int line, ...);

void assGsDrawText(float x, float y, const char* str, const color_t* color, const VU_MATRIX* m);
void assGsDrawMesh(const mesh_t* mesh, const color_t* color, const VU_MATRIX* m);
void assGsClear(const color_t* color);
unsigned assGsHeight();
unsigned assGsWidth();
float assGsTvAspectRatio();

