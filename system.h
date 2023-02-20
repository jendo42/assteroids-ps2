#pragma once
#include <stdarg.h>
#include <stdbool.h>
#include <libvux.h>
#include <draw_types.h>

#define STB_SPRINTF_NOUNALIGNED
#include "stb_sprintf.h"

#define LOG_DEBUG(f, ...) assDebug(f, __FUNCTION__, __LINE__, ##__VA_ARGS__)

typedef struct io io_t;

struct io
{
	float time;
	float deltaTime;
	float fps;

	float dirx;
	float diry;
	float throttle;

	bool left;
	bool right;
	bool fire;

	bool exit;
	bool test;

	bool startPressed;
	bool firePressed;
	bool testPressed;
};

void assInputInit();
void assDebugInit();
void assDebug(const char * format, const char *func, int line, ...);
bool assLoadModule(const char *name);
bool assWaitPad(int port, int slot);
void assUpdateIO(io_t *data);
float assGetTime();
