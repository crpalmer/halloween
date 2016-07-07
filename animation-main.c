#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "server.h"
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

static void
do_prop_common_locked(unsigned station, unsigned pin)
{
    lights_select(lights[station], pin);
    stations[station].actions[pin].action(stations[station].actions[pin].action_data, lights[station], pin);
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

static void
wait_for_no_buttons(unsigned station)
{
    while ((wb_get_all() & wb_mask[station]) != 0) {
    }
}

static void
init_stations(void)
{
    unsigned pin0 = 1;
    unsigned i;

    for (i = 0; stations[i].actions && i < MAX_STATIONS; i++) {
	unsigned j;

	wb_mask[i] = 0;
	for (j = 0; stations[i].actions[j].action; j++) {
	    wb_mask[i] |= WB_PIN_MASK(pin0 + j);
	}
	min_pin[i] = pin0;
	lights[i] = lights_new(pin0, pin0 + j);
	lights_chase(lights[i]);
	pin0 += j;
	max_pin[i] = pin0;
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
	    lights_select(lights[station], pin);
	    handle_button_press(station, pin);
	    wait_for_no_buttons(station);
	    lights_off(lights[station]);
	    ms_sleep(BUTTON_LOCKOUT_MS);
	    lights_chase(lights[station]);
	}
    }
}
