#include <stdint.h>
#include <stdlib.h>
#include <libvux.h>
#include <math.h>

#include "system.h"
#include "resources.h"
#include "maths.h"

#include "game.h"

static float g_canvas[2];

static VU_MATRIX g_proj;
static VU_MATRIX g_osd;

static int g_col = 0;
static color_t g_color;

static assteroid_t *g_player;
static assteroid_t *g_bullet;
static int g_score = 0;
static int g_active = 0;
static const float g_scale = 0.5f;

// entities
static assteroid_t g_asteroids[256];
static const uint32_t g_assteroidsSize = sizeof(g_asteroids) / sizeof(assteroid_t);
static uint32_t g_lastAsteroid = 0;

static VU_VECTOR g_vertexPool[4096];
static const uint32_t g_vertexPoolSize = sizeof(g_vertexPool) / sizeof(VU_VECTOR);
static uint32_t g_lastVertex = 0;

static bool allocateVertices(mesh_t *mesh, uint32_t count)
{
	uint32_t next = g_lastVertex + count;
	if (next >= g_vertexPoolSize) {
		return false;
	}

	mesh->count = count;
	mesh->data = &g_vertexPool[g_lastVertex];
	g_lastVertex = next;
	return true;
}

static void cleanupVertices()
{
	g_lastVertex = 0;
}

static int generateAssCompare(const void *a, const void *b)
{
	VU_VECTOR *v0 = (VU_VECTOR *)a;
	VU_VECTOR *v1 = (VU_VECTOR *)b;
	float angle_a = atan2f(v0->y, v0->x);
	float angle_b = atan2f(v1->y, v1->x);
	return angle_a >= angle_b;
}

static void generateAssMesh(mesh_t* mesh)
{
	VU_VECTOR *data = (VU_VECTOR *)mesh->data;
	float angle = 0.0f;
	float step = (M_PI * 2.0f) / (mesh->count - 1);
	for (uint32_t j = 0; j < mesh->count; j++) {
		data[j].x = 1.0f;
		data[j].y = 0.0f;
		data[j].z = 0.0f;
		data[j].w = 1.0f;
		assRotateVec2(data + j, angle);
		data[j].x += assRandRange(-0.2f, 0.2f);
		data[j].y += assRandRange(-0.2f, 0.2f);
		angle += step;
	}

	// close the "circle" path
	data[mesh->count - 1] = data[0];
}

static void generateAss(assteroid_t *asteroid)
{
	// initialize state variables
	asteroid->m_fragment = false;
	asteroid->m_scale = assRandRange(25, 100);
	asteroid->m_angle = assRandRange(-M_PI, M_PI);
	asteroid->m_angularSpeed = assRandRange(-5.0f, 5.0f);
	asteroid->m_acc.x = 0;
	asteroid->m_acc.y = 0;
regenPos:
	asteroid->m_position.x = assRandRange(0, g_canvas[0]);
	asteroid->m_position.y = assRandRange(0, g_canvas[1]);

	VU_VECTOR temp = {
		.x = g_player->m_position.x - asteroid->m_position.x,
		.y = g_player->m_position.y - asteroid->m_position.y
	};
	if (assLengthVec2(&temp) < 200) {
		// assteroid is too close to the player
		goto regenPos;
	}

	asteroid->m_speed.x = assRandRange(-300, 300);
	asteroid->m_speed.y = assRandRange(-300, 300);
	asteroid->m_state = ASS_Alive;
	asteroid->m_alpha = 1.0f;

	// asteroid mesh: allocate & generate
	uint32_t vertices = assRandRange(4, 15);
	if (vertices < 4) {
		vertices = 4;
	}
	allocateVertices(&asteroid->m_mesh, vertices);
	generateAssMesh(&asteroid->m_mesh);
}

static bool wrapRect(VU_VECTOR *a)
{
	bool ret = false;
	if (a->x >= g_canvas[0]) {
		a->x = 0.0f;
		ret = true;
	}
	if (a->y >= g_canvas[1]) {
		a->y = 0.0f;
		ret = true;
	}
	if (a->x < 0.0f) {
		a->x = g_canvas[0];
		ret = true;
	}
	if (a->y < 0.0f) {
		a->y = g_canvas[1];
		ret = true;
	}
	return ret;
}

static assteroid_t *allocAsteroid()
{
	uint32_t start = g_lastAsteroid;

	do {
		assteroid_t *ass = &g_asteroids[g_lastAsteroid];
		g_lastAsteroid = (g_lastAsteroid + 1) % g_assteroidsSize;
		if (ass->m_state == ASS_Free) {
			ass->m_state = ASS_Dead;
			return ass;
		}
	} while (start != g_lastAsteroid);

	return NULL;
}

static void cleanupAsteroids()
{
	g_lastAsteroid = 0;
	for (uint32_t i = 0; i < g_assteroidsSize; i++) {
		g_asteroids[i].m_state = ASS_Free;
	}
}

void assRestartLevel()
{
	LOG_DEBUG("Restarting game");
	g_score = 0;

	float aspect = assGsTvAspectRatio();
	g_canvas[0] = (float)assGsWidth() * aspect;
	g_canvas[1] = (float)assGsHeight();
	LOG_DEBUG("Projection canvas: %f x %f (%f)", g_canvas[0], g_canvas[1], aspect);

	// setup transform
	assOrthoProjRH(&g_proj, 0, g_canvas[0], 0, g_canvas[1], 0.0f, 1.0f);
	Vu0CopyMatrix(&g_osd, &g_proj);
	g_osd.m[0][0] *= 2.0f;
	g_osd.m[1][1] *= 2.0f;

	// cleanup entities
	cleanupAsteroids();
	cleanupVertices();

	// init player
	g_player = allocAsteroid();
	g_player->m_state = ASS_Alive;
	g_player->m_position.x = g_canvas[0] * 0.5f;
	g_player->m_position.y = g_canvas[1] * 0.5f;
	g_player->m_angle = 0.0f;
	g_player->m_speed.x = 0.0f;
	g_player->m_speed.y = 0.0f;
	g_player->m_acc.z = 0.0f;
	g_player->m_acc.w = 0.0f;
	g_player->m_acc.x = 0.0f;
	g_player->m_acc.y = 0.0f;
	g_player->m_acc.z = 0.0f;
	g_player->m_acc.w = 0.0f;
	g_player->m_scale = 35.0f;
	g_player->m_mesh = g_rocket;

	g_bullet = allocAsteroid();
	g_bullet->m_speed.x = 0.0f;
	g_bullet->m_speed.y = 0.0f;
	g_bullet->m_speed.z = 0.0f;
	g_bullet->m_speed.w = 0.0f;
	g_bullet->m_acc.x = 0.0f;
	g_bullet->m_acc.y = 0.0f;
	g_bullet->m_acc.z = 0.0f;
	g_bullet->m_acc.w = 0.0f;
	g_bullet->m_angularSpeed = 0.0f;
	g_bullet->m_state = ASS_Dead;
	g_bullet->m_scale = 5.0f;
	allocateVertices(&g_bullet->m_mesh, 5);

	// initialize enemy asteroids
	for (uint32_t i = 0; i < 3; i++) {
		generateAss(allocAsteroid());
	}

	// create background asteroids
	for (uint32_t i = 0; i < 50; i++) {
		assteroid_t *ass = allocAsteroid();
		generateAss(ass);
		ass->m_state = ASS_Dead;
		ass->m_alpha = assRandRange(0.1f, 0.5f);
		ass->m_scale *= 0.1f;
	}
}

static void assTransformRenderAssteroid(assteroid_t *ass)
{
	color_t col = g_color_white;
	col.a = ass->m_alpha * 127;

	VU_MATRIX s;
	Vu0ResetMatrix(&s);
	s.m[0][0] = ass->m_scale * g_scale;
	s.m[1][1] = ass->m_scale * g_scale;

	VU_MATRIX r;
	Vu0ResetMatrix(&r);
	VuxRotMatrixZ(&r, ass->m_angle);

	VU_MATRIX t;
	Vu0ResetMatrix(&t);
	t.m[3][0] = ass->m_position.x;
	t.m[3][1] = ass->m_position.y;

	// scale x rotation x translation x projection
	Vu0MulMatrix(&s, &r, &s);
	Vu0MulMatrix(&s, &t, &s);
	Vu0MulMatrix(&s, &g_proj, &s);

	// draw mesh
	assGsDrawMesh(&ass->m_mesh, &col, &s);
}

static void spawnFragments(float scale, VU_VECTOR *position, VU_VECTOR *speed)
{
	for (int i = 0; i < 6; i++) {
		assteroid_t *frag = allocAsteroid();
		frag->m_fragment = true;
		frag->m_mesh = g_line;
		frag->m_state = ASS_Alive;
		frag->m_scale = assRandRange(10, 50) * scale;
		frag->m_angle = assRandRange(-M_PI, M_PI);
		frag->m_angularSpeed = assRandRange(-3.0f, 3.0f);
		frag->m_speed.x = assRandRange(-300, 300) + speed->x;
		frag->m_speed.y = assRandRange(-300, 300) + speed->y;
		frag->m_alpha = 1.0f;
		frag->m_position = *position;
	}
}

void assGameUpdate(io_t *io)
{
	if (io->testPressed) {
		assRestartLevel();
	}

	float timeCoef = io->deltaTime;
	float speed = assClamp(assLengthVec2(&g_player->m_speed) / 1000.0f, 0.0f, 1.0f);

	// bullet is special
	bool bulletAlive = assLengthVec2(&g_bullet->m_speed) > 0.0f;
	if (bulletAlive) {
		g_bullet->m_alpha = 1.0f;
		if (g_bullet->m_wrapped) {
			// kill the bullet outside bounds
			g_bullet->m_speed.x = 0.0f;
			g_bullet->m_speed.y = 0.0f;
		}
	} else {
		g_bullet->m_alpha = 0.0f;
	}

	const float rotationSpeed = -3.0f;
	switch (g_player->m_state) {
	case ASS_Alive:
	case ASS_Win:
		g_player->m_angularSpeed = 0.0f;
		g_player->m_angularSpeed = io->dirx * rotationSpeed;
		if (io->right) {
			g_player->m_angularSpeed = rotationSpeed;
		}
		if (io->left) {
			g_player->m_angularSpeed = -rotationSpeed;
		}
		if (io->fire && !bulletAlive) {
			// create ranom bullet shape
			generateAssMesh(&g_bullet->m_mesh);
			// make bullet move in player direction
			g_bullet->m_position = g_player->m_position;
			g_bullet->m_angle = g_player->m_angle;
			g_bullet->m_angularSpeed = 31.40f;
			g_bullet->m_speed.x = 0.0f;
			g_bullet->m_speed.y = 1000.0f;
			assRotateVec2(&g_bullet->m_speed, g_player->m_angle);
			g_bullet->m_speed.x += g_player->m_speed.x;
			g_bullet->m_speed.y += g_player->m_speed.y;
		}
		if (io->throttle > 0.0f) {
			float k = (1.0f - speed) * io->throttle;
			g_player->m_acc.x = 0.0f    * k;
			g_player->m_acc.y = 2000.0f * k;
			assRotateVec2(&g_player->m_acc, g_player->m_angle);
		} else if (speed > 0.0f) {
			float k = -0.2f * speed;
			g_player->m_acc = g_player->m_speed;
			assNormalizeVec2(&g_player->m_acc);
			g_player->m_acc.x *= k;
			g_player->m_acc.y *= k;
		}
		break;
	}

	switch (g_player->m_state) {
	case ASS_Alive:
		g_player->m_alpha = 1.0f;
		break;
	case ASS_Dying:
		g_player->m_alpha -= 10.0f * timeCoef;
		g_player->m_scale -= 100.0f * timeCoef;
		if (g_player->m_scale < 0.0f) {
			g_player->m_scale = 0.0f;
		}
		if (g_player->m_alpha <= 0.0f) {
			g_player->m_alpha = 0.0f;
			g_player->m_state = ASS_Dead;
		}
		break;
	case ASS_Dead:
	case ASS_Win:
		if (io->startPressed) {
			assRestartLevel();
		}
		break;
	}

	// process all assteroids
	uint32_t aliveAss = 0;
	uint32_t active = 0;
	for (uint32_t i = 0; i < g_assteroidsSize; i++) {
		assteroid_t *asteroid = &g_asteroids[i];
		if (asteroid->m_state == ASS_Free) {
			continue;
		}
		++active;
		if (asteroid->m_fragment) {
			assteroid_t *frag = asteroid;
			if (frag->m_state == ASS_Dead) {
				frag->m_state = ASS_Free;
				continue;
			}
			frag->m_angle += frag->m_angularSpeed * timeCoef;
			frag->m_position.x += frag->m_speed.x * timeCoef;
			frag->m_position.y += frag->m_speed.y * timeCoef;
			frag->m_alpha -= 0.25f * timeCoef;
			frag->m_scale -= 10.0f * timeCoef;
			frag->m_wrapped = wrapRect(&frag->m_position);
			if (frag->m_scale < 0.0f) {
				frag->m_scale = 0.0f;
			}
			if (frag->m_alpha <= 0.0f) {
				frag->m_state = ASS_Dead;
			} else if (frag->m_alpha <= 0.95f) {
				frag->m_state = ASS_Dying;
			}
			if (frag->m_state == ASS_Dying) {
				for (uint32_t j = 0; j < g_assteroidsSize; j++) {
					assteroid_t *ass = &g_asteroids[j];
					assteroid_state_t state = frag->m_alpha <= 0.5f ? ASS_Dead : ASS_Alive;
					VU_VECTOR f = frag->m_position;
					f.z = 2.0f * g_scale;
					VU_VECTOR a = ass->m_position;
					a.z = ass->m_scale * g_scale;
					if (ass->m_state == state && ass->m_alpha > 0.0f && assTestCircle(&f, &a) <= 0.0f) {
						frag->m_position.x -= frag->m_speed.x * timeCoef;
						frag->m_position.y -= frag->m_speed.y * timeCoef;
						frag->m_speed.x = (ass->m_speed.x - frag->m_speed.x) * 0.5f;
						frag->m_speed.y = (ass->m_speed.y - frag->m_speed.y) * 0.5f;
						frag->m_angularSpeed = -frag->m_angularSpeed * 0.9f;
					}
				}
			}

			assTransformRenderAssteroid(frag);
			continue;
		}

		asteroid->m_angle += asteroid->m_angularSpeed * timeCoef;
		asteroid->m_speed.x += asteroid->m_acc.x * timeCoef;
		asteroid->m_speed.y += asteroid->m_acc.y * timeCoef;
		asteroid->m_position.x += asteroid->m_speed.x * timeCoef;
		asteroid->m_position.y += asteroid->m_speed.y * timeCoef;
		asteroid->m_wrapped = wrapRect(&asteroid->m_position);

		if (asteroid != g_player) {
			if (g_player->m_state == ASS_Alive && asteroid->m_state == ASS_Alive) {
				VU_VECTOR bullet = g_bullet->m_position;
				bullet.z = g_bullet->m_scale * g_scale;
				VU_VECTOR player = g_player->m_position;
				player.z = g_player->m_scale * g_scale;
				VU_VECTOR ass = asteroid->m_position;
				ass.z = asteroid->m_scale * g_scale;
				// test bullet with asteroid
				if (bulletAlive && assTestCircle(&bullet, &ass) <= 0) {
					g_score += asteroid->m_scale;
					// disable bullet
					g_bullet->m_speed.x = 0.0f;
					g_bullet->m_speed.y = 0.0f;
					// destroy asteroid
					asteroid->m_state = ASS_Dying;
					asteroid->m_alpha = 1.0f;
					asteroid->m_angularSpeed *= -10.0f;
					spawnFragments(asteroid->m_scale * 0.01f, &asteroid->m_position, &asteroid->m_speed);
					// generate smaller pieces
					size_t cnt = 0;
					if (asteroid->m_scale > 75) {
						cnt = 3;
					} else if (asteroid->m_scale > 45) {
						cnt = 2;
					}
					for (uint32_t j = 0; j < cnt; j++) {
						const float k = 0.75f;
						assteroid_t *newAss = allocAsteroid();
						generateAss(newAss);
						newAss->m_position = asteroid->m_position;
						newAss->m_speed.x = (newAss->m_speed.x + asteroid->m_speed.x + g_bullet->m_speed.x) * k;
						newAss->m_speed.y = (newAss->m_speed.y + asteroid->m_speed.y + g_bullet->m_speed.y) * k;
						newAss->m_scale = asteroid->m_scale * assRandRange(0.66f, 1.0f) * 0.66f;
					}
				}
				// test player with asteroid
				if (assTestCircle(&player, &ass) <= 0.0f) {
					// colision with rocket, die
					g_player->m_state = ASS_Dying;
					g_player->m_angularSpeed -= asteroid->m_angularSpeed;

					VU_VECTOR speed;
					float k = 0.33f;
					speed.x = asteroid->m_speed.x - g_player->m_speed.x * k;
					speed.y = asteroid->m_speed.y - g_player->m_speed.y * k;

					spawnFragments(1.0f, &g_player->m_position, &speed);
				}
			}

			switch (asteroid->m_state) {
				case ASS_Alive:
					++aliveAss;
					break;
				case ASS_Dying:
					asteroid->m_alpha -= 6.6f * timeCoef;
					asteroid->m_scale -= 200.0f * timeCoef;
					if (asteroid->m_scale <= 0.0f) {
						asteroid->m_scale = 0.0f;
						asteroid->m_state = ASS_Free;
					}
					if (asteroid->m_alpha <= 0.0f) {
						asteroid->m_alpha = 0.0f;
					}
					break;
			}
		}

		assTransformRenderAssteroid(asteroid);
	}

	g_active = active;
	if (g_player->m_state == ASS_Alive && !aliveAss) {
		g_player->m_state = ASS_Win;
	}

	static float x;
	x += io->deltaTime;
	if (x >= 0.5f) {
		x = 0.0f;
		LOG_DEBUG("speed: %f; acc: %f; pos: %f,%f; angle: %f", assLengthVec2(&g_player->m_speed), assLengthVec2(&g_player->m_acc), g_player->m_position.x, g_player->m_position.y, g_player->m_angle);
	}
}

void assGameDrawOsd(io_t *io)
{
	char buff[2048];
	float vpool = (float)g_lastVertex / g_vertexPoolSize * 100.0f;
	float epool = (float)g_active / g_assteroidsSize * 100.0f;
	int siz = stbsp_snprintf(buff, sizeof(buff), "FPS: %.2f  VPOOL: %.1f%%  EPOOL: %.1f%%", io->fps, vpool, epool) + 1;
	int siz2 = stbsp_snprintf(&buff[siz], sizeof(buff) - siz, "SCORE: %d", g_score);
	VU_MATRIX osd = g_osd;
	osd.m[0][0] *= 4.0f;
	osd.m[1][1] *= 4.0f;
	assGsDrawText(10.0f, g_canvas[1] * 0.5f - 10.0f, buff, &g_color_white, &g_osd);
	assGsDrawText(10.0f, 30.0f, &buff[siz], &g_color_white, &g_osd);
	switch (g_player->m_state) {
		case ASS_Dead:
			assGsDrawText(12.5, 35, "You DIED", &g_color_red, &osd);
			break;
		case ASS_Win:
			assGsDrawText(15, 35, "You WIN", &g_color_white, &osd);
			break;
	}
}
