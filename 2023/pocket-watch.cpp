#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "externals/PIGPIO/pigpio.h"
#include "gp-input.h"
#include "pi.h"
#include "physics.h"
#include "servo.h"
#include "util.h"

#define ACCEL_S		0.100 /* seconds before reaching full speed */
#define MAX_V          (1.0 / (0.15 / 60 * 270))  /* cover full range at max speed of the DS-3218MG @ 5V */
#define FAST_V		(MAX_V * .75)
#define SLOW_V		(MAX_V * .5)
#define ACCEL_T		0.500 /* seconds */
#define MAX_ACCEL	(MAX_V / ACCEL_T)

#define LOW	0.0
#define HIGH	1.0
#define D       (HIGH - LOW)

static Servo *servo;
static GPInput *button;

void move(Physics *p, double p0, int dir)
{
    while (! p->done()) {
	double pos = p0 + dir * p->get_pos();
	servo->go(pos);
	ms_sleep(20);
    }
}

int
main()
{
    pi_init();

    Physics *p = new Physics();

    gpioInitialise();

    servo = new Servo(0);
    button = new GPInput(1);
    button->set_pullup_up();

    double last_pos = 0.5;

    servo->go(last_pos);
    ms_sleep(2000);

    while (1) {
#if 1
	double pos;

	do {
	    pos = random_double_in_range(LOW, HIGH);
	} while (fabs(pos - last_pos) < .25);

	double v = random_double_in_range(SLOW_V, FAST_V);
	//double a = MAX_ACCEL;
	double a = v / ACCEL_T;
	p->set_acceleration(a);
	p->set_max_velocity(v);
	printf("move %.2f -> %.2f a %.4f v %.4f [ %.4f..%.4f ]\n", last_pos, pos, a, v, SLOW_V, FAST_V);
	double range = abs(pos - last_pos);
	p->start_motion(0, range);
	move(p, last_pos, pos > last_pos ? +1 : -1);
	last_pos = pos;
#else
	p->set_acceleration(1 / SLOW_V);
	p->set_max_velocity(SLOW_V);
	p->start_motion(0, 1);
	move(p, 0, +1);
	p->start_motion(0, 1);
	move(p, 1, -1);
#endif
    }
}
