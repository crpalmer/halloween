#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "piface.h"
#include "track.h"
#include "util.h"

#define DUTY_FILENAME "swamp.duty"
#define DEFAULT_DUTY_CYCLE .10

#define FOG_OUTPUT	7
#define OFF_DELAY	5000

static piface_t *p;
static double duty = .10;

static double
read_duty_cycle(void)
{
    FILE *f = fopen(DUTY_FILENAME, "r");
    double x;

    if (f == NULL) {
	perror(DUTY_FILENAME);
	return DEFAULT_DUTY_CYCLE;
    }

    if (fscanf(f, "%lf", &x) != 1) {
	fprintf(stderr, "Failed to ready duty cycle.\n");
	return DEFAULT_DUTY_CYCLE;
    }

    fclose(f);

    return x;
}

static double
write_duty_cycle(double x)
{
    FILE *f = fopen(DUTY_FILENAME, "w");

    if (f == NULL) {
	perror(DUTY_FILENAME);
	exit(1);
    }

    fprintf(f, "%f", x);
    fclose(f);

    return x;
}

int
main(int argc, char **argv)
{
    p = piface_new();
    duty = read_duty_cycle();

    write_duty_cycle(duty);

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
