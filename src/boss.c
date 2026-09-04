// The giant painter (village level): rests off-world, announces itself at one edge, walks to
// the world centre, pauses, walks back, then comes from the other side. Standing on the town.
#include "boss.h"
#include "lion.h"
#include "sprites.h"
#include "audio.h"

#define OAM_BOSS0    24
#define TILE_BOSS    (512 + 40 + 8 + 4 + 28 + 4)     // after the actors' tiles
#define BOSS_Y       (TOWN_Y - BOSS_CANVAS)          // bottom of the canvas on the town top
#define BOSS_CENTER  ((TOWN_W - BOSS_CANVAS) / 2)
#define REST_FRAMES     150
#define ANNOUNCE_FRAMES 60
#define WALK_FRAMES     180
#define PAUSE_FRAMES    60
#define PEEK            20

BossState boss_state;
int boss_x, boss_side;
static int timer, x_from, x_to;

static int off_x(void) { return boss_side > 0 ? -BOSS_CANVAS : TOWN_W; }
static int peek_x(void) { return boss_side > 0 ? -BOSS_CANVAS + PEEK : TOWN_W - PEEK; }

// Pace: durations shrink to 65 % as the town gets painted.
static int scaled(int frames) {
    int p = game.progress * 1000 / game.win_permil; if (p > 1000) p = 1000;
    return frames * (1000 - 350 * p / 1000) / 1000;
}

static void enter_state(BossState s) {
    boss_state = s; timer = 0;
    switch (s) {
    case BOSS_REST:     boss_x = off_x(); break;
    case BOSS_ANNOUNCE: boss_x = peek_x(); sfx_play(sfx_boss, SFX_BOSS_LEN); break;
    case BOSS_ENTER:    x_from = peek_x(); x_to = BOSS_CENTER; break;
    case BOSS_PAUSE:    boss_x = BOSS_CENTER; break;
    case BOSS_LEAVE:    x_from = BOSS_CENTER; x_to = off_x(); break;
    default: break;
    }
}

void boss_init(int enabled) {
    if (!enabled) { boss_state = BOSS_OFF; return; }
    memcpy16(&tile_mem_obj[0][TILE_BOSS], boss_tiles, sizeof boss_tiles / 2);
    memcpy16(pal_obj_mem + 32, boss_pal, 16);
    boss_side = (rnd() & 1) ? 1 : -1;
    enter_state(BOSS_REST);
}

// Ease in-out on a 0..1 walk (t in 0..1024): smoothstep.
static int ease(int t) { return t * t * (3 * 1024 - 2 * t) / (1024 * 1024); }

void boss_update(void) {
    if (boss_state == BOSS_OFF) return;
    timer++;
    switch (boss_state) {
    case BOSS_REST:     if (timer >= scaled(REST_FRAMES)) enter_state(BOSS_ANNOUNCE); break;
    case BOSS_ANNOUNCE: if (timer >= ANNOUNCE_FRAMES) enter_state(BOSS_ENTER); break;
    case BOSS_ENTER:
    case BOSS_LEAVE: {
        int dur = scaled(WALK_FRAMES);
        int t = timer * 1024 / dur; if (t > 1024) t = 1024;
        boss_x = x_from + (x_to - x_from) * ease(t) / 1024;
        if (timer >= dur) {
            if (boss_state == BOSS_ENTER) enter_state(BOSS_PAUSE);
            else { boss_side = -boss_side; enter_state(BOSS_REST); }
        }
        break;
    }
    case BOSS_PAUSE:    if (timer >= PAUSE_FRAMES) enter_state(BOSS_LEAVE); break;
    default: break;
    }
    if (boss_state == BOSS_REST || game.over || game.invuln > 0) return;
    // Collision: silhouette box (mirrored when facing left) against the lion's 24x24 core.
    int bx0 = boss_side > 0 ? BOSS_BOX_X0 : BOSS_CANVAS - BOSS_BOX_X1;
    int bx1 = boss_side > 0 ? BOSS_BOX_X1 : BOSS_CANVAS - BOSS_BOX_X0;
    int lx = UNFIX(lion.x) + 4, ly = UNFIX(lion.y) + 4;
    if (lx < boss_x + bx1 && lx + 24 > boss_x + bx0 && ly < BOSS_Y + BOSS_BOX_Y1 && ly + 24 > BOSS_Y + BOSS_BOX_Y0)
        lion_hit(boss_x + BOSS_CANVAS / 2);
}

void boss_draw(int cam_x) {
    // Parts: {dx, dy, w, h, tile offset, shape/size attrs}
    static const struct { int dx, dy, w, tiles; u16 a0, a1; } parts[4] = {
        { 0, 0, 64, 0,   ATTR0_SQUARE, ATTR1_SIZE_64 },
        { 64, 0, 32, 64, ATTR0_TALL,   ATTR1_SIZE_64 },   // TALL + SIZE_64 = 32x64
        { 0, 64, 64, 96, ATTR0_WIDE,   ATTR1_SIZE_64 },   // WIDE + SIZE_64 = 64x32
        { 64, 64, 32, 128, ATTR0_SQUARE, ATTR1_SIZE_32 },
    };
    for (int i = 0; i < 4; i++) {
        OBJ_ATTR *o = &oam_mem[OAM_BOSS0 + i];
        if (boss_state == BOSS_OFF || boss_state == BOSS_REST) { obj_hide(o); continue; }
        int dx = boss_side > 0 ? parts[i].dx : BOSS_CANVAS - parts[i].dx - parts[i].w;
        int sx = boss_x + dx - cam_x;
        int shake = (boss_state == BOSS_ANNOUNCE && (timer & 2)) ? 1 : 0;
        if (sx <= -64 || sx >= SCREEN_WIDTH) { obj_hide(o); continue; }
        obj_set_attr(o,
            parts[i].a0 | ATTR0_4BPP | ATTR0_Y((BOSS_Y + parts[i].dy + shake) & 0xFF),
            parts[i].a1 | ATTR1_X(sx & 0x1FF) | (boss_side < 0 ? ATTR1_HFLIP : 0),
            ATTR2_PALBANK(2) | ATTR2_ID(TILE_BOSS + parts[i].tiles));
    }
}
