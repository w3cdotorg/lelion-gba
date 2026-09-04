// The lion: movement with a little inertia, facing, puke pose. Sprite in OAM slot 0.
#include "lion.h"
#include "sprites.h"

#define LION_SPEED   FIX(3) / 2          // 1.5 px/frame  (350 px/s on a 2000 px town -> 84 px/s here)
#define LION_ACCEL   (FIX(1) / 5)
#define TILE_IDLE    512                  // bitmap modes: OBJ tiles start at 512
#define TILE_PUKE    (512 + 16)
#define LION_Y_MIN   HUD_H
#define LION_Y_MAX   (SCREEN_HEIGHT - 40)  // keeps the jet inside the screen

Lion lion;

static inline s32 vers(s32 v, s32 cible, s32 pas) {
    if (v < cible) return (v + pas > cible) ? cible : v + pas;
    if (v > cible) return (v - pas < cible) ? cible : v - pas;
    return v;
}

void lion_init(void) {
    lion.x = FIX(40);
    lion.y = FIX(60);
    lion.vx = lion.vy = 0;
    lion.facing = 1;
    lion.puking = 0;
    memcpy16(&tile_mem_obj[0][TILE_IDLE], lion_idle_tiles, sizeof lion_idle_tiles / 2);
    memcpy16(&tile_mem_obj[0][TILE_PUKE], lion_puke_tiles, sizeof lion_puke_tiles / 2);
    memcpy16(pal_obj_mem, sprites_pal, 16);
}

void lion_update(void) {
    int dx = key_tri_horz(), dy = key_tri_vert();
    lion.vx = vers(lion.vx, dx * LION_SPEED, LION_ACCEL);
    lion.vy = vers(lion.vy, dy * LION_SPEED, LION_ACCEL);
    if (dx) lion.facing = dx;
    lion.x = borne(lion.x + lion.vx, 0, FIX(TOWN_W - LION_IDLE_W));
    lion.y = borne(lion.y + lion.vy, FIX(LION_Y_MIN), FIX(LION_Y_MAX));
    lion.puking = key_is_down(KEY_A);
}

void lion_draw(int cam_x) {
    OBJ_ATTR *o = &oam_mem[0];
    int sx = UNFIX(lion.x) - cam_x, sy = UNFIX(lion.y);
    obj_set_attr(o,
        ATTR0_SQUARE | ATTR0_4BPP | ATTR0_Y(sy & 0xFF),
        ATTR1_SIZE_32 | ATTR1_X(sx & 0x1FF) | (lion.facing < 0 ? ATTR1_HFLIP : 0),
        ATTR2_PALBANK(0) | ATTR2_ID(lion.puking ? TILE_PUKE : TILE_IDLE));
}
