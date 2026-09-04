#pragma once
#include "game.h"
#include "audio_data.h"

typedef enum { THEME_TOWN, THEME_BOSS } Theme;

void audio_init(void);
void audio_update(void);                        // once per frame: sequencer + PCM bookkeeping
void music_play(Theme theme, int intensity);    // restarts the theme from the top
void music_set_intensity(int n);                // 0 = bass + drums, 1 = + arpeggios, 2 = + melody
void sfx_play(const signed char *data, int len); // one-shot on DirectSound B
void sfx_puke(int on);                          // gurgle loop on DirectSound A while puking

extern int music_step, music_intensity, sfx_frames_left;
