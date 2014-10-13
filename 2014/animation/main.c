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
    gpio_off_id(gpio, id);
    ms_sleep(ms);
    gpio_on_id(gpio, id);
}

static void
do_attack(unsigned id)
{
#if 0
    unsigned i;

printf("attacking on %d\n", id);
    for (i = 0; i < 10; i++) {
	gpio_on_id(gpio, 0);
	ms_sleep(200);
 	gpio_off_id(gpio, 0);
	ms_sleep(200);
    }
#endif
do_popup(id, 5000);
}

static void
do_snake(void)
{
printf("snake\n");
do_popup(SNAKE_GPIO, 5000);
do_popup(SNAKE_SPIT_GPIO, 5000);
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
ms_sleep(2500);
    lights_chase(lights);
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

    return result;
}

int
main(int argc, char **argv)
{
    unsigned i, button;
    server_args_t server_args;
    pthread_t server_thread;

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
	    }
	}
    }

    pthread_mutex_destroy(&event_lock);
    lights_destroy(lights);
    piface_destroy(piface);

    return 0;
}
