#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "track.h"
#include "util.h"
#include "wb.h"

#include "animation-actions.h"
#include "animation-common.h"
#include "animation-lights.h"

#define OCTO        "octo"
#define SQUID       "squid"
#define DIVER       "diver"
#define CUDA        "cuda"
#define QUESTION    "question"
#define EEL	    "eel"

#define CUDA_MS	        1000
#define QUESTION_MS	2000
#define DIVER_MS	2000

static pthread_mutex_t station_lock, eel_lock;
static track_t *laugh;

static void
do_popup(void *ms_as_vp, unsigned pin)
{
    unsigned ms = (unsigned) ms_as_vp;

    wb_set(ANIMATION_OUTPUT_BANK, pin, 1);
    ms_sleep(ms);
    wb_set(ANIMATION_OUTPUT_BANK, pin, 0);
}

static void
do_attack(unsigned pin, double up, double down)
{
    unsigned i;

    for (i = 0; i < 3; i++) {
	wb_set(ANIMATION_OUTPUT_BANK, pin, 1);
	ms_sleep((500 + random_number_in_range(0, 250) - 125)*up);
	wb_set(ANIMATION_OUTPUT_BANK, pin, 0);
	ms_sleep((200 + random_number_in_range(0, 100) - 50)*down);
    }
}

static void
do_question(void *unused, unsigned pin)
{
    track_play_asynchronously(laugh, NULL);
    lights_blink();
    do_popup((void *) QUESTION_MS, pin);
}

static void
do_squid(void *unused, unsigned pin)
{
    do_attack(pin, 1, 1.75);
}

static void
do_octo(void *unused, unsigned pin)
{
    do_attack(pin, 1, 1);
}

static void
do_eel(void *unused, unsigned pin)
{
    do_attack(pin, 1, 3);
}

action_t actions[] = {
    { NULL,	NULL,		NULL,			NULL },
    { SQUID,	do_squid,	NULL,			&station_lock },
    { CUDA,	do_popup,	(void *) CUDA_MS,	&station_lock },
    { DIVER,	do_popup,	(void *) DIVER_MS,	&station_lock },
    { OCTO,	do_octo,	NULL,			&station_lock },
    { QUESTION, do_question,	NULL,			&station_lock },
    { EEL,	do_eel,		NULL,			&eel_lock },
    { NULL,	NULL,		NULL,			NULL },
};
    
void
actions_init(void)
{
    pthread_mutex_init(&station_lock, NULL);
    pthread_mutex_init(&eel_lock, NULL);

    if ((laugh = track_new("laugh.wav")) == NULL) {
	perror("laugh.wav");
	exit(1);
    }
}
