#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pi-gpio.h"
#include "gp-input.h"
#include "pi.h"
#include "physics.h"
#include "servo-gpio.h"
#include "util.h"

#define ACCEL		3
#define SERVO_MAX_V(s_per_60_deg, range_in_deg) (1.0 / (s_per_60_deg / 60 * range_in_deg))  /* cover full range at max speed of the DS-3218MG @ 5V */

#define SLOW_DOWN	10

#define PAN_SERVO	0
#define TILT_SERVO	1

#define PAN_DEE		0.0
#define PAN_BOOT	0.5
#define PAN_DUMB	1.0
#define TILT_DEE	0.85
#define TILT_BOOT	0.8
#define TILT_DUMB	0.75

#define PAN_SERVO_RANGE	188
#define PAN_SERVO_MAX_V	SERVO_MAX_V(0.16, PAN_SERVO_RANGE)

#define PAN_MAX_V       (PAN_SERVO_MAX_V * fabs(PAN_DEE - PAN_DUMB))
#define TILT_MAX_V	(PAN_SERVO_MAX_V * fabs(TILT_DEE - TILT_DUMB))
#define PAN_ACCEL	(ACCEL * fabs(PAN_DEE - PAN_DUMB))
#define TILT_ACCEL	(ACCEL * fabs(TILT_DEE - TILT_DUMB))

typedef struct {
    int pin;
    int min_us, max_us;
    double max_v;
    double a;
    double pos, last_pos;
    Servo *servo;
    Physics *p;
} servo_t;

#define N_SERVOS	2

static servo_t servos[N_SERVOS] = {
    { TILT_SERVO, SERVO_EXTENDED_MIN, SERVO_EXTENDED_MAX, TILT_MAX_V / SLOW_DOWN, TILT_ACCEL, TILT_DEE, TILT_DUMB, },
    { PAN_SERVO,  SERVO_HS425BB_MIN,  SERVO_HS425BB_MAX,  PAN_MAX_V / SLOW_DOWN, PAN_ACCEL, PAN_DEE,  PAN_DUMB, },
};

double positions[2][N_SERVOS] = {
    { TILT_DEE, PAN_DEE },
    { TILT_DUMB, PAN_DUMB },
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
	s->servo->move_to(pos);
    }
}

static bool
is_done()
{
    for (int i = 0; i < N_SERVOS; i++) {
	if (! servos[i].p->done()) return false;
    }
    return true;
}

int
main()
{
    pi_init();
    gpioInitialise();
    int which = 0;

    ms_sleep(1000);
    printf("Tweetle is waiting to start up to let everything settle\n");
    ms_sleep(1000);

    printf("Tweetle is now starting:\n");

    for (int servo = 0; servo < N_SERVOS; servo++) {
	servo_t *s = &servos[servo];

	printf("  servo: %d [%d .. %d] range, %.03f max_v, %f a\n", s->pin, s->min_us, s->max_us, s->max_v, s->a);

        s->servo = new GpioServo(s->pin, s->min_us, s->max_us);
        s->p = new Physics();
        s->p->set_max_velocity(s->max_v);
        s->p->set_acceleration(s->a);
        s->servo->move_to(s->pos);
        ms_sleep(1000);

    }

#if 0
    // If you want to start at the boot position, uncomment the sleep to give you a chance to unplug it
    //ms_sleep(10*1000);
    double pos = 0.8;
    
    // Helper code to tune what positions to use for a servo
    while (1) {
	printf("%.2f\n", pos);
	servos[0].servo->move_to(pos);
	ms_sleep(200);
	pos += 0.01;
	if (pos > .90) pos = .70;
    }
#endif

    while (1) {
	printf("moving to %d: ");

	for (int i = 0; i < N_SERVOS; i++) {
	    printf("%s%d -> %.2f", i > 0 ? ", " : "",  servos[i].pin, positions[which][i]);
	    start_motion(&servos[i], positions[which][i]);
	}
 	
	printf("\n");

	while (! is_done()) {
	    for (int i = 0; i < N_SERVOS; i++) move(&servos[i]);
	    ms_sleep(20);
	}

	which = (which + 1) % 2;

	// TODO: What to do between movements
	ms_sleep(1000);
    }
}
