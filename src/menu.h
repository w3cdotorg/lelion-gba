#pragma once
#include "game.h"

void menu_draw_title(int row);
void menu_draw_intro(int timer);          // level name, READY?, VOMIT!
void menu_draw_pause(void);
void menu_draw_continue(int count);
void menu_draw_summary(int won, int can_next);
void menu_draw_arcade_end(void);
void draw_time(int x, int y, u32 frames, u8 colour, int scale);
