#pragma once
#include "game.h"

void town_init(int level);
void town_render(int cam_x);                       // copy the visible window into the back page
void draw_static(u16 *page);                       // HUD band background (once per page)
void sky_render(void);                             // sky gradient into the back page, DMA per row
void panel_render(int y0, int y1);                 // dark panel rows [y0, y1) behind menu text
