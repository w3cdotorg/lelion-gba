#pragma once
#include "game.h"

void town_init(void);
void town_render(int cam_x);                       // copy the visible window into the back page
void draw_static(u16 *page);                       // HUD band + sky gradient (once per page)
