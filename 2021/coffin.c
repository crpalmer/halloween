#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "track.h"
#include "time-utils.h"
#include "util.h"
#include "wb.h"

#define LIGHT_PIN	2, 1
#define LID_PIN		2, 8

#define INTERDELAY_LOW	2000
#define INTERDELAY_HIGH 2000

#define LID_OPEN_LOW	350
#define LID_OPEN_HIGH	500
#define LID_CLOSED_LOW	250
#define LID_CLOSED_HIGH	300

#define LIGHT_LOW	50
#define LIGHT_HIGH	200

#define ACTION_START	(1*10)

static void blink_lights_for(bool *is_lit, int ms)
{
    struct timespec start;
    struct timespec now;

    nano_gettime(&start);
    do {
	int light_time = random_number_in_range(LIGHT_LOW, LIGHT_HIGH);

	*is_lit = !(*is_lit);
	wb_set(LIGHT_PIN, *is_lit);
	ms_sleep(light_time);
	nano_gettime(&now);
    } while (nano_elapsed_ms(&now, &start) < ms);
}

int
main(int argc, char **argv)
{
    stop_t *stop;
    track_t *track;

    seed_random();

    if (wb_init() < 0) {
	perror("wb_init");
	exit(1);
    }

    if ((track = track_new("coffin-dog.wav")) == NULL) {
	perror("coffin-dog.wav");
    } else {
	track_set_volume(track, 50);
    }

    stop = stop_new();

    wb_set(LIGHT_PIN, 1);
    wb_set(LID_PIN, 0);

    while (true) {
	bool is_lit = true;
	int it = 0;

	stop_reset(stop);

	if (track) track_play_asynchronously(track, stop);

	ms_sleep(ACTION_START);

	while (! (track == NULL && it > 4) && ! stop_is_stopped(stop)) {
	    int open_time = random_number_in_range(LID_OPEN_LOW, LID_OPEN_HIGH);
	    int closed_time = random_number_in_range(LID_CLOSED_LOW, LID_CLOSED_HIGH);

	    wb_set(LID_PIN, 1);
	    blink_lights_for(&is_lit, open_time);
	    wb_set(LID_PIN, 0);
	    blink_lights_for(&is_lit, closed_time);
	    it++;
	}
	
	wb_set(LIGHT_PIN, 1);
	wb_set(LID_PIN, 0);

	ms_sleep(random_number_in_range(INTERDELAY_LOW, INTERDELAY_HIGH));
    }

    return 0;
}
