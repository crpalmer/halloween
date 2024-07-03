#include <stdio.h>
#include <stdlib.h>
#include "pi.h"
#include "audio.h"
#include "audio-player.h"
#include "random-utils.h"
#include "wav.h"
#include "ween2020.h"

static unsigned int range[2][2] = {
    { 4*60*1000, 6*60*1000 },
    { 30*1000, 1*60*1000 }
};

static Audio *audio = Audio::create_instance();
static AudioPlayer *player = new AudioPlayer(audio);

int main(int argc, char **argv)
{
    seed_random();

    AudioBuffer *wav = wav_open("nibble nibble little mouse.wav");

    while (1) {
	unsigned int *r;

	ween2020_wait_until_valid();
	r = range[ween2020_is_trick_or_treating()];
	player->play_sync(wav);
	ms_sleep(random_number_in_range(r[0], r[1]));
    }

    return 0;
}
