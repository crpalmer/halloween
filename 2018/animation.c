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

#define TOAD        "toad"
#define HOP	    "hop"
#define QUESTION    "question"
#define SNAKE       "snake"
#define CAT         "cat"

#define FOGGER_PIN	1, 8
#define SNAKE_PIN	2, 8

#define DOG_MIN_MS	5000
#define DOG_MAX_MS	6000

static pthread_mutex_t station_lock, mermaid_lock;
static track_t *laugh;
static stop_t *stop;

static void
do_attack(unsigned pin, lights_t *l, double up, double down)
{
    unsigned i;

    for (i = 0; i < 3; i++) {
	wb_set(ANIMATION_OUTPUT_BANK, pin, 1);
	ms_sleep((500 + random_number_in_range(0, 250) - 125)*up);
	wb_set(ANIMATION_OUTPUT_BANK, pin, 0);
	ms_sleep((200 + random_number_in_range(0, 100) - 50)*down);
    }
}

static void
do_hop(void *unused, lights_t *l, unsigned pin)
{
    do_attack(pin, l, 1, 1.5);
}

static void
do_cat(void *unused, lights_t *l, unsigned pin)
{
    do_attack(pin, l, 2, 3.0);
}

static void
do_question(void *unused, lights_t *l, unsigned pin)
{
    track_play_asynchronously(laugh, stop);
    lights_blink(l);
    wb_set(ANIMATION_OUTPUT_BANK, pin, 1);
    stop_await_stop(stop);
    wb_set(ANIMATION_OUTPUT_BANK, pin, 0);
}

static void
do_toad(void *unused, lights_t *l, unsigned pin)
{
    do_attack(pin, l, 1, 1.5);
}

static void
do_snake(void *unused, lights_t *l, unsigned pin)
{
    wb_set(SNAKE_PIN, 1);
    do_attack(pin, l, 1, 3);
    wb_set(SNAKE_PIN, 0);
}

static action_t main_actions[] = {
    { TOAD,     do_toad,	NULL },
    { HOP,	do_hop,		NULL },
    { QUESTION, do_question,	NULL },
    { SNAKE,	do_snake,	NULL },
    { CAT,	do_cat,		NULL },
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

    stop = stop_new();

    fogger_run_in_background(FOGGER_PIN, NULL);
    animation_main(stations);

    return 0;
}

