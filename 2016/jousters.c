#include <stdio.h>
#include <stdlib.h>
#include "track.h"
#include "util.h"
#include "wb.h"

#define TRIGGER_PIN 8

#define MIDDLE_MS	3000
#define TROT_MS		2700
#define REVERSE_MS	7500

static int forward[2][2] = {{ -1, -1 }, { 6, 8 }};
static int backward[2][2] = { { -1, -1 }, {5, 7 }};

static void set(int *on, int *off, float duty)
{
    wb_pwm(WB_OUTPUT(1, off[0]), 0);
    wb_pwm(WB_OUTPUT(1, off[1]), 0);
    wb_pwm(WB_OUTPUT(1, on[0]), duty);
    wb_pwm(WB_OUTPUT(1, on[1]), duty);
}

static void
wait_for_trigger(void)
{
    unsigned yes = 0;

    while (true) {
	if (wb_get(TRIGGER_PIN)) yes++;
	else yes = 0;
	if (yes >= 10) return;
	ms_sleep(1);
    }
}

static void
wait_until_start_position(void)
{
    ms_sleep(REVERSE_MS);
}

static void
wait_until_middle(void)
{
    ms_sleep(MIDDLE_MS);
}

int
main(int argc, char **argv)
{
    stop_t *stop = stop_new();
    track_t *jousting, *crash, *beeping;

    if (wb_init() < 0) {
	fprintf(stderr, "Failed to initialize wb\n");
	exit(1);
    }

    jousting = track_new_fatal("jousters_jousting.wav");
    crash = track_new_fatal("jousters_crash.wav");
    beeping = track_new_fatal("jousters_beeping.wav");

#if 0
    set(backward[1], forward[1], 0.25);
    wait_until_start_position();
#endif
    set(backward[1], forward[1], 0);

    while (true) {
fprintf(stderr, "Waiting\n");
	wait_for_trigger();
fprintf(stderr, "Starting joust\n");

	track_play_asynchronously(jousting, stop);
	set(forward[1], backward[1], 1);
	wait_until_middle();
	stop_stop(stop);

	track_play_asynchronously(crash, stop);
	set(forward[1], backward[1], .35);
	ms_sleep(TROT_MS);
	/* gradual deceleration needed here? */

	stop_stop(stop);
	set(forward[1], backward[1], 0);
	ms_sleep(100);

	track_play_asynchronously(beeping, stop);

	set(backward[1], forward[1], 0.5);
	wait_until_start_position();
	set(forward[1], backward[1], 0);
	stop_stop(stop);
    }

    return 0;
}
