#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "time-utils.h"
#include "track.h"
#include "util.h"
#include "wb.h"

#include "animation-actions.h"
#include "animation-common.h"
#include "animation-lights.h"

#define GAME_MS		15000
#define MS_TO_HIT 1000
#define MS_WAIT_FOR_UP 100
#define MS_BETWEEN	500

#define MOLARi(i) 1, i
#define BUTTON_OUT 1, 6
#define BUTTON_IN 6

static track_t *track;
static pthread_mutex_t station_lock;

static void
test()
{
    int i;

    printf("Testing the button\n");
    wb_set(BUTTON_OUT, 1);
    while (! wb_get(BUTTON_IN)) {}
    wb_set(BUTTON_OUT, 0);
    for (i = 1; i <= 5; i++) {
	printf("Testing tooth %d\n", i);
	wb_set(MOLARi(i), 1);
	ms_sleep(MS_WAIT_FOR_UP);
	printf("   waiting for it to be not triggered\n");
	while (wb_get(i)) {}
	printf("   waiting for it to be triggered\n");
	while (! wb_get(i)) {}
	wb_set(MOLARi(i), 0);
    }
}

#define PLAY "play"

static void
play()
{
    struct timespec game_start;

    nano_gettime(&game_start);
    while (nano_elapsed_ms_now(&game_start) < GAME_MS) {
	struct timespec start;
	wb_set(MOLARi(1), 1);
	ms_sleep(MS_WAIT_FOR_UP);
	printf("up\n"); fflush(stdout);
	nano_gettime(&start);
	while (nano_elapsed_ms_now(&start) < MS_TO_HIT && wb_get(1)) {}
	printf("ready\n"); fflush(stdout);
	while (nano_elapsed_ms_now(&start) < MS_TO_HIT) {
	    if (wb_get(1)) {
		track_play(track);
		printf("hit\n"); fflush(stdout);
		break;
	    }
	}
	wb_set(MOLARi(1), 0);
	ms_sleep(MS_BETWEEN);
    }
}

static action_t main_actions[] = {
    { PLAY,     play,           NULL },
    { NULL,     NULL,           NULL },
};

static station_t stations[] = {
    { true, main_actions, &station_lock },
    { NULL, NULL },
};

int
main(int argc, char **argv)
{
    seed_random();

    if (wb_init() < 0) {
	perror("wb_init");
	exit(1);
    }

    if (argc == 2 && strcmp(argv[1], "--test") == 0) {
	test();
	exit(0);
    }

    if ((track = track_new("beep.wav")) == NULL) {
	perror("beep.wav");
	exit(1);
    }

    pthread_mutex_init(&station_lock, NULL);

    animation_main_with_pin0(stations, BUTTON_IN);

    return 0;
}
