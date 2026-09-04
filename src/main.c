// LeLion GBA — main loop. Mode 4 bitmap for the town, OBJ sprites for actors.
#include "game.h"
#include "town.h"
#include "lion.h"
#include "paint.h"
#include "jet.h"
#include "hud.h"
#include "actors.h"
#include "audio.h"

Game game;
u32 rnd_seed = 0x1E51ABCD;
const u16 RAINBOW[NB_COLORS] = {
    RGB15C(31, 0, 0), RGB15C(31, 16, 0), RGB15C(31, 31, 0), RGB15C(0, 31, 0),
    RGB15C(0, 31, 31), RGB15C(0, 0, 31), RGB15C(29, 16, 29)
};

static void init_palettes(void) {
    // Sky gradient: deep blue -> purple -> orange, same mood as the Godot version.
    for (int i = 0; i < SKY_SHADES; i++) {
        int t = i * 255 / (SKY_SHADES - 1);
        int r = (23 * (255 - t) + 230 * t) / 255;
        int g = (26 * (255 - t) + 128 * t) / 255;
        int b = (61 * (255 - t) + 82 * t) / 255;
        pal_bg_mem[PAL_SKY0 + i] = RGB15(r >> 3, g >> 3, b >> 3);
    }
    pal_bg_mem[PAL_HORIZON] = pal_bg_mem[PAL_SKY0 + SKY_SHADES - 1];
    pal_bg_mem[PAL_BUILDING] = RGB15(0, 0, 0);
    for (int i = 0; i < NB_COLORS; i++) pal_bg_mem[PAL_PAINT0 + i] = RAINBOW[i];
    pal_bg_mem[PAL_HUD_BG] = RGB15(3, 3, 6);
    pal_bg_mem[PAL_HUD_FG] = RGB15(31, 31, 31);
}

static int camera_x(void) {
    int cx = UNFIX(lion.x) + 16 - SCREEN_WIDTH / 2;
    return borne(cx, 0, TOWN_W - SCREEN_WIDTH) & ~1;
}

int main(void) {
    REG_WAITCNT = 0x4317;   // ROM 3/1 waitstates + prefetch: roughly twice the speed for ROM code
    REG_DISPCNT = DCNT_MODE4 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D;
    init_palettes();
    oam_init(oam_mem, 128);
    town_init();
    paint_init();
    lion_init();
    jet_init();
    actors_init();
    audio_init();
    music_play(THEME_TOWN, 0);
    game.colors = 0;
    draw_static((u16 *)vid_mem_front);
    draw_static((u16 *)vid_mem_back);

    DEBUG->magic = DEBUG_MAGIC;
    u32 frame = 0;
    while (1) {
        key_poll();
        game.frame = frame;
        lion_update();
        actors_update();
        int cam = camera_x();

        jet_update(cam);
        paint_scan_step();
        game.progress = paint_progress_permil();
        if (game.progress >= WIN_PERMIL && !game.won) { game.won = 1; sfx_play(sfx_victoire, SFX_VICTOIRE_LEN); }
        music_set_intensity(game.progress * 3 / WIN_PERMIL);
        sfx_puke(lion.puking && game.colors > 0 && !game.won);
        audio_update();

        town_render(cam);
        hud_draw();
        lion_draw(cam);
        actors_draw(cam);

        DEBUG->lion_x = UNFIX(lion.x);
        DEBUG->lion_y = UNFIX(lion.y);
        DEBUG->cam_x = cam;
        DEBUG->frame = ++frame;
        DEBUG->puking = lion.puking;
        DEBUG->progress = game.progress;
        DEBUG->colors = game.colors;
        DEBUG->won = game.won;
        DEBUG->lives = game.lives;
        DEBUG->invuln = game.invuln;
        DEBUG->over = game.over;
        DEBUG->music_step = music_step;
        DEBUG->music_intensity = music_intensity;
        DEBUG->sfx_frames_left = sfx_frames_left;
        DEBUG->sound_on = REG_SNDSTAT & 0x8F;   // master enable + active channel flags
        DEBUG->n_pickups = actors_count(ACTOR_PICKUP);
        DEBUG->n_enemies = actors_count(ACTOR_SAUCER) + actors_count(ACTOR_LADYBUG);
        const Actor *p = actors_first(ACTOR_PICKUP);
        DEBUG->pickup_x = p ? UNFIX(p->x) : -1; DEBUG->pickup_y = p ? UNFIX(p->y) : -1;
        const Actor *e = actors_first(ACTOR_SAUCER); if (!e) e = actors_first(ACTOR_LADYBUG);
        DEBUG->enemy_x = e ? UNFIX(e->x) : -1; DEBUG->enemy_y = e ? UNFIX(e->y) : -1;

        vid_vsync();
        vid_flip();
    }
    return 0;
}
