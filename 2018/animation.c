#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "track.h"
#include "util.h"
#include "wb.h"

#include "animation-actions.h"
#include "animation-common.h"
#include "animation-lights.h"
#include "fogger.h"

#define CHICKEN     "chicken"
#define SQUID       "squid"
#define QUESTION    "question"
#define INTESTINES  "intestines"
#define FROG	    "frog"

#define FOGGER_PIN	1, 8
#define SNAKE_PIN2	8

#define DOG_MIN_MS	5000
#define DOG_MAX_MS	6000

static pthread_mutex_t station_lock, mermaid_lock;
static track_t *laugh, *chicken;
static stop_t *stop;

static void
do_attack2(unsigned pin, unsigned pin2, lights_t *l, double up, double down)
{
    unsigned i;

    for (i = 0; i < 3; i++) {
	wb_set(ANIMATION_OUTPUT_BANK, pin, 1);
	if (pin2 != -1) wb_set(ANIMATION_OUTPUT_BANK, pin2, 1);
	ms_sleep((500 + random_number_in_range(0, 250) - 125)*up);
	wb_set(ANIMATION_OUTPUT_BANK, pin, 0);
	if (pin2 != -1) wb_set(ANIMATION_OUTPUT_BANK, pin2, 0);
	ms_sleep((200 + random_number_in_range(0, 100) - 50)*down);
    }
}

static void
do_attack(unsigned pin, lights_t *l, double up, double down)
{
    do_attack2(pin, -1, l, up, down);
}

static void
do_chicken(void *unused, lights_t *l, unsigned pin)
{
    wb_set(ANIMATION_OUTPUT_BANK, pin, 1);
    track_set_volume(chicken, 100);
    track_play(chicken);
    wb_set(ANIMATION_OUTPUT_BANK, pin, 0);
}

static void
do_squid(void *unused, lights_t *l, unsigned pin)
{
    wb_set(ANIMATION_OUTPUT_BANK, pin, 1);
    ms_sleep(5000);
    wb_set(ANIMATION_OUTPUT_BANK, pin, 0);
}

static void
do_question(void *unused, lights_t *l, unsigned pin)
{
    track_set_volume(laugh, 85);
    track_play_asynchronously(laugh, stop);
    lights_blink(l);
    wb_set(ANIMATION_OUTPUT_BANK, pin, 1);
    stop_await_stop(stop);
    wb_set(ANIMATION_OUTPUT_BANK, pin, 0);
}

static void
do_intestines(void *unused, lights_t *l, unsigned pin)
{
    do_attack(pin, l, 1, 1.5);
}

static void
do_frog(void *unused, lights_t *l, unsigned pin)
{
    do_attack(pin, l, 1, 1);
}

static action_t main_actions[] = {
    { CHICKEN,  do_chicken,	NULL },
    { SQUID,	do_squid,	NULL },
    { QUESTION, do_question,	NULL },
    { FROG,	do_frog,	NULL },
    { INTESTINES, do_intestines, NULL },
    { NULL,	NULL,		NULL },
};

static station_t stations[] = {
    { true, main_actions, &station_lock },
    { NULL, NULL },
};

int
main(int argc, char **argv)
{
    pthread_mutex_init(&station_lock, NULL);
    pthread_mutex_init(&mermaid_lock, NULL);

    if ((laugh = track_new("laugh.wav")) == NULL) {
	perror("laugh.wav");
	exit(1);
    }

    if ((chicken = track_new("chicken.wav")) == NULL) {
	perror("chicken.wav");
	exit(1);
    }

    stop = stop_new();

    fogger_run_in_background(FOGGER_PIN, NULL);
    animation_main(stations);

    return 0;
}

