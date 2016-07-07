#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "server.h"
#include "time-utils.h"
#include "util.h"
#include "wb.h"

#include "animation-actions.h"
#include "animation-common.h"
#include "animation-lights.h"

#define BUTTON_LOCKOUT_MS 1000
#define MAX_STATIONS 8

static lights_t *lights[MAX_STATIONS];
static unsigned min_pin[MAX_STATIONS];
static unsigned max_pin[MAX_STATIONS];
static unsigned wb_mask[MAX_STATIONS];

static pthread_t thread[MAX_STATIONS];
static pthread_mutex_t station_lock[MAX_STATIONS];
static pthread_cond_t station_cond[MAX_STATIONS];
static unsigned current_pin[MAX_STATIONS];

static void
do_prop_common_locked(unsigned station, unsigned pin)
{
    lights_select(lights[station], pin);
    stations[station].actions[pin].action(stations[station].actions[pin].action_data, lights[station], pin + min_pin[station]);
    lights_chase(lights[station]);
}

static void
handle_button_press(unsigned station, unsigned pin)
{
    pthread_mutex_lock(stations[station].lock);
    do_prop_common_locked(station, pin);
    pthread_mutex_unlock(stations[station].lock);
}

static char *
remote_event(void *unused, const char *command)
{
    unsigned station;

    for (station = 0; stations[station].actions; station++) {
	action_t *actions = stations[station].actions;
	unsigned pin;

	for (pin = 0; actions[pin].action; pin++) {
	    if (strcmp(actions[pin].cmd, command) == 0) {
		if (pthread_mutex_trylock(stations[station].lock) != 0) {
		    return strdup("prop is busy");
		}
		do_prop_common_locked(station, pin);
		pthread_mutex_unlock(stations[station].lock);
		return strdup(SERVER_OK);
	     }
	}
    }
	    
    fprintf(stderr, "Invalid net command: [%s]\n", command);
    return strdup("Invalid command\n");
}

static void *
station_main(void *station_as_vp)
{
    unsigned station = (unsigned) station_as_vp;
    enum { READY, LOCKOUT } state = READY;
    struct timespec last_button;
    unsigned pin;
    pthread_mutex_t *lock = &station_lock[station];
    pthread_cond_t  *cond = &station_cond[station];
    lights_t         *l   = lights[station];

    lights_chase(l);
    while (true) {
	pthread_mutex_lock(lock);
	while (current_pin[station] == -1) {
	    if (state == LOCKOUT) {
		struct timespec now, wait_until;

		wait_until = last_button;
		nano_add_ms(&wait_until, BUTTON_LOCKOUT_MS);

		pthread_cond_timedwait(cond, lock, &wait_until);

		nano_gettime(&now);
		if (nano_later_than(&now, &wait_until)) {
		    state = READY;
		    lights_chase(l);
		} else if (current_pin[station] != -1) {
		    last_button = now;
		    current_pin[station] = -1;
		}
	    } else {
		pthread_cond_wait(cond, lock);
	    }
	}
	pin = current_pin[station];
	current_pin[station] = -1;
	pthread_mutex_unlock(lock);


	if (state == READY) {
	    lights_select(l, pin);
	    handle_button_press(station, pin);
	    lights_off(l);
	    state = LOCKOUT;
	    nano_gettime(&last_button);
	}
    }

    return NULL;
}

static void
init_stations(void)
{
    unsigned pin0 = 1;
    unsigned i;

    for (i = 0; stations[i].actions && i < MAX_STATIONS; i++) {
	unsigned j;
	pthread_condattr_t attr;

	wb_mask[i] = 0;
	for (j = 0; stations[i].actions[j].action; j++) {
	    wb_mask[i] |= WB_PIN_MASK(pin0 + j);
	}
	min_pin[i] = pin0;
	lights[i] = lights_new(pin0, pin0 + j);
	pin0 += j;
	max_pin[i] = pin0;
	current_pin[i] = -1;

	pthread_mutex_init(&station_lock[i], NULL);
	pthread_condattr_init(&attr);
	pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
	pthread_cond_init(&station_cond[i], &attr);

	pthread_create(&thread[i], NULL, station_main, (void *) i);
    }

    while (i < MAX_STATIONS) {
	min_pin[i] = max_pin[i] = 9;
	lights[i] = NULL;
	i++;
    }
}

void
animation_main(void)
{
    server_args_t server_args;
    pthread_t server_thread;

    seed_random();
    wb_init();
    actions_init();

    init_stations();

    server_args.port = 5555;
    server_args.command = remote_event;
    server_args.state = NULL;

    pthread_create(&server_thread, NULL, server_thread_main, &server_args);

    while (true) {
	unsigned pin = wb_wait_for_pins(WB_PIN_MASK_ALL, WB_PIN_MASK_ALL);
	unsigned station;

	for (station = 0; station < MAX_STATIONS; station++) {
	    if (pin < min_pin[station] || pin > max_pin[station]) continue;
	    pin -= min_pin[station];
	    pthread_mutex_lock(&station_lock[station]);
	    current_pin[station] = pin;
	    pthread_cond_signal(&station_cond[station]);
	    pthread_mutex_unlock(&station_lock[station]);
	}
    }
}
