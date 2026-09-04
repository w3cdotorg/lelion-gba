// Shared game definitions.
#pragma once
#include <tonc.h>
#include "skyline.h"

// Screen layout (mode 4, 240x160): HUD band, sky, town band at the bottom.
#define HUD_H      16
#define TOWN_Y     (SCREEN_HEIGHT - TOWN_H)   // first screen row of the town band
#define SKY_Y      HUD_H
#define SKY_H      (TOWN_Y - SKY_Y)
#define SKY_SHADES 16

// Background palette indices.
#define PAL_HORIZON   0          // transparent in the town band -> backdrop colour
#define PAL_BUILDING  1
#define PAL_PAINT0    2          // 7 rainbow colours: 2..8
#define PAL_HUD_BG    9
#define PAL_HUD_FG    10
#define PAL_SKY0      240        // 16 sky shades: 240..255

#define NB_COLORS 7
#define RGB15C(r, g, b) ((r) | ((g) << 5) | ((b) << 10))   // constant-expression RGB15
extern const u16 RAINBOW[NB_COLORS];
#define CELL      4                       // coverage grid cell size in pixels
#define GRID_W    (TOWN_W / CELL)
#define GRID_H    (TOWN_H / CELL)
#define WIN_PERMIL 850                    // 85 % of paintable cells

// Global game state.
typedef struct {
    int colors;        // unlocked colours (0..7), rainbow order
    int progress;      // 0..1000
    int won;
    int over;          // no lives left
    int lives;
    int invuln;        // frames of invulnerability left after a hit
    u32 frame;
} Game;
#define LIVES_MAX 3
#define INVULN_FRAMES 90
extern Game game;

// Small LCG, good enough for speckles.
static inline u32 rnd(void) {
    extern u32 rnd_seed;
    rnd_seed = rnd_seed * 1664525u + 1013904223u;
    return rnd_seed >> 16;
}

// Inclusive clamp (tonc's clamp() excludes the upper bound).
static inline s32 borne(s32 v, s32 lo, s32 hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Fixed point 8.8 helpers.
#define FIX_SHIFT 8
#define FIX(x)    ((x) * (1 << FIX_SHIFT))
#define UNFIX(x)  ((x) >> FIX_SHIFT)

// Debug/inspection block at a fixed EWRAM address, read by the headless test harness.
#define DEBUG_MAGIC 0x4C494F4E  // "LION"
typedef struct {
    u32 magic;
    s32 lion_x, lion_y;   // world pixels
    s32 cam_x;
    u32 frame;
    u32 progress;         // 0..1000
    u32 colors;           // unlocked colour count
    u32 lives;
    u32 puking;
    u32 won;
    u32 n_pickups, n_enemies;   // +40, +44
    s32 pickup_x, pickup_y;     // +48, +52  first live pickup (world)
    u32 invuln;                 // +56
    s32 enemy_x, enemy_y;       // +60, +64  first live enemy (world)
    u32 over;                   // +68
    u32 music_step;             // +72
    u32 music_intensity;        // +76
    u32 sfx_frames_left;        // +80
    u32 sound_on;               // +84  REG_SNDSTAT master enable
} DebugState;
#define DEBUG ((volatile DebugState *)0x02030000)
