#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "lights.h"
#include "gpio.h"
#include "piface.h"
#include "server.h"
#include "track.h"
#include "util.h"

#define N_BUTTONS 5

#define OCTO_BUTTON     0
#define SQUID_BUTTON    1
#define UNKNOWN_BUTTON  2
#define CUDA_BUTTON     3
#define QUESTION_BUTTON 4
#define EEL_BUTTON	5

#define N_GPIOS   6

#define OCTO_GPIO     0
#define SQUID_GPIO    1
#define UNKNOWN_GPIO  2
#define CUDA_GPIO     3
#define QUESTION_GPIO 4
#define EEL_GPIO      5

#define OCTO        "octo"
#define SQUID       "squid"
#define UNKNOWN     "???"
#define CUDA        "cuda"
#define QUESTION    "question"
#define EEL	    "eel"

#define SQUID_MS	5000
#define CUDA_MS	        5000
#define QUESTION_MS	5000
#define BUTTON_LOCKOUT_MS 1000

static gpio_table_t gpio_table[N_GPIOS] = {
    [OCTO_GPIO]        = { OCTO,       26, 0 },
    [SQUID_GPIO]       = { SQUID,      19, 0 },
    [UNKNOWN_GPIO]     = { UNKNOWN,    16, 0 },
    [CUDA_GPIO]        = { CUDA,       6, 0 },
    [QUESTION_GPIO]    = { QUESTION,   13, 0 },
    [EEL_GPIO]         = { EEL,        5, 0 },
};

#define N_GPIO_TABLE (sizeof(gpio_table) / sizeof(gpio_table[0]))

static lights_t *lights;
static piface_t *piface;
static gpio_t *gpio;
static pthread_mutex_t event_lock, eel_lock;
static track_t *laugh;

static void
do_popup(unsigned id, unsigned ms)
{
printf("popup on %d\n", id);
    gpio_on_id(gpio, id);
    ms_sleep(ms);
    gpio_off_id(gpio, id);
}

static void
do_attack(unsigned id)
{
    unsigned i;

printf("attack on %d\n", id);
    for (i = 0; i < 3; i++) {
	gpio_on_id(gpio, id);
	ms_sleep(500 + random_number_in_range(0, 250) - 125);
 	gpio_off_id(gpio, id);
	ms_sleep(200 + random_number_in_range(0, 100) - 50);
    }
}

static void
do_question(void)
{
    track_play_asynchronously(laugh);
    lights_blink(lights);
    do_popup(QUESTION_GPIO, QUESTION_MS);
}

static void
do_prop(unsigned id)
{
    switch (id) {
    case OCTO_BUTTON: do_attack(OCTO_GPIO); break;
    case SQUID_BUTTON: do_popup(SQUID_GPIO, SQUID_MS); break;
    case UNKNOWN_BUTTON:  do_attack(UNKNOWN_GPIO); break;
    case CUDA_BUTTON:  do_popup(CUDA_GPIO, CUDA_MS); break;
    case QUESTION_BUTTON: do_question(); break;
    case EEL_BUTTON: do_attack(EEL_GPIO); break;
    }
}

static void
handle_event_locked(unsigned i)
{
    lights_select(lights, i);
    do_prop(i);
}

static void
handle_event(unsigned i)
{
    pthread_mutex_lock(&event_lock);
    handle_event_locked(i);
    pthread_mutex_unlock(&event_lock);
}

static void
handle_eel(void)
{
    pthread_mutex_lock(&eel_lock);
    do_prop(EEL_BUTTON);
    pthread_mutex_unlock(&eel_lock);
}

static char *
remote_event_locked(const char *command)
{
    if (strcmp(command, OCTO) == 0) {
	handle_event_locked(OCTO_BUTTON);
    } else if (strcmp(command, SQUID) == 0) {
	handle_event_locked(SQUID_BUTTON);
    } else if (strcmp(command, UNKNOWN) == 0) {
	handle_event_locked(UNKNOWN_BUTTON);
    } else if (strcmp(command, CUDA) == 0) {
	handle_event_locked(CUDA_BUTTON);
    } else if (strcmp(command, "question") == 0) {
	handle_event_locked(QUESTION_BUTTON);
    } else if (strcmp(command, EEL) == 0) {
	do_prop(EEL_BUTTON);
    } else {
	fprintf(stderr, "Invalid net command: [%s]\n", command);
	return strdup("Invalid command\n");
    }
    return strdup(SERVER_OK);
}

static char *
remote_event(void *unused, const char *command)
{
    char *result;
    pthread_mutex_t *lock = &event_lock;
    int is_eel = strcmp(command, EEL) == 0;

    if (is_eel) {
	lock = &eel_lock;
    }

    if (pthread_mutex_trylock(lock) != 0) {
	return strdup("prop is busy");
    }
    result = remote_event_locked(command);
    pthread_mutex_unlock(lock);

    if (! is_eel) {
	lights_chase(lights);
    }

    return result;
}

static void
wait_for_no_buttons(void)
{
    for (;;) {
	unsigned buttons = piface_get_all(piface);
	if (PIFACE_NONE_SELECTED(buttons)) return;
    }
}

int
main(int argc, char **argv)
{
    unsigned i, button;
    server_args_t server_args;
    pthread_t server_thread;

    seed_random();

    if ((laugh = track_new("laugh.wav")) == NULL) {
	perror("laugh.wav");
	exit(1);
    }

    piface = piface_new();
    lights = lights_new(piface);
    gpio = gpio_new(gpio_table, N_GPIO_TABLE);

    pthread_mutex_init(&event_lock, NULL);
    pthread_mutex_init(&eel_lock, NULL);

    server_args.port = 5555;
    server_args.command = remote_event;
    server_args.state = NULL;

    lights_chase(lights);

    pthread_create(&server_thread, NULL, server_thread_main, &server_args);

    while (true) {
	button = piface_wait_for_input(piface);
	for (i = 0; i < N_BUTTONS; i++) {
	    if (PIFACE_IS_SELECTED(button, i)) {
		handle_event(i);
		wait_for_no_buttons();
		lights_off(lights);
		ms_sleep(BUTTON_LOCKOUT_MS);
		lights_chase(lights);
		break;
	    }
	    if (PIFACE_IS_SELECTED(button, EEL_BUTTON)) {
		handle_eel();
	    }
	}
    }

    pthread_mutex_destroy(&event_lock);
    pthread_mutex_destroy(&eel_lock);
    lights_destroy(lights);
    piface_destroy(piface);

    return 0;
}
