// SRAM persistence. The "SRAM_V" marker in ROM tells emulators and flash carts the save type.
#include "save.h"

const char save_type_marker[] __attribute__((used, aligned(4))) = "SRAM_V113";

#define SRAM ((volatile u8 *)0x0E000000)
#define SAVE_VERSION 1
#define SAVE_SIZE 31

Records records;

static u8 checksum(const u8 *b) {
    u8 s = 0;
    for (int i = 4; i < SAVE_SIZE - 1; i++) s += b[i];
    return s;
}

void save_load(void) {
    u8 b[SAVE_SIZE];
    for (int i = 0; i < SAVE_SIZE; i++) b[i] = SRAM[i];
    for (int i = 0; i < NB_DIFFICULTIES * NB_LEVELS; i++) ((u16 *)records.best)[i] = 0;
    records.best_arcade = 0;
    if (b[0] != 'L' || b[1] != 'E' || b[2] != 'L' || b[3] != 'N' || b[4] != SAVE_VERSION || checksum(b) != b[SAVE_SIZE - 1])
        return;
    cfg.difficulty = b[5] < NB_DIFFICULTIES ? b[5] : 0;
    cfg.level = b[6] < NB_LEVELS ? b[6] : 0;
    cfg.sound = b[7] & 1;
    cfg.arcade = b[8] & 1;
    for (int i = 0; i < NB_DIFFICULTIES * NB_LEVELS; i++)
        ((u16 *)records.best)[i] = b[10 + 2 * i] | (b[11 + 2 * i] << 8);
    records.best_arcade = b[28] | (b[29] << 8);
}

void save_store(void) {
    u8 b[SAVE_SIZE] = { 'L', 'E', 'L', 'N', SAVE_VERSION, (u8)cfg.difficulty, (u8)cfg.level, (u8)cfg.sound, (u8)cfg.arcade, 0 };
    for (int i = 0; i < NB_DIFFICULTIES * NB_LEVELS; i++) {
        u16 v = ((u16 *)records.best)[i];
        b[10 + 2 * i] = v & 0xFF; b[11 + 2 * i] = v >> 8;
    }
    b[28] = records.best_arcade & 0xFF; b[29] = records.best_arcade >> 8;
    b[SAVE_SIZE - 1] = checksum(b);
    for (int i = 0; i < SAVE_SIZE; i++) SRAM[i] = b[i];
}
