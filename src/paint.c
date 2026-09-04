// Painting into the town buffer and coverage measurement.
#include "paint.h"
#include "town.h"

extern u8 town[TOWN_H][TOWN_W];

EWRAM_DATA static u8 cell_building[GRID_H][GRID_W];   // building pixels per cell (0..16)
EWRAM_DATA static u8 cell_painted[GRID_H][GRID_W];    // painted pixels per cell, refreshed by scans
static int paintable_cells, painted_cells, scan_row;
static const int SLICE = 2;                           // cell rows scanned per frame -> full pass in 9 frames

IWRAM_CODE void paint_init(void) {
    paintable_cells = painted_cells = 0;
    for (int cy = 0; cy < GRID_H; cy++)
        for (int cx = 0; cx < GRID_W; cx++) {
            int n = 0;
            for (int y = 0; y < CELL; y++)
                for (int x = 0; x < CELL; x++)
                    n += town[cy * CELL + y][cx * CELL + x] != 0;
            cell_building[cy][cx] = n;
            cell_painted[cy][cx] = 0;
            if (n >= CELL) paintable_cells++;       // at least a quarter of the cell is building
        }
}

#define STAMP_RADIUS_MAX 9

// No divisions in the inner loop (the ARM7 has no divide instruction): the speckle threshold per
// squared distance is tabulated once per radius, colours are picked with a multiply.
IWRAM_CODE ARM_CODE void paint_stamp(int wx, int wy, int radius) {
    static int last_radius = -1;
    static u8 thr[STAMP_RADIUS_MAX * STAMP_RADIUS_MAX + 1];   // 0..255 chance per d2
    if (game.colors <= 0) return;
    if (radius > STAMP_RADIUS_MAX) radius = STAMP_RADIUS_MAX;
    int r2 = radius * radius;
    if (radius != last_radius) {
        last_radius = radius;
        for (int d2 = 0; d2 <= r2; d2++) thr[d2] = (70 - 55 * d2 / r2) * 256 / 100;   // dense centre, sparse rim
    }
    u32 colours = game.colors;
    for (int dy = -radius; dy <= radius; dy++) {
        int y = wy + dy;
        if (y < 0 || y >= TOWN_H) continue;
        u8 *row = town[y];
        for (int dx = -radius; dx <= radius; dx++) {
            int x = wx + dx;
            int d2 = dx * dx + dy * dy;
            if (x < 0 || x >= TOWN_W || d2 > r2) continue;
            if (row[x] == 0) continue;                     // sky: nothing to paint
            if ((rnd() & 0xFF) < thr[d2])
                row[x] = PAL_PAINT0 + ((rnd() * colours) >> 16);
        }
    }
}

IWRAM_CODE ARM_CODE void paint_scan_step(void) {
    for (int s = 0; s < SLICE; s++) {
        int cy = scan_row;
        for (int cx = 0; cx < GRID_W; cx++) {
            if (cell_building[cy][cx] < CELL) continue;
            int n = 0;
            for (int y = 0; y < CELL; y++)
                for (int x = 0; x < CELL; x++)
                    n += town[cy * CELL + y][cx * CELL + x] >= PAL_PAINT0;
            int was = cell_painted[cy][cx] * 10 >= cell_building[cy][cx] * 4;
            int now = n * 10 >= cell_building[cy][cx] * 4;  // >= 40 % of the building pixels painted
            cell_painted[cy][cx] = n;
            painted_cells += now - was;
        }
        scan_row = (scan_row + 1) % GRID_H;
    }
}

int paint_progress_permil(void) {
    return paintable_cells ? painted_cells * 1000 / paintable_cells : 0;
}
