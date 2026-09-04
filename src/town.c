// The town: a paintable 480x72 buffer in EWRAM. 0 = sky (transparent -> horizon colour),
// PAL_BUILDING = unpainted building, PAL_PAINT0.. = painted.
#include "town.h"

EWRAM_DATA u8 town[TOWN_H][TOWN_W] __attribute__((aligned(4)));

void town_init(void) {
    for (int y = 0; y < TOWN_H; y++)
        for (int x = 0; x < TOWN_W; x++)
            town[y][x] = skyline_pixels[y * TOWN_W + x] ? PAL_BUILDING : 0;
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
    for (int y = SKY_Y; y < TOWN_Y; y++) {
        u8 shade = PAL_SKY0 + (y - SKY_Y) * SKY_SHADES / SKY_H;
        u16 v = shade | (shade << 8);
        for (int x = 0; x < SCREEN_WIDTH / 2; x++)
            page[y * (SCREEN_WIDTH / 2) + x] = v;
    }
}
