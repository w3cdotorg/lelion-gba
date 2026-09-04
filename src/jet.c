// The rainbow jet: drop sprites along an arc from the mouth, paint at the landing point.
#include "jet.h"
#include "lion.h"
#include "paint.h"

#define NB_DROPS      10
#define OAM_DROP0     1
#define TILE_DROP0    (512 + 32)          // 7 coloured 8x8 drops after the two lion frames
#define JET_DX        35                  // landing offset from the mouth (pixels)
#define JET_DY        45
#define MOUTH_DX_R    28
#define MOUTH_DX_L    4
#define MOUTH_DY      20

static int phase;

void jet_init(void) {
    // One 8x8 disc tile per rainbow colour, palette bank 1 = rainbow.
    for (int c = 0; c < NB_COLORS; c++) {
        pal_obj_mem[16 + 1 + c] = RAINBOW[c];
        u32 *tile = (u32 *)&tile_mem_obj[0][TILE_DROP0 + c];
        for (int y = 0; y < 8; y++) {
            u32 row = 0;
            for (int x = 0; x < 8; x++) {
                int dx = 2 * x - 7, dy = 2 * y - 7;
                if (dx * dx + dy * dy <= 49) row |= (u32)(1 + c) << (4 * x);
            }
            tile[y] = row;
        }
    }
    for (int i = 0; i < NB_DROPS; i++) obj_hide(&oam_mem[OAM_DROP0 + i]);
}

void jet_update(int cam_x) {
    int active = lion.puking && game.colors > 0 && !game.won;
    int mx = UNFIX(lion.x) + (lion.facing > 0 ? MOUTH_DX_R : MOUTH_DX_L);
    int my = UNFIX(lion.y) + MOUTH_DY;
    int lx = mx + lion.facing * JET_DX;
    int ly = my + JET_DY;

    if (active) {
        int radius = 3 + game.colors;                         // 4..10
        paint_stamp(lx, ly - TOWN_Y, radius);
        phase = (phase + 3) & 0xFF;
    }
    for (int i = 0; i < NB_DROPS; i++) {
        OBJ_ATTR *o = &oam_mem[OAM_DROP0 + i];
        if (!active) { obj_hide(o); continue; }
        // t in 0..1 along the arc, shifted by the phase so drops appear to travel.
        int t256 = ((i * 256) / NB_DROPS + phase) & 0xFF;
        int x = mx + (lion.facing * JET_DX * t256) / 256;
        int y = my + (JET_DY * t256 * t256) / (256 * 256);
        // colours fan out: drop i takes colour i modulo the unlocked count
        int colour = i % game.colors;
        obj_set_attr(o,
            ATTR0_SQUARE | ATTR0_4BPP | ATTR0_Y((y - 4) & 0xFF),
            ATTR1_SIZE_8 | ATTR1_X((x - cam_x - 4) & 0x1FF),
            ATTR2_PALBANK(1) | ATTR2_ID(TILE_DROP0 + colour));
    }
}
