// LeLion GBA — entry point. Phase 0: mode 4 bitmap, palette, a moving square.
#include <tonc.h>

#define PAL_SKY   1
#define PAL_TOWN  2
#define PAL_LION  3

static void fill_sky(void) {
    // Mode 4: 8-bit paletted pixels, written two at a time (VRAM is 16-bit only).
    u16 *page = (u16 *)vid_page;
    u16 sky2 = PAL_SKY | (PAL_SKY << 8);
    u16 town2 = PAL_TOWN | (PAL_TOWN << 8);
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        u16 v = (y >= SCREEN_HEIGHT - 40) ? town2 : sky2;
        for (int x = 0; x < SCREEN_WIDTH / 2; x++)
            page[y * (SCREEN_WIDTH / 2) + x] = v;
    }
}

int main(void) {
    REG_DISPCNT = DCNT_MODE4 | DCNT_BG2;
    pal_bg_mem[PAL_SKY] = RGB15(6, 4, 14);
    pal_bg_mem[PAL_TOWN] = RGB15(0, 0, 0);
    pal_bg_mem[PAL_LION] = RGB15(31, 24, 4);

    int x = 100, y = 60;
    while (1) {
        vid_vsync();
        key_poll();
        x += key_tri_horz() * 2;
        y += key_tri_vert() * 2;
        fill_sky();
        m4_rect(x, y, x + 16, y + 16, PAL_LION);
        vid_flip();
    }
    return 0;
}
