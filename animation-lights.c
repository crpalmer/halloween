#include <stdio.h>
#include <pthread.h>
#include <malloc.h>
#include "mem.h"
#include "util.h"
#include "wb.h"

#include "animation-common.h"

static pthread_t    	thread;
static pthread_mutex_t  lock;
static pthread_cond_t   cond;

static enum {
    LIGHTS_NONE, LIGHTS_BLINK, LIGHTS_CHASE
} action = LIGHTS_NONE;

#define ANIMATION_SLEEP_MS	200

static void
set_all(unsigned value)
{
    unsigned pin;

    for (pin = 1; pin <= 8; pin++) {
	wb_set(LIGHTS_OUTPUT_BANK, pin, value);
    }
}

static void
blink_locked(void)
{
    set_all(1);
    pthread_mutex_unlock(&lock);
    ms_sleep(ANIMATION_SLEEP_MS);
    pthread_mutex_lock(&lock);
    if (action == LIGHTS_BLINK) {
	set_all(0);
	ms_sleep(ANIMATION_SLEEP_MS);
    }
}

static void
chase_locked(void)
{
    unsigned pin;

    for (pin = 1; pin <= N_STATION_BUTTONS && action == LIGHTS_CHASE; pin++) {
	set_all(0);
	wb_set(LIGHTS_OUTPUT_BANK, pin, 1);
	wb_set(LIGHTS_OUTPUT_BANK, pin < N_STATION_BUTTONS ? pin+1 : 1, 1);
	pthread_mutex_unlock(&lock);
	ms_sleep(ANIMATION_SLEEP_MS);
	pthread_mutex_lock(&lock);
    }
}

static void *
lights_work(void *unused)
{
    while (true) {
	pthread_mutex_lock(&lock);
	if (action == LIGHTS_NONE) {
	    pthread_cond_wait(&cond, &lock);
	} else if (action == LIGHTS_BLINK) {
	    blink_locked();
	} else if (action == LIGHTS_CHASE) {
	    chase_locked();
	}
	pthread_mutex_unlock(&lock);
    }
    return NULL;
}

void
lights_init(void)
{
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);

    pthread_create(&thread, NULL, lights_work, NULL);
}
    
void
lights_chase(void)
{
    pthread_mutex_lock(&lock);
    set_all(0);
    action = LIGHTS_CHASE;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&lock);
}

void
lights_on(void)
{
    pthread_mutex_lock(&lock);
    set_all(1);
    action = LIGHTS_NONE;
    pthread_mutex_unlock(&lock);
}

void
lights_off(void)
{
    pthread_mutex_lock(&lock);
    set_all(0);
    action = LIGHTS_NONE;
    pthread_mutex_unlock(&lock);
}

void
lights_select(unsigned selected)
{
    pthread_mutex_lock(&lock);
    set_all(0);
    wb_set(LIGHTS_OUTPUT_BANK, selected, 1);
    action = LIGHTS_NONE;
    pthread_mutex_unlock(&lock);
}

void
lights_blink(void)
{
    pthread_mutex_lock(&lock);
    set_all(0);
    action = LIGHTS_BLINK;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&lock);
}
