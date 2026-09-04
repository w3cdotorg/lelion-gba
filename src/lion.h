#pragma once
#include "game.h"

typedef struct {
    s32 x, y;          // world position, 8.8 fixed point (top-left of the 32x32 sprite)
    s32 vx, vy;        // velocity, 8.8 px/frame
    int facing;        // 1 = right, -1 = left
    int puking;
    s32 kx;            // knockback velocity, 8.8
} Lion;

extern Lion lion;

void lion_init(void);
void lion_update(void);
void lion_draw(int cam_x);
void lion_hit(int from_world_x);   // lose a life, knock back away from the enemy, start invulnerability
