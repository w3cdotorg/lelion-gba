// Music on the Game Boy channels (square 1 = melody, square 2 = arpeggio, wave = bass,
// noise = drums) and PCM effects on the two DirectSound FIFOs.
#include "audio.h"

// Wave and noise channel bits (libtonc has no names for these).
#define SND3SEL_BANK1      0x0040   // bank to play; the other bank is the writable one
#define SND3SEL_ENABLE     0x0080
#define SND3CNT_VOL100     0x2000
#define NOISE_BUILD(r, step7, shift) ((r) | ((step7) << 3) | ((shift) << 4))

int music_step, music_intensity, sfx_frames_left;
static Theme theme;
static int tick;
static int puke_on, puke_frames_left;

static const signed char *melody, *arp, *bass;
static int frames_per_eighth;

// One period of a triangle in 32 4-bit samples (two samples per byte, high nibble first).
static const u32 triangle_wave[4] = { 0x01234567, 0x89ABCDEF, 0xFEDCBA98, 0x76543210 };

static inline u16 wave_reg(int semitone) {          // wave channel: f = 65536 / (2048 - x)
    return 2048 - ((2048 - NOTE_REG(semitone)) >> 1);
}

void audio_init(void) {
    REG_SNDSTAT = SSTAT_ENABLE;
    REG_SNDDMGCNT = SDMG_BUILD_LR(SDMG_SQR1 | SDMG_SQR2 | SDMG_WAVE | SDMG_NOISE, 7);
    REG_SNDDSCNT = SDS_DMG100 | SDS_A100 | SDS_B100 | SDS_AR | SDS_AL | SDS_BR | SDS_BL | SDS_ATMR0 | SDS_BTMR0;
    REG_SNDBIAS = 0x0200;
    REG_SND1SWEEP = SSW_OFF;
    // Wave RAM: write bank 0 while bank 1 is selected, then play bank 0.
    REG_SND3SEL = SND3SEL_BANK1;
    REG_WAVE_RAM0 = triangle_wave[0];
    REG_WAVE_RAM1 = triangle_wave[1];
    REG_WAVE_RAM2 = triangle_wave[2];
    REG_WAVE_RAM3 = triangle_wave[3];
    REG_SND3SEL = SND3SEL_ENABLE;
    REG_SND3CNT = SND3CNT_VOL100;
    // Timer 0 clocks both FIFOs at SFX_RATE.
    REG_TM0D = 65536 - 16777216 / SFX_RATE;
    REG_TM0CNT = TM_ENABLE;
    music_play(THEME_TOWN, 1);
}

void music_play(Theme t, int intensity) {
    theme = t;
    melody = t == THEME_BOSS ? boss_melody : town_melody;
    arp = t == THEME_BOSS ? boss_arp : town_arp;
    bass = t == THEME_BOSS ? boss_bass : town_bass;
    frames_per_eighth = t == THEME_BOSS ? BOSS_FRAMES_PER_EIGHTH : TOWN_FRAMES_PER_EIGHTH;
    music_step = MUSIC_STEPS - 1;
    tick = 1;
    music_intensity = intensity;
}

void music_set_intensity(int n) {
    music_intensity = n < 0 ? 0 : (n > 2 ? 2 : n);
}

static void play_step(int step) {
    int m = melody[step], a = arp[step];
    if (music_intensity >= 2 && m != NOTE_REST) {
        REG_SND1CNT = SSQR_ENV_BUILD(11, 0, 3) | SSQR_DUTY1_4;
        REG_SND1FREQ = SFREQ_RESET | NOTE_REG(m);
    }
    if (music_intensity >= 1) {
        REG_SND2CNT = SSQR_ENV_BUILD(7, 0, 1) | SSQR_DUTY1_2;
        REG_SND2FREQ = SFREQ_RESET | NOTE_REG(a);
    }
    if ((step & 1) == 0) {                           // on the beat: bass note + kick
        REG_SND3FREQ = SFREQ_RESET | wave_reg(bass[step >> 1]);
        REG_SND4CNT = SSQR_ENV_BUILD(12, 0, 1);
        REG_SND4FREQ = SFREQ_RESET | NOISE_BUILD(3, 1, 5);    // low, short burst = kick
    } else {                                         // off-beat hat
        REG_SND4CNT = SSQR_ENV_BUILD(5, 0, 1);
        REG_SND4FREQ = SFREQ_RESET | NOISE_BUILD(1, 0, 1);
    }
}

static void fifo_start(int fifo_b, const signed char *data) {
    if (fifo_b) {
        REG_DMA2CNT = 0;
        REG_SNDDSCNT |= SDS_BRESET;
        REG_DMA2SAD = (u32)data;
        REG_DMA2DAD = (u32)&REG_FIFO_B;
        REG_DMA2CNT = DMA_DST_FIXED | DMA_REPEAT | DMA_32 | DMA_AT_FIFO | DMA_ENABLE;
    } else {
        REG_DMA1CNT = 0;
        REG_SNDDSCNT |= SDS_ARESET;
        REG_DMA1SAD = (u32)data;
        REG_DMA1DAD = (u32)&REG_FIFO_A;
        REG_DMA1CNT = DMA_DST_FIXED | DMA_REPEAT | DMA_32 | DMA_AT_FIFO | DMA_ENABLE;
    }
}

void sfx_play(const signed char *data, int len) {
    fifo_start(1, data);
    sfx_frames_left = len * 60 / SFX_RATE + 1;
}

void sfx_puke(int on) {
    if (on && !puke_on) { fifo_start(0, sfx_vomi); puke_frames_left = SFX_VOMI_LEN * 60 / SFX_RATE; }
    if (!on && puke_on) REG_DMA1CNT = 0;
    puke_on = on;
}

void audio_update(void) {
    if (--tick <= 0) {
        tick = frames_per_eighth;
        music_step = (music_step + 1) % MUSIC_STEPS;
        play_step(music_step);
    }
    if (sfx_frames_left > 0 && --sfx_frames_left == 0) REG_DMA2CNT = 0;
    if (puke_on && --puke_frames_left <= 0) {         // loop the gurgle
        fifo_start(0, sfx_vomi);
        puke_frames_left = SFX_VOMI_LEN * 60 / SFX_RATE;
    }
}
