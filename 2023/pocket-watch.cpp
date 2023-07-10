#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "externals/PIGPIO/pigpio.h"
#include "gp-input.h"
#include "pi.h"
#include "physics.h"
#include "servo.h"
#include "util.h"

#define ACCEL_S		0.050 /* seconds before reaching full speed */
#define SLOW_V		1.5   /* seconds to cover the full range */
#define FAST_V          (0.14 / 60 * 360)  /* seconds to cover the full range, maximum speed of the DS-3218MG @ 6V */

#define LOW	0.0
#define HIGH	1.0
#define D       (HIGH - LOW)

static Servo *head;
static GPInput *button;

static enum { FREQUENT, RARE } mode = RARE;

#define WAIT_1 1000
#define WAIT_2_RARE 30000
#define WAIT_2_FREQUENT 2500

static void sleep_with_button(int ms)
{
    if (ms > 100) {
        printf("Sleep %5d ms\n", ms);
	fflush(stdout);
    }

    while (ms > 0) {
	ms_sleep(ms > 20 ? 20 : ms);
	ms -= 20;

	if (button->get()) {
	    while (button->get()) {}
	    mode = mode == RARE ? FREQUENT : RARE;
	    return;
	}
    }
}

void move(Physics *p, double p0, int dir)
{
    while (! p->done()) {
	double pos = p0 + dir * p->get_pos();
	head->go(pos);
	sleep_with_button(20);
    }
}

int
main()
{
    pi_init();

    ms_sleep(1000);

    Physics *slow = new Physics((1.0/SLOW_V) / ACCEL_S, 1.0 / SLOW_V);
    Physics *fast = new Physics((1.0/FAST_V) / ACCEL_S, 1.0 / FAST_V);

    gpioInitialise();

    head = new Servo(0);
    button = new GPInput(1);
    button->set_pullup_up();

    while (1) {
	printf("Go to HIGH\n");
	slow->start_motion(0, D);
	move(slow, LOW, +1);
	sleep_with_button(WAIT_1);
	printf("Go to LOW\n");
	fast->start_motion(0, D);
	move(fast, HIGH, -1);
	int ms = (mode == RARE ? WAIT_2_RARE : WAIT_2_FREQUENT);
	sleep_with_button(ms);
    }
}
