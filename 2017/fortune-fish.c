#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "animation-actions.h"
#include "animation-common.h"
#include "track.h"
#include "util.h"
#include "wb.h"

#define HAND_PIN  1

static void fortune(void *unused, lights_t *l, unsigned pin)
{
    fprintf(stderr, "Fortune time!\n");
}

static void waiting_for_button(unsigned ms)
{
}

static pthread_mutex_t lock;

static action_t actions[] = {
    { "fortune",  fortune, NULL },
    { NULL, NULL, NULL }
};

static station_t stations[] = {
    { actions, &lock, waiting_for_button, 1000 },
    { NULL, NULL }
};

int
main(int argc, char **argv)
{
    if (wb_init() < 0) {
	fprintf(stderr, "Failed to initialize wb\n");
	exit(1);
    }

    pthread_mutex_init(&lock, NULL);

    wb_set_pull_up(HAND_PIN, WB_PULL_UP_DOWN);

    animation_main(stations);

    return 0;
}
