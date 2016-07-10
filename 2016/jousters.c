#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "animation-actions.h"
#include "animation-common.h"
#include "track.h"
#include "util.h"
#include "wb.h"

#define HOME_1_PIN  5
#define WIN_1_PIN   6
#define HOME_2_PIN  7
#define WIN_2_PIN   8

#define MOTOR_BANK 1

#define WIN_MAX_MS		4000
#define RETURN_TO_START_MAX_MS 12000

#define REVERSE_DUTY 0.40
#define TROT_DUTY 0.30

#define TROT_MS		2000
#define REVERSE_MS	7500

static stop_t *stop;
static track_t *winner, *loser;
static track_t *jousting, *crash, *beeping;

static int motor_pwm[2] = { 5, 7 };
static int motor_dir[2] = { 6, 8 };

static void motor(unsigned jouster, bool forward, float duty)
{
    wb_set(MOTOR_BANK, motor_dir[jouster], forward);
    wb_pwm(MOTOR_BANK, motor_pwm[jouster], forward ? 1-duty : duty);
}

static void motor_forward(unsigned jouster, float duty)
{
    motor(jouster, true, duty);
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
wait_until_start_position(void)
{
    fprintf(stderr, "waiting for start\n");
    if (wb_wait_for_pin_timeout(HOME_1_PIN, 0, RETURN_TO_START_MAX_MS)) {
	fprintf(stderr, "at start\n");
    } else {
	fprintf(stderr, "Failed to return to start!\n");
    }
}

static void
go_to_start_position(void)
{
    motor_backward(1, REVERSE_DUTY);
    wait_until_start_position();
    motor_stop(1);
}

static void joust(void *picked_winner_as_vp, lights_t *l, unsigned pin)
{
    unsigned picked_winner = (unsigned) picked_winner_as_vp;
    unsigned winner_pin_mask;
    unsigned winner_pin;

    fprintf(stderr, "Starting joust\n");

    lights_blink_one(l, pin);

    track_play_asynchronously(jousting, stop);
    motor_forward(1, 1);

    winner_pin_mask = WB_PIN_MASK(WIN_1_PIN) | WB_PIN_MASK(WIN_2_PIN);

    if ((winner_pin = wb_wait_for_pins_timeout(winner_pin_mask, 0, WIN_MAX_MS)) != 0) {
	    stop_stop(stop);
	    track_play_asynchronously(crash, stop);
	    motor_forward(1, TROT_DUTY);
	    ms_sleep(TROT_MS);
    }

    stop_stop(stop);
    motor_stop(1);

    fprintf(stderr, "wanted %d got %d\n", picked_winner, winner_pin);

    if (winner_pin == picked_winner) {
	track_play(winner);
    } else {
	track_play(loser);
    }

    ms_sleep(100);

    track_play_asynchronously(beeping, stop);
    go_to_start_position();
    stop_stop(stop);
}

static pthread_mutex_t lock;

static action_t actions[] = {
    { "red",	joust,	 (void *) WIN_1_PIN },
    { "blue",  joust,	(void *) WIN_2_PIN },
    { NULL, NULL, NULL }
};

static station_t stations[] = {
    { actions, &lock },
    { NULL, NULL }
};

int
main(int argc, char **argv)
{
    audio_device_t second_audio_dev;

    if (wb_init() < 0) {
	fprintf(stderr, "Failed to initialize wb\n");
	exit(1);
    }

    pthread_mutex_init(&lock, NULL);

    stop = stop_new();

    audio_device_init(&second_audio_dev, 1, 0, true);

    winner = track_new_audio_dev_fatal("jousters_crash.wav", &second_audio_dev);
    loser = track_new_audio_dev_fatal("jousters_loser.wav", &second_audio_dev);

    jousting = track_new_fatal("jousters_jousting.wav");
    crash = track_new_fatal("jousters_crash.wav");
    beeping = track_new_fatal("jousters_beeping.wav");

    wb_set_pull_up(HOME_1_PIN, WB_PULL_UP_UP);
    wb_set_pull_up(WIN_1_PIN, WB_PULL_UP_UP);

    motor_forward(1, 0.5);
    ms_sleep(1000);
    motor_stop(1);
    go_to_start_position();

    animation_main(stations);

    return 0;
}
