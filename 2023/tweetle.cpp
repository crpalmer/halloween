#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "externals/PIGPIO/pigpio.h"
#include "gp-input.h"
#include "pi.h"
#include "physics.h"
#include "servo.h"
#include "util.h"

#define MAX_V(s_per_60_deg, range_in_deg) (1.0 / (s_per_60_deg / 60 * range_in_deg))  /* cover full range at max speed of the DS-3218MG @ 5V */
#define ACCEL		5

#define SLOW_DOWN	5

#define PAN_DEE		0.0
#define PAN_DUMB	1.0
#define TILT_DEE	0.70
#define TILT_DUMB	0.55


#define PAN_MAX_V	MAX_V(0.15, 188)
#define PAN_SEC		(1.0 / (PAN_MAX_V / 188))
#define TILT_DEG	(fabs(TILT_DEE - TILT_DUMB) * 270)
#define TILT_MAX_V	(TILT_DEG / PAN_SEC)

typedef struct {
    int pin;
    int min_us, max_us;
    double max_v;
    double a;
    double pos, last_pos;
    Servo *servo;
    Physics *p;
} servo_t;

static servo_t servos[2] = {
    { 0, SERVO_HS425BB_MIN, SERVO_HS425BB_MAX, PAN_MAX_V / SLOW_DOWN, ACCEL, PAN_DEE, PAN_DUMB, },
    { 1, SERVO_EXTENDED_MIN, SERVO_EXTENDED_MAX, TILT_MAX_V / SLOW_DOWN, ACCEL, TILT_DEE, TILT_DUMB, }
};

static void
start_motion(servo_t *s, double pos)
{
    s->last_pos = s->pos;
    s->pos = pos;

    double range = fabs(s->pos - s->last_pos);
    s->p->start_motion(0, range);
}


static void
move(servo_t *s)
{
    if (! s->p->done()) {
	int dir = s->pos < s->last_pos ? -1 : +1;
	double pos = s->last_pos + dir * s->p->get_pos();
	s->servo->go(pos);
    }
}

int
main()
{
    pi_init();
    gpioInitialise();

    for (int servo = 0; servo < 2; servo++) {
	servo_t *s = &servos[servo];

        s->servo = new Servo(s->pin, s->min_us, s->max_us);
        s->p = new Physics();
        s->p->set_max_velocity(s->max_v);
        s->p->set_acceleration(s->a);
        s->servo->go(s->pos);
        ms_sleep(1000);

    }

    while (1) {
	if (servos[0].pos == PAN_DEE) {
	    start_motion(&servos[0], PAN_DUMB);
	    start_motion(&servos[1], TILT_DUMB);
	} else {
	    start_motion(&servos[0], PAN_DEE);
	    start_motion(&servos[1], TILT_DEE);
	}

	while (! servos[0].p->done() || ! servos[1].p->done()) {
	    move(&servos[0]);
	    move(&servos[1]);
	    ms_sleep(20);
	}

	// TODO: What to do between movements
	ms_sleep(1000);
    }
}
