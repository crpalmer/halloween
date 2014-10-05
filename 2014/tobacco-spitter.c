#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "piface.h"
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

static track_t *burp_track;

static void
wait_for_trigger()
{
    piface_set(p, BUTTON_LED, true);
    printf("Waiting for button %d: \n", BUTTON_INPUT); fflush(stdout);

    while (! piface_get(p, BUTTON_INPUT)) {}
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
}

int
main(int argc, char **argv)
{
    seed_random();

    burp_track = track_new(BURP_WAV);
    if (! burp_track) {
	perror(BURP_WAV);
	exit(1);
    }

    p = piface_new();

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
