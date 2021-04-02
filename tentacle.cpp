#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include "maestro.h"
#include "pi-usb.h"
#include "track.h"
#include "util.h"

maestro_t *m;

#define MID_POS 50
#define RANGE   50
#define MIN_POS (MID_POS - RANGE)
#define MAX_POS (MID_POS + RANGE)

static void
do_random()
{
    double pos[2] = { 50, 50 };

    while (1) {
	for (int i = 0; i < 2; i++) {
	    double old_pos = pos[i];
	    while (fabs(old_pos - pos[i]) < 20) pos[i] = random_number_in_range(MIN_POS, MAX_POS);
	    maestro_set_servo_pos(m, i, pos[i]);
	}
        ms_sleep(300);
    }
}

static void
do_stirring()
{
    while (1) {
	for (int deg = 0; deg < 360; deg += 1) {
	    double rad = deg * M_PI / 180.0;
	    double p0 = RANGE*cos(rad) + MID_POS;
	    double p1 = RANGE*sin(rad) + MID_POS;

	    maestro_set_servo_pos(m, 0, p0);
	    maestro_set_servo_pos(m, 1, p1);
	    ms_sleep(3);
	}
    }
}

static void
do_loopey()
{
     int speed = 1000;
     unsigned s_ms;

     s_ms = maestro_set_servo_speed(m, 0, speed);
     maestro_set_servo_speed(m, 1, speed);
     while (1) {
	for (int servo = 0; servo < 2; servo++) {
	    maestro_set_servo_pos(m, servo, MIN_POS);
	    ms_sleep(s_ms/2);
	    maestro_set_servo_pos(m, servo, MAX_POS);
	    ms_sleep(s_ms);
	    maestro_set_servo_pos(m, servo, MID_POS);
	    ms_sleep(s_ms/2);
	}
     }
}
	

int
main(int argc, char **argv)
{
    const char *arg = argc == 2 ? argv[1] : "";

    pi_usb_init();

    seed_random();

    if ((m = maestro_new()) == NULL) {
	fprintf(stderr, "Failed to initialize servo controller\n");
	exit(1);
    }

    maestro_set_servo_range(m, 0, SERVO_DS3218);
    maestro_set_servo_range_pct(m, 0, 25, 75);
    maestro_set_servo_range(m, 1, SERVO_DS3218);
    maestro_set_servo_range_pct(m, 1, 39, 69);
    maestro_set_servo_pos(m, 0, MID_POS);
    maestro_set_servo_pos(m, 1, MID_POS);

    if (strcmp(arg, "stir") == 0) {
	do_stirring();
    } else if (strcmp(arg, "loopey") == 0) {
	do_loopey();
    } else if (strcmp(arg, "reset") == 0) {
	// do nothing
    } else {
	do_random();
    }

    return 0;
}
