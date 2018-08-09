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
#define MS_TO_HIT 700
#define MS_WAIT_FOR_UP 300
#define MS_BETWEEN	250

#define N_MOLARS 4

#define MOLARi(i) 1, i
#define BUTTON_OUT 1, 6
#define BUTTON_IN 6

#define MOLAR_UP 1
#define MOLAR_DOWN 0

static track_t *track;
static pthread_mutex_t station_lock;

static const int points[] = { 10, 15, 20 };

static void
test()
{
    int i;

    printf("Testing the button\n");
    wb_set(BUTTON_OUT, 1);
//    while (! wb_get(BUTTON_IN)) {}
    wb_set(BUTTON_OUT, 0);
    for (i = 1; i <= N_MOLARS; i++) {
	printf("Testing tooth %d\n", i);
	wb_set(MOLARi(i), MOLAR_UP);
	ms_sleep(MS_WAIT_FOR_UP);
	printf("   waiting for it to be not triggered\n");
	while (wb_get(i)) {}
	printf("   waiting for it to be triggered\n");
	while (! wb_get(i)) {}
	wb_set(MOLARi(i), MOLAR_DOWN);
    }
}

#define PLAY "play"

static int
molars_set(unsigned molars, unsigned val)
{
    int i;
    int n = 0;

    for (i = 0; i < N_MOLARS; i++) {
	if (molars & (1 << i)) {
	    wb_set(MOLARi(i+1), val);
	    n++;
	}
    }

    return n;
}

static void
play()
{
    struct timespec game_start;
    int n_hit = 0;
    int score = 0;

    nano_gettime(&game_start);
    while (nano_elapsed_ms_now(&game_start) < GAME_MS) {
	int n = random_number_in_range(1, 3);
	int i;
	int molars;
	int this_n_hit = 0;
	struct timespec start;

	for (i = molars = 0; i < n; i++) {
	    molars |= 1 << random_number_in_range(0, N_MOLARS-1);
	}
	
	n = molars_set(molars, MOLAR_UP);
	ms_sleep(MS_WAIT_FOR_UP);
	printf("up %x\n", molars); fflush(stdout);
	nano_gettime(&start);
	while (nano_elapsed_ms_now(&start) < MS_TO_HIT && (wb_get_all() & molars) != 0) {}
	printf("ready\n"); fflush(stdout);
	while (nano_elapsed_ms_now(&start) < MS_TO_HIT && molars) {
	    int hit = wb_get_all();
	    if ((molars & hit) != 0) {
		int n_down;

		track_play(track);
		n_down = molars_set(molars & hit, MOLAR_DOWN);
		molars = molars & ~hit;
		n_hit += n_down;
		printf("hit %d - %x\n", n_down, molars);
		while (n_down--) score += points[this_n_hit++];
	    }
	}
	printf("done %x\n", molars);
	molars_set(molars, MOLAR_DOWN);
	ms_sleep(MS_BETWEEN);
    }
    printf("done with %d hit, score %d\n", n_hit, score);
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
