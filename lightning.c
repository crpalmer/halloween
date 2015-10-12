#include <stdio.h>
#include <stdlib.h>
#include "gpio.h"
#include "track.h"
#include "util.h"

static gpio_table_t gpio_table[] = { { "flash", 14, 0 } };

#define N_TRACKS	6
#define N_BOLTS		5

static track_t *tracks[N_TRACKS];

static int bolt_times[N_TRACKS][N_BOLTS] = {
    { 1100,  600, 1500,    0,  0 },
    { 1500,    0,    0,    0,  0 },
    {  800,  500, 1600,    0,  0 },
    { 2100, 2200, 2100,  600,  0 },
    { 2200,    0,    0,    0,  0 },
    { 1700, 1400,    0,    0,  0 },
};

static void
load_tracks(void)
{
    int track;

    for (track = 0; track < N_TRACKS; track++) {
	char fname[500];

	sprintf(fname, "/home/crpalmer/halloween/sound-effects/thunder/%d.wav", track);
	if ((tracks[track] = track_new(fname)) == NULL) {
	    fprintf(stderr, "failed to open %s\n", fname);
	}
    }
}

static int
do_sleep(int ms, int ms_left)
{
    int this_ms = random_number_in_range(50, ms+50);
    if (this_ms > ms_left) this_ms = ms_left;
    ms_sleep(this_ms);
    return this_ms;
}

int
main(int argc, char **argv)
{
    gpio_t *gpio = gpio_new(gpio_table, 1);
    int last_track = -1;

    if (! gpio) {
	fprintf(stderr, "failed to initialize gpio\n");
	exit(1);
    }

    load_tracks();

    while (1) {
	int track;
	int bolt;

	ms_sleep(random_number_in_range(2000, 5000));

	do {
	    track = random_number_in_range(0, N_TRACKS-1);
	} while (track == last_track);
	last_track = track;

	track_play_asynchronously(tracks[track]);

	for (bolt = 0; bolt < N_BOLTS && bolt_times[track][bolt]; bolt++) {
	    int flashes = random_number_in_range(1, 2);
	    int ms_left = bolt_times[track][bolt];

	    while (flashes-- && ms_left) {
		gpio_on_id(gpio, 0);
		ms_left -= do_sleep(200, ms_left);
		gpio_off_id(gpio, 0);
		ms_left -= do_sleep(50, ms_left);
	    }
	    if (ms_left) ms_sleep(ms_left);
	}
    }
}
