#pragma once
#include "game.h"

// 32 KB SRAM on the cartridge (8-bit bus). Layout, byte offsets:
//   0..3 magic "LELN", 4 version, 5 difficulty, 6 level, 7 sound, 8 arcade,
//   10..27 best[difficulty][level] seconds (u16, 0 = none), 28..29 best arcade seconds, 30 checksum.
typedef struct {
    u16 best[NB_DIFFICULTIES][NB_LEVELS];
    u16 best_arcade;
} Records;
extern Records records;

void save_load(void);     // fills cfg and records from SRAM (or defaults)
void save_store(void);    // writes cfg and records to SRAM
