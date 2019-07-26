#include <stdio.h>
#include <pthread.h>
#include "digital-counter.h"
#include "earth-invaders-io.h"
#include "l298n.h"
#include "pca9685.h"
#include "mcp23017.h"
#include "time-utils.h"
#include "track.h"
#include "util.h"

#define GAME_PLAY_MS	(15*1000)
#define SPEED	1

static double speed = 0;

static earth_invaders_io_t *io;

static track_t *game_over_track;
static track_t *hit_track;
static track_t *start_track;
static track_t *winner_track;
static bool game_active;
static int high_score;
static int scores[2];

static void yield()
{
    ms_sleep(1);
}

static void
change_motor(bool direction)
{
printf("motor going %s\n", direction ? "forward" : "backward");
    io->motor->speed(0);
    ms_sleep(50);
    io->motor->direction(direction);
    io->motor->speed(speed);
}

static void *
motor_main(void *unused)
{
    bool direction = true;

    while (1) {
	if (direction == TO_IDLER && io->endstop_idler->get() == ENDSTOP_TRIGGERED) {
	    direction = TO_MOTOR;
	    change_motor(direction);
	} else if (direction == TO_MOTOR && io->endstop_motor->get() == ENDSTOP_TRIGGERED) {
	    direction = TO_IDLER;
	    change_motor(direction);
	}
    }
}

static void
motor_start(void)
{
    speed = SPEED;
    io->motor->speed(speed);
}

static void
motor_stop(void)
{
    speed = 0;
    io->motor->speed(speed);
}

static void
set_active_target(int p, int t)
{
    for (int i = 0; i < 3; i++) {
	io->lights[p][i]->set(i==t);
    }
}

static void *
player_main(void *p_as_void)
{
    int p = (int) p_as_void;

    for (;;) {
	while (! game_active) yield();
	int active_target = random_number_in_range(0, 2);
	set_active_target(p, active_target);
	struct timespec active_at;
	nano_gettime(&active_at);
        while (game_active) {
#if 1
	    if (io->triggers[p]->get() == BUTTON_PUSHED && io->targets[p][active_target]->get() == TARGET_HIT) {
		track_play_asynchronously(hit_track, NULL);
		int last_target = active_target;
		while (last_target == active_target) active_target = random_number_in_range(0, 2);
		set_active_target(p, active_target);
		scores[p]++;
		io->score[p]->set(scores[p]);
	    }
#else
	    int need_new_target = nano_elapsed_ms_now(&active_at) > 1*1000;
	    if (io->triggers[p]->get() == BUTTON_PUSHED && io->targets[p][active_target]->get() == TARGET_HIT) {
		track_play_asynchronously(hit_track, NULL);
		scores[p]++;
		io->score[p]->set(scores[p]);
		need_new_target = true;
	    }
	    if (need_new_target) {
		int last_target = active_target;
		while (last_target == active_target) active_target = random_number_in_range(0, 2);
		set_active_target(p, active_target);
		nano_gettime(&active_at);
	    }
#endif
	}
	io->lights[p][active_target]->off();
    }
}

static void
start_pushed(void)
{
    scores[0] = scores[1] = 0;
    io->score[0]->set(0);
    io->score[1]->set(1);
    motor_start();

    track_play(start_track);

    io->laser->on();
    game_active = true;

    ms_sleep(GAME_PLAY_MS);

    game_active = false;
    motor_stop();
    io->laser->off();

    int old_high_score = high_score;
    if (scores[0] > high_score) high_score = scores[0];
    if (scores[1] > high_score) high_score = scores[1];

    if (high_score > old_high_score) {
	io->high_score->set(high_score);
	track_play(winner_track);
    } else {
	track_play(game_over_track);
    }
}

int main(int argc, char **argv)
{
    gpioInitialise();
    seed_random();

    io = new earth_invaders_io_t();

    io->motor->speed(0);
    io->motor->direction(true);

if (argc > 1) return(0);

    game_over_track = track_new("game-over.wav");
    hit_track = track_new("hit.wav");
    start_track = track_new("elfie-ready-set-go.wav");
    winner_track = track_new("elfie-winner.wav");

    pthread_t motor_thread, p1_thread, p2_thread;
    pthread_create(&motor_thread, NULL, motor_main, NULL);
    pthread_create(&p1_thread, NULL, player_main, (void *) 0);
    pthread_create(&p2_thread, NULL, player_main, (void *) 1);

    while (1) {
	while (io->start_button->get() != BUTTON_PUSHED) yield();
	start_pushed();
    }
}
