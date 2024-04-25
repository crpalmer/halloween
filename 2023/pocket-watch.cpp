#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pi-gpio.h"
#include "gp-input.h"
#include "pi.h"
#include "physics.h"
#include "servo-gpio.h"
#include "util.h"

#define COORDINATED_SERVOS 0

#define LOW  0.0
#define HIGH 1.0

#define N_SERVOS 2

#define MAX_V          (1.0 / (0.15 / 60 * 270))  /* cover full range at max speed of the DS-3218MG @ 5V */

typedef struct {
    unsigned pin;
    double min_v, max_v;
    double accel_t;
    double pos, last_pos;
    Servo *servo;
    Physics *p;
} servo_t;

static servo_t servos[N_SERVOS] = {
	{ 0, MAX_V * 0.5, MAX_V * 0.75, 0.5 },
	{ 1, MAX_V * 1.0, MAX_V * 0.75, 0.5 },
};

static void
start_motion(servo_t *s, double v, double a)
{
    s->p->set_max_velocity(v);
    s->p->set_acceleration(a);
    double range = fabs(s->pos - s->last_pos);
    s->p->start_motion(0, range);
    printf("servo %d: %.3f -> %.3f @ %.3f %.3f\n", s->pin, s->last_pos, s->pos, v, a);
}

static void
pick_new_pos(servo_t *s)
{
    s->last_pos = s->pos;
    do {
	s->pos = random_double_in_range(LOW, HIGH);
    } while (fabs(s->pos - s->last_pos) < .25);
}

static void
move(servo_t *s)
{
#if ! COORDINATED_SERVOS
    if (s->p->done()) {
	pick_new_pos(s);
	double v = random_double_in_range(s->min_v, s->max_v);
	double a = v / s->accel_t;
	start_motion(s, v, a);
    }
#endif

    int dir = s->pos < s->last_pos ? -1 : +1;
    double pos = s->last_pos + dir * s->p->get_pos();
    s->servo->move_to(pos);
}

int
main()
{
    pi_init();
    gpioInitialise();

    for (int i = 0; i < N_SERVOS; i++) {
	servo_t *s = &servos[i];
	s->pos = s->last_pos = 0.5;
	s->servo = new GpioServo(s->pin);
	s->p = new Physics();
	s->servo->move_to(s->last_pos);
    }

    ms_sleep(1000);

    while (1) {
#if COORDINATED_SERVOS
	if (servos[0].p->done()) {
	    servo_t *s0 = &servos[0];
	    pick_new_pos(s0);
	    double v = random_double_in_range(s0->min_v, s0->max_v);
	    double a = v / s0->accel_t;
	    for (int i = 0; i < N_SERVOS; i++) {
		servo_t *s = &servos[i];
		if (i > 0) {
		    s->last_pos = s->pos;
		    s->pos = s0->pos;
		}
		start_motion(s, v, a);
	    }
	}
#endif
	for (int i = 0; i < N_SERVOS; i++) {
	    move(&servos[i]);
	}
	ms_sleep(20);
    }
}
