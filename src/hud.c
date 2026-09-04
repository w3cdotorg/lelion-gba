// HUD band (rows 0..15): progress bar with the win marker, unlocked colour swatches.
#include "hud.h"

#define BAR_X 60
#define BAR_Y 4
#define BAR_W 100
#define BAR_H 8

static inline void hud_hline(u8 *page, int x0, int x1, int y, u8 c) {
    for (int x = x0; x < x1; x++) {
        u16 *p = (u16 *)(page + y * SCREEN_WIDTH + (x & ~1));
        *p = (x & 1) ? ((*p & 0x00FF) | (c << 8)) : ((*p & 0xFF00) | c);
    }
}

static void hud_box(u8 *page, int x0, int y0, int w, int h, u8 c) {
    for (int y = y0; y < y0 + h; y++) hud_hline(page, x0, x0 + w, y, c);
}

IWRAM_CODE void hud_draw(void) {
    u8 *page = (u8 *)vid_page;
    // Bar: dark frame, white fill proportional to progress, win marker at the threshold.
    hud_box(page, BAR_X - 1, BAR_Y - 1, BAR_W + 2, BAR_H + 2, PAL_HUD_FG);
    hud_box(page, BAR_X, BAR_Y, BAR_W, BAR_H, PAL_HUD_BG);
    int fill = game.progress * BAR_W / 1000;
    if (fill > 0) hud_box(page, BAR_X, BAR_Y, fill, BAR_H, PAL_HUD_FG);
    int marker = BAR_X + game.win_permil * BAR_W / 1000;
    hud_box(page, marker, BAR_Y - 1, 1, BAR_H + 2, PAL_PAINT0 + 2);   // yellow
    // Hearts: filled squares on the left, hollow when lost.
    for (int i = 0; i < DIFF_LIVES[cfg.difficulty]; i++) {
        int x = 6 + i * 12;
        hud_box(page, x, BAR_Y, 8, BAR_H, PAL_PAINT0);
        if (i >= game.lives) hud_box(page, x + 1, BAR_Y + 1, 6, BAR_H - 2, PAL_HUD_BG);
    }
    // Swatches: 7 squares, painted colour when unlocked, dark when locked.
    for (int i = 0; i < NB_COLORS; i++) {
        int x = BAR_X + BAR_W + 12 + i * 8;
        hud_box(page, x, BAR_Y, 6, BAR_H, i < game.colors ? PAL_PAINT0 + i : PAL_HUD_FG);
        if (i >= game.colors) hud_box(page, x + 1, BAR_Y + 1, 4, BAR_H - 2, PAL_HUD_BG);
    }
}
