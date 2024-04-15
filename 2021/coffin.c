#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pi-threads.h"
#include <string.h>
#include "maestro.h"
#include "pi-usb.h"
#include "track.h"
#include "time-utils.h"
#include "util.h"
#include "wb.h"
#include "ween-hours.h"

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

#define HEAD		0
#define HEAD_FORWARD	50
#define HEAD_FORWARD_SPEED 4000
#define HEAD_SIDE	70
#define HEAD_SIDE_SPEED	1000

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
    maestro_t *servo;

    seed_random();
    pi_usb_init();

    if (wb_init() < 0) {
	perror("wb_init");
	exit(1);
    }

    if ((servo = maestro_new()) == NULL) {
	fprintf(stderr, "Could not initialize maestro\n");
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

	while (! ween_hours_is_primetime()) sleep(1);

	stop_reset(stop);

	if (track) track_play_asynchronously(track, stop);

	ms_sleep(ACTION_START);

	while (! (track == NULL && it > 4) && ! stop_is_stopped(stop)) {
	    int open_time = random_number_in_range(LID_OPEN_LOW, LID_OPEN_HIGH);
	    int closed_time = random_number_in_range(LID_CLOSED_LOW, LID_CLOSED_HIGH);

	    if (it == 1) {
		maestro_set_servo_speed(servo, HEAD, HEAD_SIDE_SPEED);
		maestro_set_servo_pos(servo, HEAD, HEAD_SIDE);
	    }

	    if (ween_time_is_trick_or_treating() || ween_hours_is_ignored()) {
		wb_set(LID_PIN, 1);
		blink_lights_for(&is_lit, open_time);
		wb_set(LID_PIN, 0);
		blink_lights_for(&is_lit, closed_time);
	    } else {
		ms_sleep(open_time + closed_time);
	    }

	    it++;
	}
	
	wb_set(LIGHT_PIN, 1);
	wb_set(LID_PIN, 0);

	maestro_set_servo_speed(servo, HEAD, HEAD_FORWARD_SPEED);
	maestro_set_servo_pos(servo, HEAD, HEAD_FORWARD);
	double servo_ms = fabs(HEAD_FORWARD - HEAD_SIDE) / 100.0 * HEAD_FORWARD_SPEED;
	ms_sleep(servo_ms);

	ms_sleep(random_number_in_range(INTERDELAY_LOW, INTERDELAY_HIGH));
    }

    return 0;
}
