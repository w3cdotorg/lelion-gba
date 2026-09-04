#pragma once
#include "game.h"

void paint_init(void);
void paint_stamp(int wx, int wy, int radius);   // speckled multicolour disc at a world position
void paint_scan_step(void);                     // measures a slice of the coverage grid each frame
int  paint_progress_permil(void);
