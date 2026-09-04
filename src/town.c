// The town: a paintable 480x72 buffer in EWRAM. 0 = sky (transparent -> horizon colour),
// PAL_BUILDING = unpainted building, PAL_PAINT0.. = painted.
#include "town.h"

EWRAM_DATA u8 town[TOWN_H][TOWN_W] __attribute__((aligned(4)));

IWRAM_CODE void town_init(int level) {
    const u8 *src = skylines[level];
    for (int y = 0; y < TOWN_H; y++)
        for (int x = 0; x < TOWN_W; x++)
            town[y][x] = src[y * TOWN_W + x] ? PAL_BUILDING : 0;
}

IWRAM_CODE void town_render(int cam_x) {
    cam_x &= ~1;  // 16-bit aligned source for DMA
    u8 *page = (u8 *)vid_page;
    for (int y = 0; y < TOWN_H; y++)
        dma3_cpy(page + (TOWN_Y + y) * SCREEN_WIDTH, &town[y][cam_x], SCREEN_WIDTH);
}

void draw_static(u16 *page) {
    u16 hud2 = PAL_HUD_BG | (PAL_HUD_BG << 8);
    for (int y = 0; y < HUD_H; y++)
        for (int x = 0; x < SCREEN_WIDTH / 2; x++)
            page[y * (SCREEN_WIDTH / 2) + x] = hud2;
}

static u32 sky_rows[SKY_H];   // one 32-bit fill value (4 pixels) per sky row

void panel_render(int y0, int y1) {
    u8 *page = (u8 *)vid_page;
    for (int y = y0; y < y1; y++)
        dma3_fill(page + y * SCREEN_WIDTH, PAL_HUD_BG * 0x01010101u, SCREEN_WIDTH);
}

IWRAM_CODE void sky_render(void) {
    u8 *page = (u8 *)vid_page;
    if (sky_rows[0] == 0)
        for (int y = 0; y < SKY_H; y++) {
            u8 shade = PAL_SKY0 + y * SKY_SHADES / SKY_H;
            sky_rows[y] = shade * 0x01010101u;
        }
    for (int y = 0; y < SKY_H; y++)
        dma3_fill(page + (SKY_Y + y) * SCREEN_WIDTH, sky_rows[y], SCREEN_WIDTH);
}
