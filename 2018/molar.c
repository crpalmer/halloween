#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "digital-counter.h"
#include "time-utils.h"
#include "track.h"
#include "util.h"
#include "wb.h"

#include "animation-actions.h"
#include "animation-common.h"
#include "animation-lights.h"

#define MAX_AT_ONCE	3
#define GAME_MS		15000
#define MS_TO_LOWER	400
#define MS_TO_HIT 	(MS_TO_LOWER + 100)
#define MS_WAIT_FOR_UP	250
#define MS_BETWEEN	250
#define DEBOUNCE_MS	2
#define UP_DEBOUNCE_MS	10

#define N_MOLARS	5
#define BROKENi(i)	(1 << ((i) - 1))
#define BROKEN_MOLARS	0 // (BROKENi(4) | BROKENi(5))


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
static digital_counter_t *score_display, *high_score_display;
static track_t *go_track;
static track_t *game_over_track;
static track_t *high_score_track;
static int high_score;

static const int points[] = { 10, 15, 20 };

#define DEBUG_PLAY	1
#define DEBUG_AUDIO	0
#define DEBUG_SHOW_SCORES 1

static struct {
    const char *fname;
    double	pct;
    track_t    *track;
} hit_tracks[] = {
    { "elfie-hit.wav",   85, },
    { "elfie-hit-1.wav",  4, },
    { "elfie-hit-2.wav",  4, },
    { "elfie-hit-3.wav",  4, },
    { "elfie-hit-4.wav",  1, },
    { "elfie-hit-long.wav", 2 }
};

static const int n_hit_tracks = sizeof(hit_tracks) / sizeof(hit_tracks[0]);

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

static track_t *
pick_hit_track(int *is_default_track)
{
    double pct = random_number() * 100;
    int i;

    for (i = 0; i < n_hit_tracks; i++) {
	if (pct < hit_tracks[i].pct) {
	    *is_default_track = (i == 0);
	    return hit_tracks[i].track;
	}
        pct -= hit_tracks[i].pct;
    }

    *is_default_track = 1;

    return hit_tracks[0].track;
}

static void *
hit_sound_main(void *unused)
{
    int i;
    int is_default_track = 1;

    for (i = 0; i < n_hit_tracks; i++) {
	if ((hit_tracks[i].track = track_new(hit_tracks[i].fname)) == NULL) {
	    exit(1);
	}
    }

    while (true) {
	pthread_mutex_lock(&hit_sound_lock);
	if (DEBUG_AUDIO) printf("Waiting for hit_sound request\n");
	while (! hit_sound_needed) pthread_cond_wait(&hit_sound_cond, &hit_sound_lock);
	hit_sound_needed = 0;
	if (DEBUG_AUDIO) printf("request received: is_default_track = %d, hit_sound stopped? %d\n", is_default_track, stop_is_stopped(hit_sound_stop));
	if (is_default_track || stop_is_stopped(hit_sound_stop)) {
	    if (DEBUG_AUDIO) printf("playing track\n");
	    stop_request_stop(hit_sound_stop);
	    stop_reset(hit_sound_stop);
	    pthread_mutex_unlock(&hit_sound_lock);

	    track_play_with_stop(pick_hit_track(&is_default_track), hit_sound_stop);
	} else {
	    if (DEBUG_AUDIO) printf("ignoring track\n");
	    pthread_mutex_unlock(&hit_sound_lock);
	}
    }
}

static void
hit_sound_play()
{
    pthread_mutex_lock(&hit_sound_lock);
    if (DEBUG_AUDIO) printf("requesting track\n");
    hit_sound_needed = 1;
    pthread_mutex_unlock(&hit_sound_lock);
    pthread_cond_signal(&hit_sound_cond);
}

static void
hit_sound_wait()
{
    while (! stop_is_stopped(hit_sound_stop)) {}
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

    wb_set(BUTTON_OUT, 0);

    digital_counter_set(score_display, 0);
    track_play(go_track);

    nano_gettime(&game_start);
    while (nano_elapsed_ms_now(&game_start) < GAME_MS) {
	int n = random_number_in_range(1, MAX_AT_ONCE);
	int i;
	int molars;
	int this_n_hit = 0;
	struct timespec start;
	int are_up = 1;

	for (i = molars = 0; i < n; i++) {
	    int molar;

	    do {
		molar = 1 << random_number_in_range(0, N_MOLARS-1);
	    } while ((BROKEN_MOLARS & molar) != 0);

	    molars |= molar;
	}
	
	n = molars_set(molars, MOLAR_UP);
	ms_sleep(MS_WAIT_FOR_UP);
	if (DEBUG_PLAY) printf("%4d up %x\n", 0, molars);
	nano_gettime(&start);
	while (nano_elapsed_ms_now(&start) < MS_TO_HIT && (wb_get_all_with_debounce(UP_DEBOUNCE_MS) & molars) != molars) {}
	if (DEBUG_PLAY) printf("%4d ready\n", nano_elapsed_ms_now(&start));
	while (nano_elapsed_ms_now(&start) < MS_TO_HIT && molars) {
	    int hit = ~wb_get_all_with_debounce(DEBOUNCE_MS);
	    if ((molars & hit) != 0) {
		int n_down;

		hit_sound_play();
		n_down = molars_set(molars & hit, MOLAR_DOWN);
		molars = molars & ~hit;
		n_hit += n_down;
		if (DEBUG_PLAY) printf("%4d hit %d - %x\n", nano_elapsed_ms_now(&start), n_down, molars);
		while (n_down--) score += points[this_n_hit++];
		digital_counter_set(score_display, score);
	    }
	    if (are_up && nano_elapsed_ms_now(&start) > MS_TO_LOWER) {
		if (DEBUG_PLAY) printf("%4d down\n", nano_elapsed_ms_now(&start));
		molars_set(molars, MOLAR_DOWN);
		are_up = 0;
	    }
	
	}
	molars_set(molars, MOLAR_DOWN); /* just incase */
	if (DEBUG_PLAY) printf("%4d done %x\n", nano_elapsed_ms_now(&start), molars);
	ms_sleep(MS_BETWEEN);
    }
    if (DEBUG_SHOW_SCORES) printf("done with %d hit, score %d\n", n_hit, score);
    ms_sleep(1000);
    hit_sound_wait();
    if (score > high_score) {
	high_score = score;
	digital_counter_set(high_score_display, high_score);
	track_play(high_score_track);
    } else {
	track_play(game_over_track);
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
    int i;

    seed_random();

    if (wb_init() < 0) {
	perror("wb_init");
	exit(1);
    }

    score_display = digital_counter_new(2, 1, -1, 2);
    high_score_display = digital_counter_new(2, 3, -1, 4);
    digital_counter_set_pause(high_score_display, 20, -1, -1);

    if ((go_track = track_new("elfie-ready-set-go.wav")) == NULL) {
	exit(1);
    }

    if ((high_score_track = track_new("elfie-winner.wav")) == NULL) {
	exit(1);
    }

    if ((game_over_track = track_new("elfie-loser.wav")) == NULL) {
	exit(1);
    }

    for (i = 0; i < N_MOLARS; i++) wb_set_pull_up(i+1, WB_PULL_UP_UP);

    if (argc == 2 && strcmp(argv[1], "--test") == 0) {
	test();
	exit(0);
    }

    pthread_mutex_init(&station_lock, NULL);
    pthread_mutex_init(&hit_sound_lock, NULL);
    pthread_cond_init(&hit_sound_cond, NULL);

    hit_sound_stop = stop_new();

    pthread_create(&hit_sound_thread, NULL, hit_sound_main, NULL);

    molars_set((1<<N_MOLARS)-1, 0);
    animation_main_with_pin0(stations, BUTTON_IN);

    return 0;
}
