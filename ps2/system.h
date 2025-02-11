#pragma once
#include <stdarg.h>
#include <stdbool.h>

#include "sprintf.h"

#define LOG_DEBUG(f, ...) assDebug(f, __FUNCTION__, __LINE__, ##__VA_ARGS__)

void assInputInit();
void assDebugInit();
void assDebug(const char * format, const char *func, int line, ...);
bool assLoadModule(const char *name);
bool assWaitPad(int port, int slot);
void assUpdateIO(void *data);
float assGetTime();
