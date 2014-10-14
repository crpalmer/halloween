#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "piface.h"
#include "server.h"
#include "track.h"
#include "util.h"

#define BURP_WAV	"burp.wav"

#define BUTTON_INPUT	0
#define BUTTON_LED	7
#define PUMP_OUTPUT	0

#define BURP_PROB	0.4
#define RECHARGE_SECONDS 3
#define PUMP_MS		200
#define MAX_CONSECUTIVE_SPITS 4

static piface_t *p;
static unsigned n_consecutive_spits = 0;
static pthread_mutex_t event_lock;

static track_t *burp_track;

static void
wait_for_trigger()
{
    piface_set(p, BUTTON_LED, true);
    printf("Waiting for button %d: \n", BUTTON_INPUT); fflush(stdout);

    for (;;) {
        unsigned button = piface_wait_for_input(p);
	if (PIFACE_IS_SELECTED(button, BUTTON_INPUT)) break;
    }

    piface_set(p, BUTTON_LED, false);
}

static void
wait_for_trigger_release()
{
    while (piface_get(p, BUTTON_INPUT)) {}
}

static void
burp()
{
    printf("burp.\n");
    track_play(burp_track);
    n_consecutive_spits = 0;
}

static void
wait_for_recharge()
{
    printf("Waiting to recharge\n");
    sleep(RECHARGE_SECONDS);
}

static void
spit()
{
    printf("spit.\n");
    piface_set(p, PUMP_OUTPUT, true);
    ms_sleep(PUMP_MS);
    piface_set(p, PUMP_OUTPUT, false);

    n_consecutive_spits++;
}

static void
take_action()
{
    pthread_mutex_lock(&event_lock);

    if (randomly_with_prob(BURP_PROB)) {
	if (! n_consecutive_spits) {
	    printf("Too many burps, do a spit instead.\n");
	    spit();
	} else {
	    burp();
	}
    } else {
	if (n_consecutive_spits >= MAX_CONSECUTIVE_SPITS) {
	    printf("Too many spits, do a burp instead.\n");
	    burp();
	} else {
	    spit();
	}
    }

    pthread_mutex_unlock(&event_lock);
}

static char *
remote_event_locked(const char *command)
{
    if (strcmp(command, "burp") == 0) {
	burp();
    } else if (strcmp(command, "spit") == 0) {
	spit();
    } else {
	return strdup("Invalid command");
    }

    return strdup(SERVER_OK);
}

static char *
remote_event(void *unused, const char *command)
{
    char *response;

    pthread_mutex_lock(&event_lock);
    response = remote_event_locked(command);
    pthread_mutex_unlock(&event_lock);

    return response;
}


int
main(int argc, char **argv)
{
    server_args_t server_args;
    pthread_t server_thread;

    seed_random();

    burp_track = track_new(BURP_WAV);
    if (! burp_track) {
	perror(BURP_WAV);
	exit(1);
    }

    p = piface_new();

    pthread_mutex_init(&event_lock, NULL);
    server_args.port = 5555;
    server_args.command = remote_event;
    server_args.state = NULL;

    pthread_create(&server_thread, NULL, server_thread_main, &server_args);

    while(true) {
	wait_for_trigger();
	take_action();
	wait_for_trigger_release();
	if (n_consecutive_spits) {
	    wait_for_recharge();
	}
    }

    return 0;
}
