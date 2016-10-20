#include <stdio.h>
#include <stdlib.h>
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

#define HEAD_ID 0
#define TAIL_ID 1

typedef struct {
    unsigned speed_ms;
    double delta_pct;
} movement_t;

typedef struct {
    movement_t head, tail;
    const char *wav_fname;
} action_t;

static action_t actions[] = {
    { {  800, 11.1 }, {  640,  4.4 }, "baxter/00.wav" },
    { { 4000, 27.8 }, { 1000,  2.8 }, "baxter/01.wav" },
    { {  400, 22.2 }, {  400, 11.1 }, "baxter/02.wav" },
    { {  600, 16.7 }, {  500, 27.8 }, "baxter/03.wav" },
    { {  200,  5.6 }, {  280,  3.9 }, "baxter/04.wav" }
};

#define N_ACTIONS (sizeof(actions) / sizeof(actions[0]))

static track_t *track[N_ACTIONS];

static void
load_tracks(void)
{
    size_t i;

    for (i = 0; i < N_ACTIONS; i++) {
	track[i] = track_new(actions[i].wav_fname);
	if (! track[i]) {
	    perror(actions[i].wav_fname);
	    exit(1);
	}
    }
}

static void
setup_servo(unsigned id, movement_t *m)
{
    printf("speed %d %d\n", id, m->speed_ms);
    printf("range %d %f..%f\n", id, 50 - m->delta_pct, 50 + m->delta_pct);
}

static void
play_track(action_t *a)
{
    int head_left = 1, tail_left = 1;
    struct timespec head_at, tail_at;

    setup_servo(HEAD_ID, &a->head);
    setup_servo(TAIL_ID, &a->tail);

    nano_gettime(&head_at);
    nano_gettime(&tail_at);

    nano_add_ms(&head_at, a->head.speed_ms/2);
    nano_add_ms(&tail_at, a->tail.speed_ms/2);

    while (1) {
	struct timespec now;

	nano_gettime(&now);
	if (nano_later_than(&now, &head_at)) {
	    head_left = !head_left;
	    printf("Move head to %s\n", head_left ? "left" : "right");
	    nano_add_ms(&head_at, a->head.speed_ms);
	}
	if (nano_later_than(&now, &tail_at)) {
	    tail_left = !tail_left;
	    printf("Move tail to %s\n", tail_left ? "left" : "right");
	    nano_add_ms(&tail_at, a->tail.speed_ms);
	}
    }
}

static void
reset_servos(void)
{
    printf("move servos back to middle\n");
}

int
main(int argc, char **argv)
{
    load_tracks();
    while (1) {
	ms_sleep(random_number_in_range(INTER_LOW, INTER_HIGH));
	play_track(&actions[random_number_in_range(0, N_ACTIONS-1)]);
	reset_servos();
    }
}

