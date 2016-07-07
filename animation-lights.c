#include <stdio.h>
#include <pthread.h>
#include <malloc.h>
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
    unsigned	     min_pin, max_pin;
};


#define ANIMATION_SLEEP_MS	200

static void
set_all(lights_t *l, unsigned value)
{
    unsigned pin;

    for (pin = l->min_pin; pin <= l->max_pin; pin++) {
	wb_set(LIGHTS_OUTPUT_BANK, pin, value);
    }
}

static void
blink_locked(lights_t *l)
{
    set_all(l, 1);
    pthread_mutex_unlock(&l->lock);
    ms_sleep(ANIMATION_SLEEP_MS);
    pthread_mutex_lock(&l->lock);
    if (l->action == LIGHTS_BLINK) {
	set_all(l, 0);
	ms_sleep(ANIMATION_SLEEP_MS);
    }
}

static void
chase_locked(lights_t *l)
{
    unsigned pin;

    for (pin = l->min_pin; pin <= l->max_pin && l->action == LIGHTS_CHASE; pin++) {
	set_all(l, 0);
	wb_set(LIGHTS_OUTPUT_BANK, pin, 1);
	if (l->max_pin - l->min_pin + 1 >= 3) {
	    /* If we only have 2 lights, only light up one at a time */
	    wb_set(LIGHTS_OUTPUT_BANK, pin < l->max_pin ? pin+1 : l->min_pin, 1);
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
    lights_t *l = fatal_malloc(sizeof(*l));

    l->action = LIGHTS_NONE;
    l->min_pin = min_pin;
    l->max_pin = max_pin;

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
    if (l->min_pin == l->max_pin) l->action = LIGHTS_BLINK;
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
    wb_set(LIGHTS_OUTPUT_BANK, selected, 1);
    l->action = LIGHTS_NONE;
    pthread_mutex_unlock(&l->lock);
}

void
lights_blink(lights_t *l)
{
    pthread_mutex_lock(&l->lock);
    set_all(l, 0);
    l->action = LIGHTS_BLINK;
    pthread_cond_signal(&l->cond);
    pthread_mutex_unlock(&l->lock);
}
