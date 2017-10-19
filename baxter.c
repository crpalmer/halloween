#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "maestro.h"
#include "pi-usb.h"
#include "util.h"
#include "talking-skull.h"
#include "time-utils.h"
#include "track.h"
#include "wb.h"

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

#define LEFT_EYE	2,5
#define RIGHT_EYE	2,6

typedef struct {
    unsigned speed_ms;
    double delta_pct;
} movement_t;

typedef struct {
    movement_t head, tail;
    const char *wav_fname;
} action_t;

static action_t actions[] = {
    { {  800, 25.0 }, {  640,  4.4 }, "baxter/growl" },
    { { 4000, 50.0 }, { 1000,  2.8 }, "baxter/howl" },
    { {  400, 35.0 }, {  400, 11.1 }, "baxter/bark" },
    { {  600, 40.0 }, {  500, 27.8 }, "baxter/happy" },
    { {  200, 15.0 }, {  280,  3.9 }, "baxter/whine" }
};

#define N_ACTIONS (sizeof(actions) / sizeof(actions[0]))

static track_t *tracks[N_ACTIONS];
static talking_skull_actor_t *actors[N_ACTIONS];
static maestro_t *m;

static void
update_mouth(void *unused, double pos)
{
    static int eyes = -1;
    int new_eyes = pos >= 50;

    maestro_set_servo_pos(m, MOUTH_ID, pos);
    if (eyes != new_eyes) {
        wb_set(LEFT_EYE, new_eyes);
        wb_set(RIGHT_EYE, new_eyes);
        eyes = new_eyes;
    }
}

static void
load_tracks(void)
{
    size_t i;

    for (i = 0; i < N_ACTIONS; i++) {
	char fname[strlen(actions[i].wav_fname) + 100];
	sprintf(fname, "%s.wav", actions[i].wav_fname);
	tracks[i] = track_new(fname);
	if (! tracks[i]) {
	    perror(fname);
	    exit(1);
	}
	sprintf(fname, "%s-servo.wav", actions[i].wav_fname);
	actors[i] = talking_skull_actor_new(fname, update_mouth, NULL);
	if (! actors[i]) {
	    perror(fname);
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
	    nano_add_ms(&head_at, a->head.speed_ms);
	}
	if (nano_later_than(&now, &tail_at)) {
	    tail_left = !tail_left;
	    maestro_set_servo_pos(m, TAIL_ID, 50 + (-1*tail_left)*a->tail.delta_pct);
	    nano_add_ms(&tail_at, a->tail.speed_ms);
	}
    }
}

static void
reset_servos(void)
{
    maestro_set_servo_pos(m, MOUTH_ID, 0);
    maestro_set_servo_pos(m, HEAD_ID, 50);
    maestro_set_servo_pos(m, TAIL_ID, 50);
}

int
main(int argc, char **argv)
{
    stop_t *stop;
    int last_track = -1;

    seed_random();
    pi_usb_init();

    if (wb_init() < 0) {
	fprintf(stderr, "Failed to initialize the weenboard\n");
	exit(1);
    }

    if ((m = maestro_new()) == NULL) {
        fprintf(stderr, "Failed to initialize servo controller\n");
        exit(1);
    }

    stop = stop_new();

    maestro_set_servo_range(m, MOUTH_ID, BAXTER_MOUTH);
    maestro_set_servo_range(m, HEAD_ID, BAXTER_HEAD);
    maestro_set_servo_range(m, TAIL_ID, BAXTER_TAIL);

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
	talking_skull_actor_play(actors[track]);
	track_play_asynchronously(tracks[track], stop);
	play_track(&actions[track], stop);
	reset_servos();
    }
}

