#include <stdio.h>
#include <stdlib.h>
#include "maestro.h"
#include "pi-usb.h"
#include "util.h"
#include "time-utils.h"
#include "track.h"

#if 0
#define INTER_LOW   5000
#define INTER_HIGH 20000
#else
#define INTER_LOW   1000
#define INTER_HIGH  2000
#endif

#define MOUTH_ID	0
#define HEAD_ID		1
#define TAIL_ID		2

typedef struct {
    unsigned speed_ms;
    double delta_pct;
} movement_t;

typedef struct {
    movement_t head, tail;
    const char *wav_fname;
} action_t;

static action_t actions[] = {
    { {  800, 25.0 }, {  640,  4.4 }, "baxter/growl.wav" },
    { { 4000, 50.0 }, { 1000,  2.8 }, "baxter/howl.wav" },
    { {  400, 35.0 }, {  400, 11.1 }, "baxter/bark.wav" },
    { {  600, 40.0 }, {  500, 27.8 }, "baxter/happy.wav" },
    { {  200, 15.0 }, {  280,  3.9 }, "baxter/whine.wav" }
};

#define N_ACTIONS (sizeof(actions) / sizeof(actions[0]))

static track_t *tracks[N_ACTIONS];
static maestro_t *m;

static void
load_tracks(void)
{
    size_t i;

    for (i = 0; i < N_ACTIONS; i++) {
	tracks[i] = track_new(actions[i].wav_fname);
	if (! tracks[i]) {
	    perror(actions[i].wav_fname);
	    exit(1);
	}
    }
}

static void
setup_servo(unsigned id, movement_t *move)
{
    maestro_set_servo_speed(m, id, move->speed_ms);
}

static void
play_track(action_t *a, stop_t *stop)
{
    int head_left = 1, tail_left = 1;
    struct timespec head_at, tail_at;

    setup_servo(HEAD_ID, &a->head);
    setup_servo(TAIL_ID, &a->tail);

    nano_gettime(&head_at);
    nano_gettime(&tail_at);

    nano_add_ms(&head_at, a->head.speed_ms/2);
    nano_add_ms(&tail_at, a->tail.speed_ms/2);

    while (! stop_is_stopped(stop)) {
	struct timespec now;

	nano_gettime(&now);
	if (nano_later_than(&now, &head_at)) {
	    head_left = !head_left;
	    maestro_set_servo_pos(m, HEAD_ID, 50 + (-1*head_left)*a->head.delta_pct);
	    printf("Move head to %s\n", head_left ? "left" : "right");
	    nano_add_ms(&head_at, a->head.speed_ms);
	}
	if (nano_later_than(&now, &tail_at)) {
	    tail_left = !tail_left;
	    printf("Move tail to %s\n", tail_left ? "left" : "right");
	    maestro_set_servo_pos(m, TAIL_ID, 50 + (-1*tail_left)*a->tail.delta_pct);
	    nano_add_ms(&tail_at, a->tail.speed_ms);
	}
    }
}

static void
reset_servos(void)
{
    maestro_set_servo_pos(m, MOUTH_ID, 50);
    maestro_set_servo_pos(m, HEAD_ID, 50);
    maestro_set_servo_pos(m, TAIL_ID, 50);
}

int
main(int argc, char **argv)
{
    stop_t *stop;
    int last_track = -1;

    pi_usb_init();

    if ((m = maestro_new()) == NULL) {
        fprintf(stderr, "Failed to initialize servo controller\n");
        exit(1);
    }

    stop = stop_new();

    maestro_set_servo_range(m, MOUTH_ID, HITEC_HS81);
    maestro_set_servo_range_pct(m, MOUTH_ID, 45, 95);
    maestro_set_servo_is_inverted(m, MOUTH_ID, true);

    maestro_set_servo_range(m, HEAD_ID, PARALLAX_STANDARD);

    load_tracks();
    while (1) {
	int track;

	ms_sleep(random_number_in_range(INTER_LOW, INTER_HIGH));

	do {
	    track = random_number_in_range(0, N_ACTIONS-1);
	} while (track == last_track);
	last_track = track;

	printf("Starting track %d\n", track);

	stop_reset(stop);
	track_play_asynchronously(tracks[track], stop);
	play_track(&actions[track], stop);
	reset_servos();
    }
}

