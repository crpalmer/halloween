#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "track.h"
#include "util.h"
#include "wb.h"

static void
set_lights(bool on1, bool on2)
{
    wb_set(1, 7, on1);
    wb_set(1, 8, on2);
}

static void *
sparky_main(void *unused)
{
    track_t *track;
    stop_t *stop;
    int last_state = 0;

    set_lights(false, false);

    if ((track = track_new("tesla.wav")) == NULL) {
	perror("tesla.wav");
	exit(1);
    }
    if (track) track_set_volume(track, 100);

    stop = stop_new();

    while (1) {
	stop_reset(stop);
	if (track) track_play_asynchronously(track, stop);
	while (! stop_is_stopped(stop)) {
	    int ms = random_number_in_range(25, 100);
	    int state;

	    while ((state = random_number_in_range(0, 3)) == last_state) {}
	    last_state = state;

	    set_lights(state & 1, state & 2);
	    ms_sleep(ms);
	}
    }

    return NULL;
}

static void
blink_main(void)
{
    unsigned mask = 31 | (31 << 8);	/* bits 1-5 of both banks*/

    while (1) {
	ms_sleep(random_number_in_range(200, 500));
	wb_set_outputs(mask, random_number_in_range(0, mask));
    }
}

int
main(int argc, char **argv)
{
    static pthread_t sparky_thread;

    if (wb_init() < 0) {
	fprintf(stderr, "failed to initialize wb\n");
	exit(1);
    }

    pthread_create(&sparky_thread, NULL, sparky_main, NULL);

    blink_main();

    return 0;
}
