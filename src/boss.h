#pragma once
#include "game.h"

typedef enum { BOSS_OFF, BOSS_REST, BOSS_ANNOUNCE, BOSS_ENTER, BOSS_PAUSE, BOSS_LEAVE } BossState;

extern BossState boss_state;
extern int boss_x;       // world x of the 96x96 canvas (top-left), y is fixed
extern int boss_side;    // 1 = comes from the left, -1 = from the right

void boss_init(int enabled);
void boss_update(void);  // state machine + collision with the lion
void boss_draw(int cam_x);
