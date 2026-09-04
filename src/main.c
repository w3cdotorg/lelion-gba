// LeLion GBA — main loop and state machine. Mode 4 bitmap for the town, OBJ sprites for actors.
#include "game.h"
#include "town.h"
#include "lion.h"
#include "paint.h"
#include "jet.h"
#include "hud.h"
#include "actors.h"
#include "audio.h"
#include "menu.h"

Game game;
Config cfg = { 0, 0, 1 };
u32 rnd_seed = 0x1E51ABCD;
const u16 RAINBOW[NB_COLORS] = {
    RGB15C(31, 0, 0), RGB15C(31, 16, 0), RGB15C(31, 31, 0), RGB15C(0, 31, 0),
    RGB15C(0, 31, 31), RGB15C(0, 0, 31), RGB15C(29, 16, 29)
};

static State state;
static int state_timer, title_row, continue_count;

static void init_palettes(void) {
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
    pal_bg_mem[PAL_GREY] = RGB15(18, 18, 20);
}

static int camera_x(void) {
    int cx = UNFIX(lion.x) + 16 - SCREEN_WIDTH / 2;
    return borne(cx, 0, TOWN_W - SCREEN_WIDTH) & ~1;
}

static void hide_all_sprites(void) {
    for (int i = 0; i < 128; i++) obj_hide(&oam_mem[i]);
}

static void start_level(void) {
    town_init(cfg.level);
    paint_init();
    lion_init();
    jet_init();
    actors_init();
    game.colors = 0; game.progress = 0; game.won = 0; game.over = 0;
    game.hits = 0; game.frame = 0; game.time = 0;
    game.win_permil = DIFF_WIN_PERMIL[cfg.difficulty];
    music_play(THEME_TOWN, 0);
    state = ST_INTRO;
    state_timer = 0;
}

static void go_title(void) {
    town_init(cfg.level);
    hide_all_sprites();
    music_play(THEME_TOWN, 1);
    state = ST_TITLE;
    state_timer = 0;
}

static void apply_sound(void) {
    REG_SNDDMGCNT = cfg.sound ? SDMG_BUILD_LR(SDMG_SQR1 | SDMG_SQR2 | SDMG_WAVE | SDMG_NOISE, 7) : 0;
    REG_SNDDSCNT = cfg.sound
        ? (SDS_DMG100 | SDS_A100 | SDS_B100 | SDS_AR | SDS_AL | SDS_BR | SDS_BL | SDS_ATMR0 | SDS_BTMR0)
        : (SDS_ATMR0 | SDS_BTMR0);
}

static void update_title(void) {
    if (key_hit(KEY_UP)) title_row = (title_row + 2) % 3;
    if (key_hit(KEY_DOWN)) title_row = (title_row + 1) % 3;
    int d = key_hit(KEY_RIGHT) - key_hit(KEY_LEFT);
    if (d) {
        if (title_row == 0) cfg.difficulty = (cfg.difficulty + d + NB_DIFFICULTIES) % NB_DIFFICULTIES;
        else if (title_row == 1) { cfg.level = (cfg.level + d + NB_LEVELS) % NB_LEVELS; town_init(cfg.level); }
        else { cfg.sound ^= 1; apply_sound(); }
        sfx_play(sfx_pickup, SFX_PICKUP_LEN);
    }
    if (key_hit(KEY_START) || key_hit(KEY_A)) start_level();
}

static void update_play(void) {
    if (key_hit(KEY_START)) { state = ST_PAUSE; return; }
    lion_update();
    actors_update();
    int cam = camera_x();
    jet_update(cam);
    paint_scan_step();
    game.progress = paint_progress_permil();
    if (DEBUG->cheat_win) game.progress = 1000;
    if (game.progress >= game.win_permil && !game.won) {
        game.won = 1;
        sfx_play(sfx_victoire, SFX_VICTOIRE_LEN);
        sfx_puke(0);
        state = ST_SUMMARY; state_timer = 0;
        hide_all_sprites();
        return;
    }
    if (game.over) {
        sfx_puke(0);
        state = ST_CONTINUE; state_timer = 0; continue_count = 9;
        return;
    }
    music_set_intensity(game.progress * 3 / game.win_permil);
    sfx_puke(lion.puking && game.colors > 0);
    game.frame++;
    game.time++;
}

int main(void) {
    REG_WAITCNT = 0x4317;   // ROM 3/1 waitstates + prefetch: roughly twice the speed for ROM code
    REG_DISPCNT = DCNT_MODE4 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D;
    init_palettes();
    oam_init(oam_mem, 128);
    lion_init();     // loads the lion tiles
    jet_init();
    actors_init();
    audio_init();
    draw_static((u16 *)vid_mem_front);
    draw_static((u16 *)vid_mem_back);
    go_title();

    DEBUG->magic = DEBUG_MAGIC;
    u32 global_frame = 0;
    while (1) {
        key_poll();
        switch (state) {
        case ST_TITLE:
            update_title();
            break;
        case ST_INTRO:
            if (state_timer == 105) sfx_play(sfx_pret, SFX_PRET_LEN);
            if (++state_timer >= 150) state = ST_PLAY;
            break;
        case ST_PLAY:
            update_play();
            break;
        case ST_PAUSE:
            if (key_hit(KEY_START)) state = ST_PLAY;
            if (key_hit(KEY_SELECT)) go_title();
            break;
        case ST_CONTINUE:
            if (key_hit(KEY_A) || key_hit(KEY_START)) { sfx_play(sfx_pickup, SFX_PICKUP_LEN); start_level(); break; }
            if (++state_timer >= 60) { state_timer = 0; if (--continue_count < 0) { continue_count = 0; state = ST_SUMMARY; hide_all_sprites(); } }
            break;
        case ST_SUMMARY: {
            int can_next = game.won && cfg.level + 1 < NB_LEVELS;
            if (key_hit(KEY_A)) start_level();
            else if (key_hit(KEY_START)) go_title();
            else if (can_next && key_hit(KEY_R)) { cfg.level++; start_level(); }
            break;
        }
        }
        audio_update();

        // Render
        int cam = (state == ST_TITLE) ? 0 : camera_x();
        sky_render();
        town_render(cam);
        switch (state) {
        case ST_TITLE:   menu_draw_title(title_row); break;
        case ST_SUMMARY: menu_draw_summary(game.won, game.won && cfg.level + 1 < NB_LEVELS); break;
        default:
            hud_draw();
            lion_draw(cam);
            actors_draw(cam);
            if (state == ST_INTRO) menu_draw_intro(state_timer);
            else if (state == ST_PAUSE) menu_draw_pause();
            else if (state == ST_CONTINUE) menu_draw_continue(continue_count);
        }
        if (state == ST_TITLE) game.frame++;   // blink timer for PRESS START

        DEBUG->lion_x = UNFIX(lion.x);
        DEBUG->lion_y = UNFIX(lion.y);
        DEBUG->cam_x = cam;
        DEBUG->frame = ++global_frame;
        DEBUG->puking = lion.puking;
        DEBUG->progress = game.progress;
        DEBUG->colors = game.colors;
        DEBUG->won = game.won;
        DEBUG->lives = game.lives;
        DEBUG->invuln = game.invuln;
        DEBUG->over = game.over;
        DEBUG->n_pickups = actors_count(ACTOR_PICKUP);
        DEBUG->n_enemies = actors_count(ACTOR_SAUCER) + actors_count(ACTOR_LADYBUG);
        const Actor *p = actors_first(ACTOR_PICKUP);
        DEBUG->pickup_x = p ? UNFIX(p->x) : -1; DEBUG->pickup_y = p ? UNFIX(p->y) : -1;
        const Actor *e = actors_first(ACTOR_SAUCER); if (!e) e = actors_first(ACTOR_LADYBUG);
        DEBUG->enemy_x = e ? UNFIX(e->x) : -1; DEBUG->enemy_y = e ? UNFIX(e->y) : -1;
        DEBUG->music_step = music_step;
        DEBUG->music_intensity = music_intensity;
        DEBUG->sfx_frames_left = sfx_frames_left;
        DEBUG->sound_on = REG_SNDSTAT & 0x8F;
        DEBUG->state = state;
        DEBUG->continue_count = continue_count;
        DEBUG->time = game.time;
        DEBUG->difficulty = cfg.difficulty;
        DEBUG->level = cfg.level;
        DEBUG->hits = game.hits;

        vid_vsync();
        vid_flip();
    }
    return 0;
}
