#pragma once
#include "game.h"

typedef enum { ACTOR_NONE, ACTOR_PICKUP, ACTOR_SAUCER, ACTOR_LADYBUG, ACTOR_HEART } ActorType;

typedef struct {
    ActorType type;
    s32 x, y;          // world, 8.8 (top-left)
    s32 vx;            // 8.8 px/frame
    s32 base_y;        // ladybug: centre of the wobble (8.8)
    int w, h;
    int param;         // pickup: colour index ; ladybug: phase
    u32 age;
} Actor;

#define MAX_ACTORS 12
extern Actor actors[MAX_ACTORS];

void actors_init(void);
void actors_update(void);        // spawner + movement + collisions with the lion
void actors_draw(int cam_x);
int  actors_count(ActorType type);
const Actor *actors_first(ActorType type);
