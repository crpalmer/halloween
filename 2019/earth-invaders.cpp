#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "digital-counter.h"
#include "earth-invaders-io.h"
#include "l298n.h"
#include "pca9685.h"
#include "mcp23017.h"
#include "time-utils.h"
#include "track.h"
#include "util.h"
#include "animation-station.h"

#define SCORE_INC	10

#define GAME_PLAY_MS	(15*1000)
#define SPEED	1

static double speed = 0;

static earth_invaders_io_t *io;

static track_t *game_over_track;
static track_t *hit_track[2];
static track_t *start_track;
static track_t *winner_track;
static bool game_active;
static int high_score;
static int scores[2];
static bool mean_mode[2];

static void yield()
{
    ms_sleep(1);
}

static void *
motor_main(void *unused)
{
    bool direction = true;

    while (1) {
	if (direction == TO_IDLER && io->endstop_idler->get() == ENDSTOP_TRIGGERED) {
	    direction = TO_MOTOR;
	    io->change_motor(direction, speed);
	} else if (direction == TO_MOTOR && io->endstop_motor->get() == ENDSTOP_TRIGGERED) {
	    direction = TO_IDLER;
	    io->change_motor(direction, speed);
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
    struct timespec last_hit;

    nano_gettime(&last_hit);
    for (;;) {
	while (! game_active) yield();
	int active_target = random_number_in_range(0, 2);
	set_active_target(p, active_target);
	struct timespec active_at;
	nano_gettime(&active_at);
        while (game_active) {
	    int need_new_target = mean_mode[p] && nano_elapsed_ms_now(&active_at) > 1*1000;
	    if (nano_elapsed_ms_now(&last_hit) > 100 && io->targets[p][active_target]->get() == TARGET_HIT) {
		track_play_asynchronously(hit_track[p], NULL);
		scores[p] += SCORE_INC;
		io->score[p]->set(scores[p]);
		need_new_target = true;
		nano_gettime(&last_hit);
	    }
	    if (need_new_target) {
		int last_target = active_target;
		while (last_target == active_target) active_target = random_number_in_range(0, 2);
		set_active_target(p, active_target);
		nano_gettime(&active_at);
	    }
	}
	io->lights[p][active_target]->off();
	mean_mode[p] = false;
    }
}

static void
start_pushed(void)
{
    scores[0] = scores[1] = 0;
    io->score[0]->set(0);
    io->score[1]->set(0);

    track_play(start_track);

    motor_start();
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

class StartButton : public AnimationStationAction {
    output_t *get_light() override { return io->start_light; }
    bool is_triggered() override { return io->start_button->get() == BUTTON_PUSHED; }
    void act(Lights *lights) override {
	start_pushed();
    }
};

class ResetHighScore : public AnimationStationAction {
    char *handle_remote_cmd(const char *cmd) override {
	if (strcmp(cmd, "reset-high-score") == 0) {
	     io->high_score->set(0);
	     high_score = 0;
	     return strdup("high score reset");
	}
	return NULL;
    }
};

class MeanMode : public AnimationStationAction {
    char *handle_remote_cmd(const char *cmd) override {
	printf("cmd: [%s]\n", cmd);
	if (strcmp(cmd, "mean-mode 1") == 0 || strcmp(cmd, "mean-mode red") == 0) {
	     //mean_mode[PLAYER_1] = true;
	     return strdup("mean mode active");
	} else if (strcmp(cmd, "mean-mode 2") == 0 || strcmp(cmd, "mean-mode green") == 0) {
	     //mean_mode[PLAYER_2] = true;
	     return strdup("mean mode active");
	}
	printf("cmd not recognized\n");
	return NULL;
    }
    bool needs_exclusivity() override { printf("needs excl\n"); return false; }
};

int main(int argc, char **argv)
{
    gpioInitialise();
    seed_random();

    io = new earth_invaders_io_t();

    io->motor->speed(0);
    io->motor->direction(true);

if (argc > 1) return(0);

    game_over_track = track_new("game-over.wav");
    hit_track[0] = track_new("hit1.wav");
    hit_track[1] = track_new("hit2.wav");
    start_track = track_new("ready-set-go.wav");
    winner_track = track_new("high-score.wav");

    pthread_t motor_thread, p1_thread, p2_thread;
    pthread_create(&motor_thread, NULL, motor_main, NULL);
    pthread_create(&p1_thread, NULL, player_main, (void *) PLAYER_1);
    pthread_create(&p2_thread, NULL, player_main, (void *) PLAYER_2);

    AnimationStation *as = new AnimationStation();
    as->add_action(new StartButton());
    as->add_action(new ResetHighScore());
    as->add_action(new MeanMode());
    AnimationStationController *asc = new AnimationStationController();
    asc->add_station(as);
    asc->main();
}
