#include <stdlib.h>
#include <time.h>
#include <timer.h>
#include <string.h>

#include <kernel.h>
#include <graph.h>

#include "engine.h"
#include "game.h"

static volatile uint32_t vblank;

int vblank_handler()
{
	++vblank;
	return 0;
}

int main(int argc, char **argv)
{
	StartTimerSystemTime();
	srand(time(NULL));

	bool interlaced = true;
	bool ntsc = true;
	bool debug = true;

	/*for (int i = 0; i < argc; i++) {
		LOG_DEBUG("arg%u: %s", i, argv[i]);
		switch(*(uint16_t *)argv[i]) {
			case 'p-':
				interlaced = false;
				break;
			case 'P-':
				ntsc = false;
				break;
			case 'D-':
				debug = true;
				break;
		}
	}*/

	if (debug) {
		assDebugInit();
	}

	// print early welcome message
	LOG_DEBUG("Assteroids PS2 remastered director's cut version");

	assGsInit(interlaced, ntsc);
	assInputInit();

	graph_add_vsync_handler(vblank_handler);

	LOG_DEBUG("Entering main loop");

	io_t io;
	float tick = 0.0f;
	uint32_t last_vblank = 0;
	memset(&io, 0, sizeof(io));
	io.resize = true;
	while (1) {
		assUpdateIO(&io);
		if (io.exit) {
			break;
		}

		assGsBeginFrame();
		assGameUpdate(&io);

		uint32_t qwords = assGsEndFrame();

		while (1) {
			uint32_t vb = vblank;
			if (vb != last_vblank) {
				last_vblank = vb;
				break;
			}
		}

		assGsWait();
		assGsExecute();
	}

	LOG_DEBUG("Main loop exit");
	return 0;
}


