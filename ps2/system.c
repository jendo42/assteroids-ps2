#include <stdio.h>
#include <stdlib.h>
#include <loadfile.h>
#include <libpad.h>
#include <timer.h>
#include <math.h>

#include "system.h"

#include "render.h"
#include "game.h"

static bool g_debugEnabled = false;
static char g_padBuf[256] __attribute__((aligned(64)));

void assDebugInit()
{
	g_debugEnabled = true;
}

void assDebug(const char * format, const char *func, int line, ...)
{
	if (g_debugEnabled) {
		va_list args;
		va_start(args, line);

		char buffer[2048];
		int siz1 = stbsp_vsnprintf(buffer, sizeof(buffer), format, args) + 1;
		int siz2 = stbsp_snprintf(&buffer[siz1], sizeof(buffer) - siz1, "[%.9f] [%s:%d] %s\n", assGetTime(), func, line, buffer);

		fwrite(&buffer[siz1], siz2, 1, stdout);
		va_end(args);
	}
}

bool assLoadModule(const char *name)
{
	LOG_DEBUG("Loading '%s'", name);
	int ret = SifLoadModule(name, 0, NULL);
	if (ret < 0) {
		LOG_DEBUG("Error: '%s' failed with code %d\n", name, ret);
		return false;
	}

	return true;
}

bool assWaitPad(int port, int slot)
{
	while(1) {
		int state = padGetState(port, slot);
		switch(state) {
			case PAD_STATE_FINDPAD:
			case PAD_STATE_FINDCTP1:
			case PAD_STATE_EXECCMD:
				// wait for operation to be done
				break;
			case PAD_STATE_STABLE:
				// pad is ready
				return true;
			case PAD_STATE_DISCONN:
			case PAD_STATE_ERROR:
				// pad is not ready
				return false;
		}
	}
}

void assInputInit()
{
	// init PAD
	LOG_DEBUG("Input init");
	assLoadModule("rom0:SIO2MAN");
	assLoadModule("rom0:PADMAN");

	padInit(0);
	padPortOpen(0, 0, g_padBuf);
}

void assUpdateIO(void *iop)
{
	// update timer counter
	io_t *io = (io_t *)iop;
	float time = assGetTime();
	float deltaTime = time - io->time;

	// FPS counter, moving average per 10 frames
	float curFps = deltaTime == 0.0f ? 0.0f : 1.0f / deltaTime;
	float avgFps = io->fps + (curFps - io->fps) / 10.0f;

	// store time info
	io->deltaTime = deltaTime;
	io->time = time;
	io->fps = avgFps;

	// handle PAD reconnection
	static int lastPadState = 0;
	int padState = padGetState(0, 0);
	if (padState != lastPadState) {
		if (lastPadState == PAD_STATE_DISCONN && padState == PAD_STATE_EXECCMD) {
			if (assWaitPad(0, 0)) {
				// PAD_MMODE_LOCK does not allow to change PAD mode by user
				padSetMainMode(0, 0, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);
				assWaitPad(0, 0);
				padEnterPressMode(0, 0);
				assWaitPad(0, 0);
				LOG_DEBUG("PAD RESET!");
			}
		}
		lastPadState = padState;
	}

	// handle pad action
	struct padButtonStatus buttons;
	int ret = padRead(0, 0, &buttons);
	if (ret != 0) {
		static unsigned short lastBtns = 0;
		unsigned short btns = ~buttons.btns;
		unsigned short btnsx = lastBtns ^ btns;
		unsigned short btnsp = btns & btnsx;
		lastBtns = btns;

		io->throttle = (float)buttons.cross_p / 255;

		const float deadZone = 0.2f;
		io->dirx = (((float)buttons.ljoy_h - 127) / 127);
		io->dirx = fabsf(io->dirx) < deadZone ? 0.0f : io->dirx;

		io->diry = (((float)buttons.ljoy_v - 127) / 127);
		io->diry = fabsf(io->diry) < deadZone ? 0.0f : io->diry;

		io->fire = btns & PAD_R2;
		io->firePressed = btnsp & PAD_R2;

		io->left = btns & PAD_LEFT;
		io->right = btns & PAD_RIGHT;

		io->test = btns & PAD_R3;
		io->testPressed = btnsp & PAD_R3;

		io->exit = (btns & PAD_START) && (btns & PAD_SELECT);

		io->startPressed = btnsp & PAD_START;
	}
}

float assGetTime()
{
	return (float)GetTimerSystemTime() / kBUSCLK;
}
