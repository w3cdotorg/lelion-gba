#pragma once
#include "game.h"

// Draws text into the current back page (mode 4). Uppercase letters, digits and a few signs.
// scale 1 = 8 px cells, scale 2 = 16 px.
void text_draw(int x, int y, const char *s, u8 colour, int scale);
void text_draw_centered(int y, const char *s, u8 colour, int scale);
int  text_width(const char *s, int scale);
void text_draw_shadow(int x, int y, const char *s, u8 colour, int scale);       // dark shadow then text
void text_draw_centered_shadow(int y, const char *s, u8 colour, int scale);
void text_number(int x, int y, int value, u8 colour, int scale);
