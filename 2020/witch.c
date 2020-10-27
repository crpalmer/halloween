#include <stdio.h>
#include <stdlib.h>
#include "track.h"
#include "util.h"
#include "ween2020.h"

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
	ween2020_wait_until_valid();
	track_play(track);
	ms_sleep(random_number_in_range(10000, 45000));
    }

    return 0;
}
