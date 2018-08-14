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

static pthread_mutex_t station_lock;

static pthread_t hit_sound_thread;
static pthread_mutex_t hit_sound_lock;
static pthread_cond_t hit_sound_cond;
static stop_t *hit_sound_stop;
static int hit_sound_needed;

static const int points[] = { 10, 15, 20 };

static void
test()
{
    int i;

    printf("Testing the button\n");
    wb_set(BUTTON_OUT, 1);
    while (! wb_get(BUTTON_IN)) {}
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

static void *
hit_sound_main(void *unused)
{
    track_t *track;

    if ((track = track_new("beep.wav")) == NULL) {
	perror("beep.wav");
	exit(1);
    }

    while (true) {
	pthread_mutex_lock(&hit_sound_lock);
	printf("Waiting for hit_sound request\n");
	while (! hit_sound_needed) pthread_cond_wait(&hit_sound_cond, &hit_sound_lock);
	hit_sound_needed = 0;
	stop_reset(hit_sound_stop);
	printf("playing track\n");
	pthread_mutex_unlock(&hit_sound_lock);

	track_play_with_stop(track, hit_sound_stop);
    }
}

static void
hit_sound_play()
{
    pthread_mutex_lock(&hit_sound_lock);
    printf("requesting track\n");
    hit_sound_needed = 1;
    stop_request_stop(hit_sound_stop);
    pthread_mutex_unlock(&hit_sound_lock);
    pthread_cond_signal(&hit_sound_cond);
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
	printf("%4d up %x\n", 0, molars);
	nano_gettime(&start);
	while (nano_elapsed_ms_now(&start) < MS_TO_HIT && (wb_get_all() & molars) != 0) {}
	printf("%4d ready\n", nano_elapsed_ms_now(&start));
	while (nano_elapsed_ms_now(&start) < MS_TO_HIT && molars) {
	    int hit = wb_get_all();
	    if ((molars & hit) != 0) {
		int n_down;

		hit_sound_play();
		n_down = molars_set(molars & hit, MOLAR_DOWN);
		molars = molars & ~hit;
		n_hit += n_down;
		printf("%4d hit %d - %x\n", nano_elapsed_ms_now(&start), n_down, molars);
		while (n_down--) score += points[this_n_hit++];
	    }
	}
	printf("%4d done %x\n", nano_elapsed_ms_now(&start), molars);
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

    pthread_mutex_init(&station_lock, NULL);
    pthread_mutex_init(&hit_sound_lock, NULL);
    pthread_cond_init(&hit_sound_cond, NULL);

    hit_sound_stop = stop_new();

    pthread_create(&hit_sound_thread, NULL, hit_sound_main, NULL);

    animation_main_with_pin0(stations, BUTTON_IN);

    return 0;
}
