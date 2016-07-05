#include <stdio.h>
#include <stdlib.h>
#include "track.h"
#include "util.h"
#include "wb.h"

#define HOME_1_PIN  1
#define TRIGGER_PIN 8

#define DEBOUNCE_COUNTER 100

#define REVERSE_DUTY 0.40
#define TROT_DUTY 0.40

#define MIDDLE_MS	3000
#define TROT_MS		2200
#define REVERSE_MS	7500

static int motor_pwm[2][2] = { { -1, -1 }, { 5, 7 } };
static int motor_dir[2][2] = { { -1, -1 }, { 6, 8 } };

static void motor(unsigned jouster, bool forward, float duty)
{
    wb_set(WB_OUTPUT(1, motor_dir[jouster][0]), forward);
    wb_set(WB_OUTPUT(1, motor_dir[jouster][1]), forward);
    wb_pwm(WB_OUTPUT(1, motor_pwm[jouster][0]), duty);
    wb_pwm(WB_OUTPUT(1, motor_pwm[jouster][1]), duty);
}

static void motor_forward(unsigned jouster, float duty)
{
    motor(jouster, true, 1-duty);
}

static void motor_backward(unsigned jouster, float duty)
{
    motor(jouster, false, duty);
}

static void motor_stop(unsigned jouster)
{
    motor(jouster, false, 0);
}

static void
wait_for_pin(int pin, unsigned value)
{
    unsigned yes = 0;

    while (true) {
	if (wb_get(pin) == value) {
	    yes++;
	} else {
	    yes = 0;
	}
	if (yes >= DEBOUNCE_COUNTER) return;
    }
}

static void
wait_for_trigger(void)
{
    wait_for_pin(TRIGGER_PIN, 1);
}

static void
wait_until_start_position(void)
{
fprintf(stderr, "waiting for start\n");
    wait_for_pin(HOME_1_PIN, 0);
fprintf(stderr, "at start\n");
}

static void
wait_until_middle(void)
{
    ms_sleep(MIDDLE_MS);
}

static void
go_to_start_position(void)
{
    motor_backward(1, REVERSE_DUTY);
    wait_until_start_position();
    motor_stop(1);
}

int
main(int argc, char **argv)
{
    stop_t *stop = stop_new();
    track_t *jousting, *crash, *beeping;

    if (wb_init() < 0) {
	fprintf(stderr, "Failed to initialize wb\n");
	exit(1);
    }

    jousting = track_new_fatal("jousters_jousting.wav");
    crash = track_new_fatal("jousters_crash.wav");
    beeping = track_new_fatal("jousters_beeping.wav");

    wb_set_pull_up(HOME_1_PIN, WB_PULL_UP_UP);

    motor_forward(1, 0.5);
    ms_sleep(1000);
    motor_stop(1);
    go_to_start_position();

    while (true) {
fprintf(stderr, "Waiting\n");
	wait_for_trigger();
fprintf(stderr, "Starting joust\n");

	track_play_asynchronously(jousting, stop);
	motor_forward(1, 1);
	wait_until_middle();
	stop_stop(stop);

	track_play_asynchronously(crash, stop);
	motor_forward(1, TROT_DUTY);
	ms_sleep(TROT_MS);
	/* gradual deceleration needed here? */

	stop_stop(stop);
	motor_stop(1);
	ms_sleep(100);

	track_play_asynchronously(beeping, stop);
	go_to_start_position();
	stop_stop(stop);
    }

    return 0;
}
