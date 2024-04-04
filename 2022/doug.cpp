#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pigpio.h"
#include "gp-input.h"
#include "pi.h"
#include "servo.h"
#include "util.h"

#define LOW	0.45
#define HIGH	0.60

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

void move(double l, double h, double delta)
{
#define MULTI_STEPS 2
    double pos = l;

    while ((delta < 0 ? pos >= h : pos <= h)) {
	pos += delta * MULTI_STEPS;
	head->go(pos);
	sleep_with_button(20 * MULTI_STEPS);
    }
}

int
main()
{
    pi_init();

    ms_sleep(1000);

    gpioInitialise();

    head = new Servo(0);
    button = new GPInput(1);
    button->set_pullup_up();

    while (1) {
	printf("Go to HIGH\n");
	move(LOW, HIGH, +.002);
	sleep_with_button(WAIT_1);
	printf("Go to LOW\n");
	move(HIGH, LOW, -.008);
	int ms = (mode == RARE ? WAIT_2_RARE : WAIT_2_FREQUENT);
	sleep_with_button(ms);
    }
}
