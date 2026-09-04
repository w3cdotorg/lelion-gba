// Menu and overlay screens, drawn as text into the mode 4 back page.
#include "menu.h"
#include "text.h"
#include "town.h"
#include "audio.h"

const int DIFF_LIVES[NB_DIFFICULTIES] = { 3, 3, 1 };
const int DIFF_WIN_PERMIL[NB_DIFFICULTIES] = { 850, 900, 950 };
const char *const DIFF_NAMES[NB_DIFFICULTIES] = { "EASY", "NORMAL", "HARD" };

void draw_time(int x, int y, u32 frames, u8 colour, int scale) {
    u32 s = frames / 60;
    char buf[8] = { '0' + (s / 60) % 10, ':', '0' + (s % 60) / 10, '0' + s % 10, 0 };
    text_draw_shadow(x, y, buf, colour, scale);
}

static void draw_choice(int y, const char *label, const char *value, int selected) {
    text_draw_shadow(40, y, label, selected ? PAL_HUD_FG : PAL_GREY, 1);
    char buf[24];
    int i = 0;
    buf[i++] = '<'; buf[i++] = ' ';
    for (const char *p = value; *p && i < 20; p++) buf[i++] = *p;
    buf[i++] = ' '; buf[i++] = '>'; buf[i] = 0;
    text_draw_shadow(120, y, buf, selected ? PAL_YELLOW : PAL_GREY, 1);
}

void menu_draw_title(int row) {
    text_draw_centered_shadow(18, "LELION", PAL_YELLOW, 3);
    text_draw_centered_shadow(46, "PAINT THE TOWN, PUKE A RAINBOW", PAL_HUD_FG, 1);
    draw_choice(66, "DIFFICULTY", DIFF_NAMES[cfg.difficulty], row == 0);
    draw_choice(80, "LEVEL", level_names[cfg.level], row == 1);
    draw_choice(94, "SOUND", cfg.sound ? "ON" : "OFF", row == 2);
    text_draw_centered_shadow(116, "PRESS START", (game.frame / 30) & 1 ? PAL_HUD_FG : PAL_YELLOW, 1);
    text_draw_centered_shadow(140, "A: PUKE   START: PAUSE", PAL_HUD_FG, 1);
}

void menu_draw_intro(int timer) {
    if (timer < 60) text_draw_centered_shadow(70, level_names[cfg.level], PAL_YELLOW, 2);
    else if (timer < 105) text_draw_centered_shadow(70, "READY?", PAL_YELLOW, 2);
    else text_draw_centered_shadow(66, "VOMIT!", PAL_YELLOW, 3);
}

void menu_draw_pause(void) {
    panel_render(52, 116);
    text_draw_centered_shadow(60, "PAUSED", PAL_HUD_FG, 2);
    text_draw_centered_shadow(90, "START: RESUME", PAL_HUD_FG, 1);
    text_draw_centered_shadow(102, "SELECT: QUIT TO TITLE", PAL_GREY, 1);
}

void menu_draw_continue(int count) {
    text_draw_centered_shadow(40, "CONTINUE?", PAL_RED, 2);
    char d[2] = { '0' + count, 0 };
    text_draw_centered_shadow(70, d, PAL_YELLOW, 4);
    text_draw_centered_shadow(120, "A: CONTINUE", PAL_HUD_FG, 1);
}

static void ratio(char *buf, int a, int b) {   // "a/b" for small numbers
    int i = 0;
    if (a >= 10) buf[i++] = '0' + a / 10;
    buf[i++] = '0' + a % 10; buf[i++] = '/';
    if (b >= 10) buf[i++] = '0' + b / 10;
    buf[i++] = '0' + b % 10; buf[i] = 0;
}

void menu_draw_summary(int won, int can_next) {
    panel_render(48, 128);
    text_draw_centered_shadow(22, won ? "VICTORY!" : "GAME OVER", won ? PAL_YELLOW : PAL_RED, 2);
    int y = 54;
    char buf[8];
    text_draw(48, y, "PAINTED", PAL_GREY, 1);
    text_number(150, y, (game.progress + 5) / 10, PAL_HUD_FG, 1);
    text_draw(150 + text_width("100", 1) + 2, y, "%", PAL_HUD_FG, 1);
    y += 12;
    text_draw(48, y, "TIME", PAL_GREY, 1);
    draw_time(150, y, game.time, PAL_HUD_FG, 1);
    y += 12;
    text_draw(48, y, "HEARTS LOST", PAL_GREY, 1);
    ratio(buf, game.hits, DIFF_LIVES[cfg.difficulty]);
    text_draw(150, y, buf, PAL_HUD_FG, 1);
    y += 12;
    text_draw(48, y, "COLORS", PAL_GREY, 1);
    ratio(buf, game.colors, NB_COLORS);
    text_draw(150, y, buf, PAL_HUD_FG, 1);
    if (won && game.hits == 0) text_draw_centered(y + 16, "FLAWLESS!", PAL_PAINT0 + 3, 1);
    text_draw_centered_shadow(140, can_next ? "R: NEXT   A: AGAIN   START: MENU" : "A: AGAIN   START: MENU", PAL_HUD_FG, 1);
}
