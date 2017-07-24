#include <stdio.h>
#include <pthread.h>
#include <malloc.h>
#include <memory.h>
#include "mem.h"
#include "util.h"
#include "wb.h"

#include "animation-common.h"
#include "animation-lights.h"

typedef enum {
    LIGHTS_NONE, LIGHTS_BLINK, LIGHTS_CHASE
} lights_action_t;

struct lightsS {
    pthread_t        thread;
    pthread_mutex_t  lock;
    pthread_cond_t   cond;
    lights_action_t  action;
    unsigned	     blink_pin;
    struct {
	int  bank;
	int pin;
    }		    *map;
    int		     n;
};


#define ANIMATION_SLEEP_MS	200

static void
set(lights_t *l, int i, unsigned value)
{
fprintf(stderr, "SET %d [%d %d] =  %d\n", i, l->map[i].bank, l->map[i].pin, value);
    wb_set(l->map[i].bank, l->map[i].pin, value);
}

static void
set_all(lights_t *l, unsigned value)
{
    unsigned i;

    for (i = 0; i < l->n; i++) {
	set(l, i, value);
    }
}

static void
blink_locked(lights_t *l)
{
    if (l->blink_pin == -1) set_all(l, 1);
    else set(l, l->blink_pin, 1);

    pthread_mutex_unlock(&l->lock);
    ms_sleep(ANIMATION_SLEEP_MS);
    pthread_mutex_lock(&l->lock);
    if (l->action == LIGHTS_BLINK) {
	if (l->blink_pin == -1) set_all(l, 0);
	else set(l, l->blink_pin, 0);

	ms_sleep(ANIMATION_SLEEP_MS);
    }
}

static void
chase_locked(lights_t *l)
{
    unsigned i;

    for (i = 0; i < l->n && l->action == LIGHTS_CHASE; i++) {
	set_all(l, 0);
	set(l, i, 1);
	if (l->n >= 3) {
	    /* If we only have 2 lights, only light up one at a time */
	    set(l, (i + 1) % l->n, 1);
	}
	pthread_mutex_unlock(&l->lock);
	ms_sleep(ANIMATION_SLEEP_MS);
	pthread_mutex_lock(&l->lock);
    }
}

static void *
lights_work(void *l_as_vp)
{
    lights_t *l = (lights_t *) l_as_vp;

    while (true) {
	pthread_mutex_lock(&l->lock);
	if (l->action == LIGHTS_NONE) {
	    pthread_cond_wait(&l->cond, &l->lock);
	} else if (l->action == LIGHTS_BLINK) {
	    blink_locked(l);
	} else if (l->action == LIGHTS_CHASE) {
	    chase_locked(l);
	}
	pthread_mutex_unlock(&l->lock);
    }
    return NULL;
}

lights_t *
lights_new(unsigned min_pin, unsigned max_pin)
{
    int n = max_pin - min_pin + 1;
    lights_map_t map[n];
    int i;

    for (i = 0; i < n; i++) {
	map[i].bank = LIGHTS_OUTPUT_BANK;
	map[i].pin = min_pin + 1;
    }

    return lights_new_map(map, n);
}

lights_t *
lights_new_map(const lights_map_t *map, int n)
{
    lights_t *l = fatal_malloc(sizeof(*l));

    l->action = LIGHTS_NONE;
    l->blink_pin = -1;
    l->map = fatal_malloc(sizeof(*l->map) * n);
    memcpy(l->map, map, sizeof(*l->map) * n);
    l->n = n;

    pthread_mutex_init(&l->lock, NULL);
    pthread_cond_init(&l->cond, NULL);

    pthread_create(&l->thread, NULL, lights_work, l);

    return l;
}
    
void
lights_chase(lights_t *l)
{
    pthread_mutex_lock(&l->lock);
    set_all(l, 0);
    if (l->n <= 1) l->action = LIGHTS_BLINK;
    else l->action = LIGHTS_CHASE;
    pthread_cond_signal(&l->cond);
    pthread_mutex_unlock(&l->lock);
} 
void
lights_on(lights_t *l)
{
    pthread_mutex_lock(&l->lock);
    set_all(l, 1);
    l->action = LIGHTS_NONE;
    pthread_mutex_unlock(&l->lock);
}

void
lights_off(lights_t *l)
{
    pthread_mutex_lock(&l->lock);
    set_all(l, 0);
    l->action = LIGHTS_NONE;
    pthread_mutex_unlock(&l->lock);
}

void
lights_select(lights_t *l, unsigned selected)
{
    pthread_mutex_lock(&l->lock);
    set_all(l, 0);
    set(l, selected, 1);
    l->action = LIGHTS_NONE;
    pthread_mutex_unlock(&l->lock);
}

void
lights_blink(lights_t *l)
{
    pthread_mutex_lock(&l->lock);
    set_all(l, 0);
    l->blink_pin = -1;
    l->action = LIGHTS_BLINK;
    pthread_cond_signal(&l->cond);
    pthread_mutex_unlock(&l->lock);
}

void
lights_blink_one(lights_t *l, unsigned pin)
{
    pthread_mutex_lock(&l->lock);
    set_all(l, 0);
    l->blink_pin = pin;
    l->action = LIGHTS_BLINK;
    pthread_cond_signal(&l->cond);
    pthread_mutex_unlock(&l->lock);
}
