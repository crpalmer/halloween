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

int
main(int argc, char **argv)
{
    track_t *track;

    seed_random();

    if (wb_init() < 0) {
	perror("wb_init");
	exit(1);
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
