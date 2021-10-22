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

typedef struct {
     int	servo;
     int	mid;
} tentacle_servo_t;

typedef struct {
     const char *name;
     tentacle_servo_t left, right;
     int	delta;
     int	angle_delta;
} tentacle_t;

static tentacle_t tentacles[2] = {
    {
	"upper", { 0, 33}, {1, 43}, 15,  180
    },
    {
	"lower", { 3, 53}, {2, 48}, 15, -30
    }
};

static void
tentacle_goto(tentacle_t *t, int deg, double magnitude)
{
    deg += t->angle_delta;
    while (deg < 0) deg += 360;

    double rad = (deg % 90) * M_PI / 180.0;
    double c = cos(rad);
    double s = sin(rad);
    double p0 = 0, p1 = 0;

    switch ((deg % 360) / 90) {
    case 0: p0 = c; p1 = s; break;
    case 1: p0 = -s; p1 = c; break;
    case 2: p0 = -c; p1 = -s; break;
    case 3: p0 = s; p1 = -c; break;
    }

    p0 = 50 * magnitude * p0 + 50;
    p1 = 50 * magnitude * p1 + 50;

fprintf(stderr, "%s %d => %5.2f %5.2f\n", t->name, deg, p0, p1);

    maestro_set_servo_pos(m, t->left.servo, p0);
    maestro_set_servo_pos(m, t->right.servo, p1);
}

static void
tentacle_servo_init(tentacle_servo_t *s, int delta)
{
    maestro_set_servo_range(m, s->servo, SERVO_DS3218);
    maestro_set_servo_range_pct(m, s->servo, s->mid - delta, s->mid + delta);
    maestro_set_servo_pos(m, s->servo, 50);
}

static void
do_random(tentacle_t *t)
{
    while (1) {
	for (tentacle_t &t : tentacles) {
	    int deg = random_number_in_range(0, 359);
	    double magnitude = random_number_in_range(1, 100) / 100.0;
	    tentacle_goto(&t, deg, magnitude);
	}
        ms_sleep(500);
    }
}

static void
do_stirring()
{
    while (1) {
	for (int deg = 0; deg < 360; deg += 2) {
	    int delta = 0;
	    for (tentacle_t &t : tentacles) {
		tentacle_goto(&t, deg + delta, 1);
		delta += 90;
	    }
	    ms_sleep(20);
	}
    }
}

static void
do_loopey(tentacle_t *t)
{
     int speed = 1000;
     unsigned s_ms;

     s_ms = maestro_set_servo_speed(m, t->left.servo, speed);
     maestro_set_servo_speed(m, t->right.servo, speed);
     while (1) {
	maestro_set_servo_pos(m, t->left.servo, 0);
	ms_sleep(s_ms/2);
	maestro_set_servo_pos(m, t->left.servo, 100);
	ms_sleep(s_ms);
	maestro_set_servo_pos(m, t->left.servo, 50);
	ms_sleep(s_ms/2);

	maestro_set_servo_pos(m, t->right.servo, 0);
	ms_sleep(s_ms/2);
	maestro_set_servo_pos(m, t->right.servo, 100);
	ms_sleep(s_ms);
	maestro_set_servo_pos(m, t->right.servo, 50);
	ms_sleep(s_ms/2);
     }
}

int
main(int argc, char **argv)
{
    const char *arg = argc >= 2 ? argv[1] : "";

    pi_usb_init();

    seed_random();

    if ((m = maestro_new()) == NULL) {
	fprintf(stderr, "Failed to initialize servo controller\n");
	exit(1);
    }

    for (tentacle_t &t : tentacles) {
	tentacle_servo_init(&t.left, t.delta);
	tentacle_servo_init(&t.right, t.delta);
    }

    int t = 1;

    if (strcmp(arg, "stir") == 0) {
	do_stirring();
    } else if (strcmp(arg, "loopey") == 0) {
	do_loopey(&tentacles[t]);
    } else if (strcmp(arg, "reset") == 0) {
	// do nothing
    } else if (strcmp(arg, "goto") == 0) {
	int argi = 2;
	int deg = 0;

	for (tentacle_t &t : tentacles) {
	     if (argc > argi) deg = atoi(argv[argi++]);
	     tentacle_goto(&t, deg, 1);
	}
    } else {
	do_random(&tentacles[t]);
    }

    return 0;
}
