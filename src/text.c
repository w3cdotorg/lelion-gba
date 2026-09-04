// Bitmap text renderer for mode 4 pages.
#include "text.h"
#include "font.h"

static int glyph_index(char c) {
    if (c >= 'a' && c <= 'z') c -= 32;
    for (int i = 0; font_order[i]; i++) if (font_order[i] == c) return i;
    return 0;
}

static inline void plot(u8 *page, int x, int y, u8 c) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    u16 *p = (u16 *)(page + y * SCREEN_WIDTH + (x & ~1));
    *p = (x & 1) ? ((*p & 0x00FF) | (c << 8)) : ((*p & 0xFF00) | c);
}

IWRAM_CODE ARM_CODE void text_draw(int x, int y, const char *s, u8 colour, int scale) {
    u8 *page = (u8 *)vid_page;
    for (; *s; s++, x += 6 * scale) {
        const u8 *g = &font_glyphs[glyph_index(*s) * 8];
        for (int r = 0; r < 7; r++)
            for (int cbit = 0; cbit < 5; cbit++)
                if (g[r] & (0x80 >> cbit))
                    for (int sy = 0; sy < scale; sy++)
                        for (int sx = 0; sx < scale; sx++)
                            plot(page, x + cbit * scale + sx, y + r * scale + sy, colour);
    }
}

void text_draw_shadow(int x, int y, const char *s, u8 colour, int scale) {
    text_draw(x + 1, y + 1, s, PAL_HUD_BG, scale);
    text_draw(x, y, s, colour, scale);
}

void text_draw_centered_shadow(int y, const char *s, u8 colour, int scale) {
    text_draw_shadow((SCREEN_WIDTH - text_width(s, scale)) / 2, y, s, colour, scale);
}

int text_width(const char *s, int scale) {
    int n = 0;
    for (; *s; s++) n++;
    return n ? (n * 6 - 1) * scale : 0;
}

void text_draw_centered(int y, const char *s, u8 colour, int scale) {
    text_draw((SCREEN_WIDTH - text_width(s, scale)) / 2, y, s, colour, scale);
}

void text_number(int x, int y, int value, u8 colour, int scale) {
    char buf[12];
    int i = 11;
    buf[i] = 0;
    if (value < 0) value = 0;
    do { buf[--i] = '0' + value % 10; value /= 10; } while (value && i > 1);
    text_draw(x, y, &buf[i], colour, scale);
}
