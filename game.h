#pragma once
#include <draw_types.h>

#include "render.h"

typedef enum assteroid_state assteroid_state_t;
typedef struct assteroid assteroid_t;

enum assteroid_state
{
	ASS_Alive,
	ASS_Dying,
	ASS_Dead,
	ASS_Win,
	ASS_Free
};

struct assteroid
{
	assteroid_state_t m_state;
	VU_VECTOR m_position;
	VU_VECTOR m_speed;
	VU_VECTOR m_acc;
	float m_angle;
	float m_angularSpeed;
	float m_scale;
	float m_alpha;
	bool m_wrapped;
	mesh_t m_mesh;
	bool m_fragment;
};

void assRestartLevel();
void assGameUpdate(io_t *io);
void assGameDrawOsd(io_t *io);
