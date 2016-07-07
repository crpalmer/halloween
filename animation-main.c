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

static void
do_prop_common_locked(unsigned pin)
{
    if (pin <= N_STATION_BUTTONS) lights_select(pin);
    actions[pin].action(actions[pin].action_data, pin);
    if (pin <= N_STATION_BUTTONS) lights_chase();
}

static void
handle_button_press(unsigned pin)
{
    pthread_mutex_lock(actions[pin].lock);
    do_prop_common_locked(pin);
    pthread_mutex_unlock(actions[pin].lock);
}

static char *
remote_event(void *unused, const char *command)
{
    unsigned pin;

    for (pin = 1; actions[pin].action; pin++) {
	if (strcmp(actions[pin].cmd, command) == 0) {
	    if (pthread_mutex_trylock(actions[pin].lock) != 0) return strdup("prop is busy");
	    do_prop_common_locked(pin);
	    pthread_mutex_unlock(actions[pin].lock);
	    return strdup(SERVER_OK);
	}
    }
	    
    fprintf(stderr, "Invalid net command: [%s]\n", command);
    return strdup("Invalid command\n");
}

static void
wait_for_no_buttons(void)
{
    while (wb_get_all()) {
    }
}

void
animation_main(void)
{
    server_args_t server_args;
    pthread_t server_thread;

    seed_random();
    actions_init();
    lights_init();

    server_args.port = 5555;
    server_args.command = remote_event;
    server_args.state = NULL;

    lights_chase();

    pthread_create(&server_thread, NULL, server_thread_main, &server_args);

    while (true) {
	unsigned pin = wb_wait_for_pins(WB_PIN_MASK_ALL, WB_PIN_MASK_ALL);
	lights_select(pin);
	handle_button_press(pin);
	wait_for_no_buttons();
	if (pin <= N_STATION_BUTTONS) {
	    lights_off();
	    ms_sleep(BUTTON_LOCKOUT_MS);
	}
	lights_chase();
    }
}
