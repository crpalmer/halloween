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
static struct timespec start_waiting[MAX_STATIONS];

static station_t *stations;
static unsigned pin_mask = 0;

static void
do_prop_common_locked(unsigned station, unsigned pin)
{
    if (lights[station]) lights_select(lights[station], pin);
    stations[station].actions[pin].action(stations[station].actions[pin].action_data, lights[station], pin + min_pin[station]);
    if (lights[station]) lights_chase(lights[station]);
}

static char *
remote_event(void *unused, const char *command, struct sockaddr_in *addr, size_t size)
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
		nano_gettime(&start_waiting[station]);
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
    struct timespec last_button, last_notify;
    unsigned pin;
    pthread_mutex_t *lock = &station_lock[station];
    pthread_mutex_t *prop_lock = stations[station].lock;
    pthread_cond_t  *cond = &station_cond[station];
    lights_t         *l   = lights[station];
    station_t       *s    = &stations[station];

    if (l) lights_chase(l);
    nano_gettime(&start_waiting[station]);
    last_notify = start_waiting[station];

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
		    if (l) lights_chase(l);
		    nano_gettime(&start_waiting[station]);
		    last_notify = start_waiting[station];
		} else if (current_pin[station] != -1) {
		    last_button = now;
		    current_pin[station] = -1;
		}
	    } else if (s->waiting_period_ms) {
		struct timespec now, wait_until;

		wait_until = last_notify;
		nano_add_ms(&wait_until, s->waiting_period_ms);
		
		pthread_cond_timedwait(cond, lock, &wait_until);

		pthread_mutex_lock(prop_lock);
		nano_gettime(&now);
		wait_until = last_notify;
		nano_add_ms(&wait_until, s->waiting_period_ms);

		if (current_pin[station] == -1 && nano_later_than(&now, &wait_until)) {
		    s->waiting(nano_elapsed_ms(&now, &start_waiting[station]));
		    last_notify = now;
		}
		pthread_mutex_unlock(prop_lock);

	    } else {
		pthread_cond_wait(cond, lock);
	    }
	}
	pin = current_pin[station];
	current_pin[station] = -1;
	pthread_mutex_unlock(lock);


	if (state == READY) {
	    if (l) lights_select(l, pin);
	    pthread_mutex_lock(prop_lock);
	    do_prop_common_locked(station, pin);
	    pthread_mutex_unlock(prop_lock);
	    if (l) lights_off(l);
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
	pin_mask |= wb_mask[i];
	min_pin[i] = pin0;
	max_pin[i] = pin0 + j-1;
	if (stations[i].has_lights) {
	    lights[i] = lights_new(min_pin[i], max_pin[i]);
	} else {
	     lights[i] = NULL;
	}

	pin0 += max_pin[i];
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
animation_main(station_t *stations_)
{
    server_args_t server_args;
    pthread_t server_thread;

    stations = stations_;

    seed_random();
    if (wb_init() < 0) {
	fprintf(stderr, "Failed to initialize the wb\n");
	exit(1);
    }

    init_stations();

    server_args.port = 5555;
    server_args.command = remote_event;
    server_args.state = NULL;

    pthread_create(&server_thread, NULL, server_thread_main, &server_args);

    while (true) {
	unsigned pin = wb_wait_for_pins(pin_mask, pin_mask);
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
