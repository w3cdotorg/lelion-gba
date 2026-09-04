// Headless GBA test harness on top of libmgba.
// Usage: harness ROM SCRIPT OUTDIR
// Script commands (one per line):
//   run N            advance N frames
//   keys A,B,LEFT,RIGHT,UP,DOWN,START,SELECT,L,R   (hold exactly these keys; "keys" alone releases all)
//   shot NAME        write OUTDIR/NAME.ppm
//   peek8|peek16|peek32 ADDR   print value at ADDR (hex or decimal) as "NAME= value"? -> prints "peek ADDR value"
//   expect8|expect16|expect32 ADDR VALUE   fail if memory differs
//   expect_range32 ADDR MIN MAX
//   pixel X Y        print the pixel colour at (X, Y) as RRGGBB
// Exit code: 0 if every expectation passed, 1 otherwise.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mgba/core/core.h>
#include <mgba/core/interface.h>
#include <mgba/core/log.h>

// GBA key bit order (also what mCore::setKeys expects).
enum { GBA_KEY_A, GBA_KEY_B, GBA_KEY_SELECT, GBA_KEY_START, GBA_KEY_RIGHT, GBA_KEY_LEFT, GBA_KEY_UP, GBA_KEY_DOWN, GBA_KEY_R, GBA_KEY_L };

static void log_muet(struct mLogger *l, int cat, enum mLogLevel lvl, const char *fmt, va_list args) {
    (void)l; (void)cat; (void)lvl; (void)fmt; (void)args;
}
static struct mLogger logger = { .log = log_muet };

static int echecs = 0;
static unsigned largeur, hauteur;
static color_t *tampon;

static unsigned lire_nombre(const char *s) { return (unsigned)strtoul(s, NULL, 0); }

static unsigned touches(const char *liste) {
    unsigned masque = 0;
    char copie[256];
    strncpy(copie, liste, sizeof copie - 1);
    copie[sizeof copie - 1] = 0;
    for (char *t = strtok(copie, ", \n"); t; t = strtok(NULL, ", \n")) {
        if (!strcmp(t, "A")) masque |= 1 << GBA_KEY_A;
        else if (!strcmp(t, "B")) masque |= 1 << GBA_KEY_B;
        else if (!strcmp(t, "SELECT")) masque |= 1 << GBA_KEY_SELECT;
        else if (!strcmp(t, "START")) masque |= 1 << GBA_KEY_START;
        else if (!strcmp(t, "RIGHT")) masque |= 1 << GBA_KEY_RIGHT;
        else if (!strcmp(t, "LEFT")) masque |= 1 << GBA_KEY_LEFT;
        else if (!strcmp(t, "UP")) masque |= 1 << GBA_KEY_UP;
        else if (!strcmp(t, "DOWN")) masque |= 1 << GBA_KEY_DOWN;
        else if (!strcmp(t, "R")) masque |= 1 << GBA_KEY_R;
        else if (!strcmp(t, "L")) masque |= 1 << GBA_KEY_L;
    }
    return masque;
}

static void ecrire_ppm(const char *chemin) {
    FILE *f = fopen(chemin, "wb");
    if (!f) { perror(chemin); echecs++; return; }
    fprintf(f, "P6\n%u %u\n255\n", largeur, hauteur);
    for (unsigned i = 0; i < largeur * hauteur; i++) {
        color_t c = tampon[i];
        unsigned char rgb[3] = { c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF };  // mGBA: XBGR8888
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: harness ROM SCRIPT OUTDIR\n"); return 2; }
    mLogSetDefaultLogger(&logger);
    struct mCore *core = mCoreFind(argv[1]);
    if (!core) { fprintf(stderr, "ROM illisible : %s\n", argv[1]); return 2; }
    core->init(core);
    mCoreInitConfig(core, NULL);
    core->desiredVideoDimensions(core, &largeur, &hauteur);
    tampon = calloc(largeur * hauteur, sizeof(color_t));
    core->setVideoBuffer(core, tampon, largeur);
    if (!mCoreLoadFile(core, argv[1])) { fprintf(stderr, "chargement échoué\n"); return 2; }
    core->reset(core);

    FILE *script = fopen(argv[2], "r");
    if (!script) { perror(argv[2]); return 2; }
    char ligne[512];
    unsigned frame = 0;
    while (fgets(ligne, sizeof ligne, script)) {
        char cmd[32] = {0}, a1[256] = {0}, a2[64] = {0}, a3[64] = {0};
        if (sscanf(ligne, "%31s %255s %63s %63s", cmd, a1, a2, a3) < 1 || cmd[0] == '#') continue;
        if (!strcmp(cmd, "run")) {
            unsigned n = lire_nombre(a1);
            for (unsigned i = 0; i < n; i++) core->runFrame(core);
            frame += n;
        } else if (!strcmp(cmd, "keys")) {
            core->setKeys(core, touches(ligne + 4));
        } else if (!strcmp(cmd, "shot")) {
            char chemin[600];
            snprintf(chemin, sizeof chemin, "%s/%s.ppm", argv[3], a1);
            ecrire_ppm(chemin);
            printf("shot %s (frame %u)\n", a1, frame);
        } else if (!strncmp(cmd, "peek", 4) || !strncmp(cmd, "expect", 6)) {
            int bits = atoi(cmd + (cmd[0] == 'p' ? 4 : 6));
            int plage = !strcmp(cmd, "expect_range32");
            if (plage) bits = 32;
            unsigned addr = lire_nombre(a1);
            unsigned v = bits == 8 ? core->busRead8(core, addr) : bits == 16 ? core->busRead16(core, addr) : core->busRead32(core, addr);
            if (cmd[0] == 'p') {
                printf("peek 0x%08X = %u (0x%X)\n", addr, v, v);
            } else if (plage) {
                unsigned mn = lire_nombre(a2), mx = lire_nombre(a3);
                int ok = v >= mn && v <= mx;
                printf("%s expect 0x%08X in [%u, %u] : %u (frame %u)\n", ok ? "OK  " : "FAIL", addr, mn, mx, v, frame);
                if (!ok) echecs++;
            } else {
                unsigned attendu = lire_nombre(a2);
                int ok = v == attendu;
                printf("%s expect 0x%08X == %u : %u (frame %u)\n", ok ? "OK  " : "FAIL", addr, attendu, v, frame);
                if (!ok) echecs++;
            }
        } else if (!strncmp(cmd, "poke", 4)) {
            int bits = atoi(cmd + 4);
            unsigned addr = lire_nombre(a1), v = lire_nombre(a2);
            if (bits == 8) core->busWrite8(core, addr, v); else if (bits == 16) core->busWrite16(core, addr, v); else core->busWrite32(core, addr, v);
            printf("poke 0x%08X <- %u\n", addr, v);
        } else if (!strcmp(cmd, "pixel")) {
            unsigned x = lire_nombre(a1), y = lire_nombre(a2);
            color_t c = tampon[y * largeur + x];
            printf("pixel %u,%u = %02X%02X%02X\n", x, y, c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF);
        }
    }
    fclose(script);
    core->deinit(core);
    printf("== %d failure(s) ==\n", echecs);
    return echecs ? 1 : 0;
}
