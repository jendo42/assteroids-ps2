#include <packet.h>
#include <graph.h>
#include <draw.h>
#include <math3d.h>
#include <osd_config.h>

#include <gs_psm.h>
#include <gif_tags.h>

#include <dma.h>
#include <dma_tags.h>

#include "system.h"
#include "render.h"

#include "stb_easy_font.h"

typedef struct {
	float x;
	float y;
	float z;
	unsigned char color[4];
} stb_font_vertex_t;

const static prim_t g_lineStrip = {
	.type = PRIM_LINE_STRIP,
	.shading = PRIM_SHADE_FLAT,
	.mapping = DRAW_DISABLE,
	.fogging = DRAW_DISABLE,
	.blending = DRAW_ENABLE,
	.antialiasing = DRAW_ENABLE,
	.mapping_type = PRIM_MAP_UV,
	.colorfix = PRIM_UNFIXED
};

const static prim_t g_triangleStrip = {
	.type = PRIM_TRIANGLE_STRIP,
	.shading = PRIM_SHADE_FLAT,
	.mapping = DRAW_DISABLE,
	.fogging = DRAW_DISABLE,
	.blending = DRAW_DISABLE,
	.antialiasing = DRAW_ENABLE,
	.mapping_type = PRIM_MAP_UV,
	.colorfix = PRIM_UNFIXED
};

const static prim_t g_sprite = {
	.type = PRIM_SPRITE,
	.shading = PRIM_SHADE_FLAT,
	.mapping = DRAW_DISABLE,
	.fogging = DRAW_DISABLE,
	.blending = DRAW_ENABLE,
	.antialiasing = DRAW_ENABLE,
	.mapping_type = PRIM_MAP_UV,
	.colorfix = PRIM_UNFIXED
};

static framebuffer_t g_frame;
static zbuffer_t g_zbuffer;
static packet_t *g_gifList[2];
static uint32_t g_context;

static float g_fbAspect;
static float g_tvAspect;

static float g_center[2];
static float g_origin[2];

static qword_t *dmatag;
static qword_t *q;

void assGsInit(bool interlace, bool ntsc)
{
	// Init GIF dma channel.
	dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
	dma_channel_fast_waits(DMA_CHANNEL_GIF);

	// select vertical resolution
	g_frame.height = ntsc ? 480 : 576;
	if (!interlace) {
		g_frame.height >>= 1;
	}

	// calculate TV (output) aspect ratio
	float tvFrameAspect = configGetTvScreenType() == TV_SCREEN_169 ? (16.0f / 9.0f) : (4.0f / 3.0f);
	float tvCanvasAspect = ntsc ? (704.0f / g_frame.height) : (720.0f / g_frame.height);
	g_tvAspect = tvFrameAspect / tvCanvasAspect;

	// calculate framebuffer aspect ratio
	g_frame.width = 640;
	g_fbAspect = (float)g_frame.width / g_frame.height;

	LOG_DEBUG("Graphics init: %ux%u%c %s", g_frame.width, g_frame.height, interlace ? 'i' : 'p', ntsc ? "NTSC" : "PAL");

	// Define a 32-bit framebuffer.
	g_frame.mask = 0;
	g_frame.psm = GS_PSM_16;
	g_frame.address = graph_vram_allocate(g_frame.width, g_frame.height, g_frame.psm, GRAPH_ALIGN_PAGE);

	// setup z-buffer, make this trick for fast FB clearing
	g_zbuffer.address = g_frame.address;
	g_zbuffer.enable = DRAW_DISABLE;

	g_center[0] = g_frame.width >> 1;
	g_center[1] = g_frame.height >> 1;
	g_origin[0] = 2048.0f - g_center[0];
	g_origin[1] = 2048.0f - g_center[1];
	LOG_DEBUG("origin: %f,%f", g_origin[0], g_origin[1]);

	// Set video mode with flicker filter.
	// Initialize the screen and tie the first framebuffer to the read circuits.
	graph_set_mode(interlace ? GRAPH_MODE_INTERLACED : GRAPH_MODE_NONINTERLACED, ntsc ? GRAPH_MODE_NTSC : GRAPH_MODE_PAL, interlace ? GRAPH_MODE_FIELD : GRAPH_MODE_FRAME, GRAPH_ENABLE);
	graph_set_screen(0, 0, g_frame.width, g_frame.height);
	graph_set_bgcolor(0, 0, 0);
	graph_set_framebuffer_filtered(g_frame.address, g_frame.width, g_frame.psm, 0, 0);
	graph_enable_output();

	g_gifList[0] = packet_init(8192, PACKET_NORMAL);
	g_gifList[1] = packet_init(8192, PACKET_NORMAL);

	// This is our generic qword pointer.
	qword_t *q = g_gifList[0]->data;

	// This will setup a default drawing environment.
	// Now reset the primitive origin
	// disable zbuffer
	// Finish setting up the environment.
	q = draw_setup_environment(q, 0, &g_frame, &g_zbuffer);
	q = draw_primitive_xyoffset(q, 0, g_origin[0], g_origin[1]);
	q = draw_disable_tests(q, 0, &g_zbuffer);
	q = draw_finish(q);
	//assGsClear(&g_color_black);

	// Now send the packet, no need to wait since it's the first.
	dma_channel_send_normal(DMA_CHANNEL_GIF, g_gifList[0]->data, q - g_gifList[0]->data, 0, 0);
	dma_wait_fast();
}

void assGsBeginFrame()
{
	// prepare for generating GIF packet
	g_context = g_context ^ 1;
	packet_reset(g_gifList[g_context]);
	dmatag = g_gifList[g_context]->data;
	q = dmatag + 1;
}

uint32_t assGsEndFrame()
{
	// Setup a finish event.
	q = draw_finish(q);

	// Define our dmatag for the dma chain.
	uint32_t cnt = (q - g_gifList[g_context]->data);
	DMATAG_END(dmatag, cnt - 1, 0, 0, 0);
	return cnt;
}

void assGsExecute()
{
	// Now send our current dma chain & Wait for scene to finish drawing
	dma_channel_send_chain(DMA_CHANNEL_GIF, g_gifList[g_context]->data, q - g_gifList[g_context]->data, 0, 0);
}

void assGsWait()
{
	draw_wait_finish();
}

void assGsWaitVSync()
{
	graph_wait_vsync();
}

float assGsTvAspectRatio()
{
	return g_tvAspect;
}

float assGsFbAspectRatio()
{
	return g_fbAspect;
}

uint32_t assGsWidth()
{
	return g_frame.width;
}

uint32_t assGsHeight()
{
	return g_frame.height;
}

void assGsClear(const color_t *color)
{
	//q = draw_clear(q, 0, g_origin[0], g_origin[1], g_frame.width, g_frame.height, 0, 0, 0);
	xyz_t v0 = {
		.x = ftoi4(g_origin[0]),
		.y = ftoi4(g_origin[1]),
		.z = 0
	};

	xyz_t v1 = {
		.x = ftoi4(g_origin[0] + g_frame.width),
		.y = ftoi4(g_origin[1] + g_frame.height),
		.z = 0
	};

	q = draw_prim_start(q, 0, (prim_t *)&g_sprite, (color_t *)color);
	q->dw[0] = v0.xyz;
	q->dw[1] = v1.xyz;
	q++;
	q = draw_prim_end(q, 2, DRAW_XYZ_REGLIST);
}

void assGsDrawMesh(const mesh_t *mesh, const color_t *color, const VU_MATRIX *m)
{
	VU_VECTOR buffer[16];
	xyz_t fixed[16];
	VU_VECTOR *temp = (VU_VECTOR *)mesh->data;

	if (m) {
		temp = &buffer[0];
		for (int i = 0; i < mesh->count; i++) {
			Vu0ApplyMatrix((VU_MATRIX *)m, (VU_VECTOR *)mesh->data + i, temp + i);
		}
	}

	for (int i = 0; i < mesh->count; i++) {
		assToFixed(fixed + i, g_center[0], g_center[1], g_origin[0], g_origin[1], temp + i);
	}

	// prepare count of primitoves
	int cnt = mesh->count;
	bool isOdd = mesh->count & 1;
	if (isOdd) {
		--cnt;
	}

	q = draw_prim_start(q, 0, (prim_t *)&g_lineStrip, (color_t *)color);
	for (int i = 0; i < cnt; i += 2) {
		q->dw[0] = fixed[i].xyz;
		q->dw[1] = fixed[i+1].xyz;
		q++;
	}

	// odd count - process last primitive
	if (isOdd) {
		int i = mesh->count - 1;
		q->dw[0] = fixed[i].xyz;
		q->dw[1] = fixed[i].xyz;
		q++;
	}

	q = draw_prim_end(q, 2, DRAW_XYZ_REGLIST);
}

void assGsDrawQuad(const xyz_t *vec, const color_t *color)
{
	q = draw_prim_start(q, 0, (prim_t *)&g_triangleStrip, (color_t *)color);
	q->dw[0] = vec[0].xyz;
	q->dw[1] = vec[1].xyz;
	q++;
	q->dw[0] = vec[3].xyz;
	q->dw[1] = vec[2].xyz;
	q++;
	q = draw_prim_end(q, 2, DRAW_XYZ_REGLIST);
}

void assGsDrawText(float x, float y, const char *str, const color_t *color, const VU_MATRIX *m)
{
	// fill color with 1.0f constant to override w component
	const static float one = 1.0f;

	xyz_t fixed[4096];
	stb_font_vertex_t buffer[4096];
	VU_VECTOR *temp = (VU_VECTOR *)&buffer[0];

	int vertices = stb_easy_font_print(x, -y, (char *)str, (unsigned char *)&one, buffer, sizeof(buffer)) * 4;

	// transform
	for (int i = 0, j = 0; i < vertices; i++) {
		temp[i].y *= -1.0f;
		Vu0ApplyMatrix((VU_MATRIX *)m, temp + i, temp + i);
		assToFixed(fixed + i, g_center[0], g_center[1], g_origin[0], g_origin[1], temp + i);
		if ((i & 3) == 3) {
			assGsDrawQuad(fixed + j, color);
			j += 4;
		}
	}
}

void assToFixed(xyz_t *output, float center_x, float center_y, float origin_x, float origin_y, VU_VECTOR *input)
{
	// assume that input vector is in range [-1, 1]
	// so we move it to the [0, 2] (by adding +1) and multiply it by half-screen size so we get [0, screenSize] in fixed point
	// last step is to move the coordinates to the origin of the screen (GS settings)

	u32 max_z = -1;
	output->x = ((input->x + 1.0f) * ftoi4(center_x)) + ftoi4(origin_x);
	output->y = ((input->y + 1.0f) * ftoi4(center_y)) + ftoi4(origin_y);
	output->z = input->z * max_z;
}
