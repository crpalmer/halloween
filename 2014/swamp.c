#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "piface.h"
#include "track.h"
#include "util.h"

#define FOG_OUTPUT	7
#define OFF_DELAY	5000

static piface_t *p;
static double duty = .10;

int
main(int argc, char **argv)
{
    p = piface_new();

    while(true) {
	fprintf(stderr, "sleeping for OFF\n");
	ms_sleep(5000 * (1 - duty));
	piface_set(p, FOG_OUTPUT, 1);
	fprintf(stderr, "sleeping for ON\n");
	ms_sleep(5000 * (duty));
	piface_set(p, FOG_OUTPUT, 0);
	fprintf(stderr, "sleeping for OFF_DELAY\n");
	ms_sleep(OFF_DELAY);
    }

    return 0;
}
