#ifndef __RANDOM_AUDIO_H__
#define __RANDOM_AUDIO_H__

#include "audio-player.h"
#include "consoles.h"
#include "random-utils.h"
#include "wav.h"

#define MAX_RANDOM_AUDIO 32

class RandomAudio {
public:
    bool add(const char *wav_fname) {
        if (n_audio >= MAX_RANDOM_AUDIO) {
            consoles_printf("too many audio: %s\n", wav_fname);
	    return false;
        }

	if ((audios[n_audio] = wav_open(wav_fname)) == NULL) {
	    return false;
	}
	n_audio++;
	return true;
    }

    void play_random(AudioPlayer *player) {
	int pick;

        do {
            pick = random_number_in_range(0, n_audio-1);
        } while (n_audio > 1 && pick == last_audio);
        last_audio = pick;
	player->play(audios[pick]);
    }

    bool is_empty() { return n_audio == 0; }

    void dump() {
	consoles_printf("random-audio: %d tracks, last played %d\n", n_audio, last_audio);
	for (int i = 0; i < n_audio; i++) {
	    consoles_printf("  %3d : %s\n", i, audios[i]->get_fname());
	}
    }

private:
    AudioBuffer *audios[MAX_RANDOM_AUDIO];
    int n_audio = 0;
    int last_audio = -1;
};

#endif
