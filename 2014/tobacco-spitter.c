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

#define BURP_PROB	0.2
#define RECHARGE_SECONDS 5
#define PUMP_MS		500

static piface_t *p;
static bool last_was_burp = false;

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
burp()
{
    printf("burp.\n");
    track_play(burp_track);
    last_was_burp = true;
}

static void
wait_for_recharge()
{
    while (piface_get(p, BUTTON_INPUT)) {}
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

    wait_for_recharge();

    last_was_burp = false;
}

static void
take_action()
{
    if (randomly_with_prob(BURP_PROB)) {
	if (last_was_burp) {
	    printf("Too many burps, do a spit instead.\n");
	    spit();
	} else {
	    burp();
	}
    } else {
	spit();
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
    }

    return 0;
}
