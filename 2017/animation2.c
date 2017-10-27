#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "util.h"
#include "wb.h"

#include "animation-actions.h"
#include "animation-common.h"
#include "animation-lights.h"

#define FROG		"frog"
#define HOP		"hop"

#define SNAKE_PIN	2, 8

static pthread_mutex_t station_lock;

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
do_frog(void *unused, lights_t *l, unsigned pin)
{
    do_attack(pin, l, 1, 1);
}

static void
do_hop(void *unused, lights_t *l, unsigned pin)
{
    do_attack(pin, l, 1, 1.5);
}

static action_t main_actions[] = {
    { FROG,	do_frog,	NULL },
    { HOP,	do_hop,		NULL },
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
    animation_main(stations);

    return 0;
}

