#ifndef __RANDOM_WAVS_H__
#define __RANDOM_WAVS_H__

#include "audio-player.h"
#include "util.h"
#include "wav.h"

#define MAX_WAVS 32

class RandomWavs {
public:
    void add(const char *wav_fname) {
        if (n_wavs >= MAX_WAVS) {
            consoles_fatal_printf("too many wavs: %s\n", wav_fname);
        }

        Wav *wav = new Wav(new BufferFile(wav_fname));
        wavs[n_wavs++] = wav;
    }

    void play_random(AudioPlayer *player) {
	int wav;

        do {
            wav = random_number_in_range(0, n_wavs-1);
        } while (n_wavs > 1 && wav == last_wav);
        last_wav = wav;
	player->play(wavs[wav]->to_audio_buffer());
    }

    bool is_empty() { return n_wavs == 0; }

private:
    Wav *wavs[MAX_WAVS];
    int n_wavs = 0;
    int last_wav = -1;
};

#endif
