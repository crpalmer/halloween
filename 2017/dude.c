#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "track.h"
#include "util.h"
#include "wb.h"

#define MOVE_PAUSE_MIN	350
#define MOVE_PAUSE_MAX	650

static void
set_state(bool on1, bool on2)
{
    wb_set(1, 1, on1);
    wb_set(1, 2, on2);
printf("%d %d\n", on1, on2); fflush(stdout);
}

int
main(int argc, char **argv)
{
    track_t *track;
    stop_t *stop;
    int last_state = -1, last_state2 = -1;

    if (wb_init() < 0) {
	fprintf(stderr, "failed to initialize wb\n");
	exit(1);
    }

    set_state(false, false);

    if ((track = track_new("dude.wav")) == NULL) {
	perror("dude.wav");
	exit(1);
    }
    if (track) track_set_volume(track, 100);

    stop = stop_new();

    while (1) {
	stop_reset(stop);
	if (track) track_play_asynchronously(track, stop);
	while (! stop_is_stopped(stop)) {
	    int ms = random_number_in_range(MOVE_PAUSE_MIN, MOVE_PAUSE_MAX);
	    int state;

	    /* Pick a new state and allow 1, but not 2, repetitions of the same state */
	    while ((state = random_number_in_range(0, 3)) == last_state && last_state == last_state2) {}
	    last_state2 = last_state;
	    last_state = state;

	    set_state(state & 1, state & 2);
	    ms_sleep(ms);
	}
    }
}
