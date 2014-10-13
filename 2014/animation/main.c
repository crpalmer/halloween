#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "gpio.h"
#include "piface.h"
#include "server.h"
#include "util.h"
#include "animation/lights.h"

#define N_BUTTONS 5

#define SPIDER_BUTTON   0
#define ZOMBIE_BUTTON   1
#define GATER_BUTTON    2
#define SNAKE_BUTTON    3
#define QUESTION_BUTTON 4

#define N_GPIOS   6

#define SPIDER_GPIO   0
#define ZOMBIE_GPIO   1
#define GATER_GPIO    2
#define SNAKE_GPIO    3
#define SNAKE_SPIT_GPIO 4
#define QUESTION_GPIO 5

#define SPIDER      "spider"
#define ZOMBIE      "zombie"
#define GATER       "gater"
#define SNAKE       "snake"
#define SNAKE_SPIT  "snake-spit"
#define QUESTION    "question"

#define ZOMBIE_MS	5000
#define SPIDER_MS	5000
#define QUESTION_MS	5000
#define BUTTON_LOCKOUT_MS 1000

static gpio_table_t gpio_table[N_GPIOS] = {
    [SPIDER_GPIO]      = { SPIDER,     26, 0 },
    [ZOMBIE_GPIO]      = { ZOMBIE,     19, 0 },
    [GATER_GPIO]       = { GATER,      16, 0 },
    [SNAKE_GPIO]       = { SNAKE,       6, 0 },
    [SNAKE_SPIT_GPIO]  = { SNAKE_SPIT,  5, 0 },
    [QUESTION_GPIO]    = { QUESTION,   13, 0 },
};

#define N_GPIO_TABLE (sizeof(gpio_table) / sizeof(gpio_table[0]))

static lights_t *lights;
static piface_t *piface;
static gpio_t *gpio;
static pthread_mutex_t event_lock;

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

    for (i = 0; i < 3; i++) {
	gpio_on_id(gpio, id);
	ms_sleep(500 + random_number_in_range(0, 250) - 125);
 	gpio_off_id(gpio, id);
	ms_sleep(200 + random_number_in_range(0, 100) - 50);
    }
}

static void
do_snake(void)
{
     gpio_on_id(gpio, SNAKE_SPIT_GPIO);
     do_attack(SNAKE_GPIO);
     gpio_off_id(gpio, SNAKE_SPIT_GPIO);
}

static void
do_prop(unsigned id)
{
    switch (id) {
    case SPIDER_BUTTON: do_attack(SPIDER_GPIO); break;
    case ZOMBIE_BUTTON: do_popup(ZOMBIE_GPIO, ZOMBIE_MS); break;
    case GATER_BUTTON:  do_attack(GATER_GPIO); break;
    case SNAKE_BUTTON:  do_snake(); break;
    case QUESTION_BUTTON: do_popup(QUESTION_GPIO, QUESTION_MS); break;
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

static char *
remote_event_locked(const char *command)
{
    if (strcmp(command, SPIDER) == 0) {
	handle_event_locked(SPIDER_BUTTON);
    } else if (strcmp(command, ZOMBIE) == 0) {
	handle_event_locked(ZOMBIE_BUTTON);
    } else if (strcmp(command, GATER) == 0) {
	handle_event_locked(GATER_BUTTON);
    } else if (strcmp(command, SNAKE) == 0) {
	handle_event_locked(SNAKE_BUTTON);
    } else if (strcmp(command, "question") == 0) {
	handle_event_locked(QUESTION_BUTTON);
    } else {
	fprintf(stderr, "Invalid net command: [%s]\n", command);
	return strdup("Invalid command\n");
    }
    return strdup("ok");
}

static char *
remote_event(void *unused, const char *command)
{
    char *result;

    if (pthread_mutex_trylock(&event_lock) < 0) {
	return strdup("prop is busy");
    }
    result = remote_event_locked(command);
    pthread_mutex_unlock(&event_lock);

    lights_chase(lights);

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

    piface = piface_new();
    lights = lights_new(piface);
    gpio = gpio_new(gpio_table, N_GPIO_TABLE);

    pthread_mutex_init(&event_lock, NULL);

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
	}
    }

    pthread_mutex_destroy(&event_lock);
    lights_destroy(lights);
    piface_destroy(piface);

    return 0;
}
