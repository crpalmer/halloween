#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "time-utils.h"
#include "track.h"
#include "util.h"
#include "wb.h"

#define MS_TO_HIT 1000
#define MS_WAIT_FOR_UP 100

#define MOLARi(i) 1, i
#define BUTTON_OUT 1, 6
#define BUTTON_IN 6

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
    
int
main(int argc, char **argv)
{
    track_t *track;

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

    while (true) {
	struct timespec start;

	ms_sleep(random_number_in_range(1000, 2000));
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
    }
	
    return 0;
}
