// Pickups and enemies: spawning, movement, collisions with the lion.
#include "actors.h"
#include "lion.h"
#include "sprites.h"
#include "audio.h"

#define OAM_ACTOR0    12
#define TILE_SAUCER   (512 + 40)
#define TILE_LADYBUG  (TILE_SAUCER + 8)
#define TILE_PICKUP0  (TILE_LADYBUG + 4)        // 7 coloured 16x16 discs, 4 tiles each
#define TILE_HEART    (TILE_PICKUP0 + 28)
#define HEART_EVERY   1200                       // 20 s, easy only, when a heart is missing

#define FIRST_PICKUP_FRAME  60
#define PICKUP_DELAY        360                  // 6 s after unlocking a colour
#define SAUCER_FIRST        180
#define LADYBUG_FIRST       420

Actor actors[MAX_ACTORS];
static int next_pickup_frame, next_saucer_frame, next_ladybug_frame, next_heart_frame;

static Actor *libre(void) {
    for (int i = 0; i < MAX_ACTORS; i++) if (actors[i].type == ACTOR_NONE) return &actors[i];
    return NULL;
}

// 0..1000: how far along the town is, drives enemy pace like the Godot version.
static int difficulty(void) {
    int d = game.progress * 1000 / game.win_permil;
    return d > 1000 ? 1000 : d;
}
static int interval(int start, int end) {   // frames, shrinking with difficulty, +-20 % jitter
    int base = start + (end - start) * difficulty() / 1000;
    if (cfg.level == BOSS_LEVEL) base *= 2;    // the painter is threat enough
    return base * (80 + rnd() % 41) / 100;
}

static void spawn_pickup(int colour, int wx, int wy) {
    Actor *a = libre(); if (!a) return;
    a->type = ACTOR_PICKUP; a->x = FIX(wx); a->y = FIX(wy); a->vx = 0;
    a->w = a->h = 16; a->param = colour; a->age = 0;
}
static void spawn_saucer(void) {
    Actor *a = libre(); if (!a) return;
    a->type = ACTOR_SAUCER; a->x = FIX(-32); a->y = FIX(HUD_H + 8 + rnd() % 60);
    a->vx = FIX(1) + FIX(1) * difficulty() / 1000;   // 1 .. 2 px/frame
    a->w = 32; a->h = 16; a->age = 0;
}
static void spawn_ladybug(void) {
    Actor *a = libre(); if (!a) return;
    a->type = ACTOR_LADYBUG; a->x = FIX(TOWN_W + 16); a->base_y = a->y = FIX(HUD_H + 16 + rnd() % 60);
    a->vx = -(FIX(3) / 4 + FIX(1) * difficulty() / 1500);
    a->w = a->h = 16; a->param = rnd() % 256; a->age = 0;
}

static void spawn_heart(void) {
    Actor *a = libre(); if (!a) return;
    a->type = ACTOR_HEART; a->x = FIX(40 + rnd() % (TOWN_W - 80)); a->y = FIX(HUD_H + 8 + rnd() % 60);
    a->vx = 0; a->w = a->h = 16; a->age = 0;
}

void actors_init(void) {
    for (int i = 0; i < MAX_ACTORS; i++) actors[i].type = ACTOR_NONE;
    // 16x16 heart in palette bank 1, colour 1 (red): implicit heart curve, 4 tiles.
    for (int t = 0; t < 4; t++) {
        u32 *tile = (u32 *)&tile_mem_obj[0][TILE_HEART + t];
        int ox = (t & 1) * 8, oy = (t >> 1) * 8;
        for (int y = 0; y < 8; y++) {
            u32 row = 0;
            for (int x = 0; x < 8; x++) {
                // heart: (px^2 + py^2 - 1)^3 - px^2 py^3 <= 0, scaled to 16 px, in 1/16 units
                int px = (ox + x) * 2 - 15, py = 12 - (oy + y) * 2;   // px,py in -15..15 (units of 1/6)
                long a2 = px * px + py * py - 36;                  // (px^2+py^2 - 1) in units of 36
                long lhs = a2 * a2 * a2;                          // *36^3
                long rhs = (long)px * px * py * py * py * 36;     // px^2 py^3 * 36  (scale to match)
                if (lhs - rhs <= 0) row |= 1u << (4 * x);
            }
            tile[y] = row;
        }
    }
    memcpy16(&tile_mem_obj[0][TILE_SAUCER], saucer_tiles, sizeof saucer_tiles / 2);
    memcpy16(&tile_mem_obj[0][TILE_LADYBUG], ladybug_tiles, sizeof ladybug_tiles / 2);
    // 16x16 disc per colour, palette bank 1 (rainbow), 4 tiles in 1D order.
    for (int c = 0; c < NB_COLORS; c++)
        for (int t = 0; t < 4; t++) {
            u32 *tile = (u32 *)&tile_mem_obj[0][TILE_PICKUP0 + c * 4 + t];
            int ox = (t & 1) * 8, oy = (t >> 1) * 8;
            for (int y = 0; y < 8; y++) {
                u32 row = 0;
                for (int x = 0; x < 8; x++) {
                    int dx = 2 * (ox + x) - 15, dy = 2 * (oy + y) - 15;
                    if (dx * dx + dy * dy <= 196) row |= (u32)(1 + c) << (4 * x);
                }
                tile[y] = row;
            }
        }
    for (int i = 0; i < MAX_ACTORS; i++) obj_hide(&oam_mem[OAM_ACTOR0 + i]);
    next_pickup_frame = FIRST_PICKUP_FRAME;
    next_saucer_frame = SAUCER_FIRST;
    next_ladybug_frame = LADYBUG_FIRST;
    next_heart_frame = HEART_EVERY;
}

static int rect_overlaps(int ax, int ay, int aw, int ah, int lx, int ly, int lw, int lh) {
    return ax < lx + lw && ax + aw > lx && ay < ly + lh && ay + ah > ly;
}

// The saucer's 32x16 sprite is a disc: only rows 6..11 span the full width, the dome above and
// the skirt below are about 16 px wide. Its hitbox is that cross, so the empty corners never hit.
#define SAUCER_RIM_Y   6
#define SAUCER_RIM_H   6
#define SAUCER_CORE_X  8
#define SAUCER_CORE_W  16

static int overlaps(const Actor *a, int lx, int ly, int lw, int lh) {
    int ax = UNFIX(a->x), ay = UNFIX(a->y);
    if (a->type == ACTOR_SAUCER)
        return rect_overlaps(ax, ay + SAUCER_RIM_Y, a->w, SAUCER_RIM_H, lx, ly, lw, lh)
            || rect_overlaps(ax + SAUCER_CORE_X, ay, SAUCER_CORE_W, a->h, lx, ly, lw, lh);
    return rect_overlaps(ax, ay, a->w, a->h, lx, ly, lw, lh);
}

void actors_update(void) {
    u32 f = game.frame;
    // Spawner
    if ((int)f == next_pickup_frame && game.colors < NB_COLORS) {
        if (game.colors == 0) spawn_pickup(0, 120, 60);            // first one: fixed, near the start
        else spawn_pickup(game.colors, 40 + rnd() % (TOWN_W - 80), HUD_H + 8 + rnd() % 60);
        next_pickup_frame = -1;
    }
    if ((int)f >= next_saucer_frame) { spawn_saucer(); next_saucer_frame = f + interval(360, 150); }
    if ((int)f >= next_ladybug_frame) { spawn_ladybug(); next_ladybug_frame = f + interval(480, 180); }
    if ((int)f >= next_heart_frame) {
        if (cfg.difficulty == 0 && game.lives < DIFF_LIVES[0] && actors_count(ACTOR_HEART) == 0) spawn_heart();
        next_heart_frame = f + HEART_EVERY;
    }

    // Lion hitbox: the 32x32 sprite shrunk by 4 px on each side.
    int lx = UNFIX(lion.x) + 4, ly = UNFIX(lion.y) + 4, lw = 24, lh = 24;
    for (int i = 0; i < MAX_ACTORS; i++) {
        Actor *a = &actors[i];
        if (a->type == ACTOR_NONE) continue;
        a->age++;
        switch (a->type) {
        case ACTOR_SAUCER:
            a->x += a->vx;
            if (a->x > FIX(TOWN_W + 32)) a->type = ACTOR_NONE;
            break;
        case ACTOR_LADYBUG: {
            a->x += a->vx;
            int amp = 6 + a->age / 40; if (amp > 22) amp = 22;
            a->y = a->base_y + lu_sin((a->age * 120 + a->param * 256) & 0xFFFF) * amp / 4096 * 16;
            if (a->x < FIX(-32)) a->type = ACTOR_NONE;
            break;
        }
        default: break;
        }
        if (a->type == ACTOR_NONE || game.over) continue;
        if (!overlaps(a, lx, ly, lw, lh)) continue;
        if (a->type == ACTOR_PICKUP) {
            if (a->param == game.colors) game.colors++;
            sfx_play(sfx_pickup, SFX_PICKUP_LEN);
            a->type = ACTOR_NONE;
            next_pickup_frame = f + PICKUP_DELAY;
        } else if (a->type == ACTOR_HEART) {
            if (game.lives < DIFF_LIVES[cfg.difficulty]) game.lives++;
            sfx_play(sfx_pickup, SFX_PICKUP_LEN);
            a->type = ACTOR_NONE;
        } else if (game.invuln == 0) {
            lion_hit(UNFIX(a->x) + a->w / 2);
        }
    }
}

void actors_draw(int cam_x) {
    for (int i = 0; i < MAX_ACTORS; i++) {
        OBJ_ATTR *o = &oam_mem[OAM_ACTOR0 + i];
        Actor *a = &actors[i];
        if (a->type == ACTOR_NONE) { obj_hide(o); continue; }
        int sx = UNFIX(a->x) - cam_x, sy = UNFIX(a->y);
        if (sx < -32 || sx > SCREEN_WIDTH) { obj_hide(o); continue; }
        switch (a->type) {
        case ACTOR_PICKUP:
            obj_set_attr(o, ATTR0_SQUARE | ATTR0_4BPP | ATTR0_Y(sy & 0xFF), ATTR1_SIZE_16 | ATTR1_X(sx & 0x1FF),
                         ATTR2_PALBANK(1) | ATTR2_ID(TILE_PICKUP0 + a->param * 4));
            break;
        case ACTOR_SAUCER:
            obj_set_attr(o, ATTR0_WIDE | ATTR0_4BPP | ATTR0_Y(sy & 0xFF), ATTR1_SIZE_32 | ATTR1_X(sx & 0x1FF),
                         ATTR2_PALBANK(0) | ATTR2_ID(TILE_SAUCER));
            break;
        case ACTOR_LADYBUG:
            obj_set_attr(o, ATTR0_SQUARE | ATTR0_4BPP | ATTR0_Y(sy & 0xFF), ATTR1_SIZE_16 | ATTR1_X(sx & 0x1FF),
                         ATTR2_PALBANK(0) | ATTR2_ID(TILE_LADYBUG));
            break;
        case ACTOR_HEART:
            obj_set_attr(o, ATTR0_SQUARE | ATTR0_4BPP | ATTR0_Y(sy & 0xFF), ATTR1_SIZE_16 | ATTR1_X(sx & 0x1FF),
                         ATTR2_PALBANK(1) | ATTR2_ID(TILE_HEART));
            break;
        default: obj_hide(o);
        }
    }
}

int actors_count(ActorType type) {
    int n = 0;
    for (int i = 0; i < MAX_ACTORS; i++) n += actors[i].type == type;
    return n;
}

const Actor *actors_first(ActorType type) {
    for (int i = 0; i < MAX_ACTORS; i++) if (actors[i].type == type) return &actors[i];
    return NULL;
}
