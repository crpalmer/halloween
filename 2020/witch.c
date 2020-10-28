#include <stdio.h>
#include <stdlib.h>
#include "track.h"
#include "util.h"
#include "ween2020.h"

unsigned int range[2][2] = {
    { 4*60*1000, 6*60*1000 },
    { 30*1000, 1*60*1000 }
};

int main(int argc, char **argv)
{
    track_t *track;
    audio_device_t dev;

    seed_random();

    audio_device_init(&dev, 1, 0, true);

    if ((track = track_new_audio_dev("nibble nibble little mouse.wav", &dev)) == NULL) {
	fprintf(stderr, "failed to load audio\n");
	exit(1);
    }

    while (1) {
	unsigned int *r;

	ween2020_wait_until_valid();
	r = range[ween2020_is_trick_or_treating()];
	track_play(track);
	ms_sleep(random_number_in_range(r[0], r[1]));
    }

    return 0;
}
