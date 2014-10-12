#include <stdio.h>
#include <pthread.h>
#include <malloc.h>
#include "mem.h"
#include "util.h"

#include "lights.h"

typedef enum { NONE, CHASE, ON, SELECTED, EXIT } action_t;

struct lightsS {
    gpio_table_t *gpios;
    size_t       n_gpios;
    gpio_t	*gpio;

    pthread_t    thread;
    pthread_mutex_t lock;
    pthread_cond_t  cond;

    action_t     action;
    char        *selected;
    size_t	 next_chase;
};

#define CHASE_SLEEP_MS	100

static void
set_all_off(lights_t *lights)
{
    size_t i;

    for (i = 0; i < lights->n_gpios; i++) {
	gpio_off_id(lights->gpio, i);
    }
}

static void
set_all_on(lights_t *lights)
{
    size_t i;

    for (i = 0; i < lights->n_gpios; i++) {
	gpio_on_id(lights->gpio, i);
    }
}

static void
do_chase_step(lights_t *lights)
{
    set_all_off(lights);
    gpio_on_id(lights->gpio, lights->next_chase);
    lights->next_chase = (lights->next_chase + 1) % lights->n_gpios;
}

static void
do_selected(lights_t *lights)
{
    set_all_off(lights);
    gpio_on(lights->gpio, lights->selected);
    free(lights->selected);
    lights->selected = NULL;
}

static void *
lights_work(void *lights_as_vp)
{
    lights_t *lights = (lights_t *) lights_as_vp;

    for (;;) {
	pthread_mutex_lock(&lights->lock);
	while (lights->action == NONE) {
	    pthread_cond_wait(&lights->cond, &lights->lock);
	}
	switch (lights->action) {
	case NONE:
	    break;
	case CHASE:
	    do_chase_step(lights);
	    break;
	case ON:
	    set_all_on(lights);
	    lights->action = NONE;
	    break;
	case SELECTED:
	    do_selected(lights);
	    lights->action = NONE;
	    break;
	case EXIT:
	    return NULL;
	}
	pthread_mutex_unlock(&lights->lock);
	if (lights->action == CHASE) ms_sleep(CHASE_SLEEP_MS);
    }
}

lights_t *
lights_new(gpio_table_t *gpios, size_t n_gpios)
{
    lights_t *lights = fatal_malloc(sizeof(*lights));

    lights->gpios = gpios;
    lights->n_gpios = n_gpios;
    lights->gpio = gpio_new(lights->gpios, lights->n_gpios);

    lights->action = NONE;
    lights->selected = NULL;
    lights->next_chase = 0;

    pthread_mutex_init(&lights->lock, NULL);
    pthread_cond_init(&lights->cond, NULL);

    pthread_create(&lights->thread, NULL, lights_work, lights);

    return lights;
}

static void
send_work(lights_t *lights, action_t action, const char *selected)
{
    pthread_mutex_lock(&lights->lock);
    /* Once exit is picked, never override it! */
    if (lights->action != EXIT) {
        lights->action = action;
        if (lights->selected) free(lights->selected);
        lights->selected = fatal_strdup(selected);
    }
    pthread_cond_signal(&lights->cond);
    pthread_mutex_unlock(&lights->lock);
}

void
lights_chase(lights_t *lights)
{
    send_work(lights, CHASE, NULL);
}

void
lights_on(lights_t *lights)
{
    send_work(lights, ON, NULL);
}

void
lights_select(lights_t *lights, const char *name)
{
    send_work(lights, SELECTED, name);
}

void
lights_destroy(lights_t *lights)
{
    send_work(lights, EXIT, NULL);
    pthread_join(lights->thread, NULL);

    pthread_mutex_destroy(&lights->lock);
    pthread_cond_destroy(&lights->cond);
    gpio_destroy(lights->gpio);
    free(lights);
}
