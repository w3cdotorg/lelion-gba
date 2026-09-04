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

// Inclusive clamp (tonc's clamp() excludes the upper bound).
static inline s32 borne(s32 v, s32 lo, s32 hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Fixed point 8.8 helpers.
#define FIX_SHIFT 8
#define FIX(x)    ((x) << FIX_SHIFT)
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
} DebugState;
#define DEBUG ((volatile DebugState *)0x02030000)
