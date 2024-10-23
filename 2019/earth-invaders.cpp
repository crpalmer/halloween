#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pi.h"
#include "audio.h"
#include "audio-player.h"
#include "digital-counter.h"
#include "earth-invaders-io.h"
#include "l298n.h"
#include "mcp23017.h"
#include "mem.h"
#include "net-listener.h"
#include "pca9685.h"
#include "pi-threads.h"
#include "time-utils.h"
#include "random-utils.h"
#include "server.h"
#include "wav.h"

#define SCORE_INC	10

#define GAME_PLAY_MS	(15*1000)
#define SPEED	1

static Audio *audio = Audio::create_instance();
static AudioPlayer *player = new AudioPlayer(audio);
static double speed = 0;

static EarthInvadersIO *io;

static AudioBuffer *game_over_track;
static AudioBuffer *hit_track[2];
static AudioBuffer *start_track;
static AudioBuffer *winner_track;
static bool game_active;
static int high_score;
static int scores[2];
static bool mean_mode[2];

static void yield() {
    ms_sleep(1);
}

class MotorThread : public PiThread {
public:
    MotorThread(const char *name) : PiThread(name) { }

    void main() {
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
};


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

class PlayerThread : public PiThread {
public:
    PlayerThread(const char *name, int p) : PiThread(name), p(p) { }

    void main() {
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
		    player->play(hit_track[p]);
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

private:
    int p;
};

static void
start_pushed(void)
{
    scores[0] = scores[1] = 0;
    io->score[0]->set(0);
    io->score[1]->set(0);

    player->play_sync(start_track);

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
	player->play_sync(winner_track);
    } else {
	player->play_sync(game_over_track);
    }
}

class EarthInvadersConnection : public ServerConnection {
public:
    EarthInvadersConnection(NetReader *r, NetWriter *w) : ServerConnection(r, w) { start(); }

    char *process_cmd(const char *cmd) override {
	if (strcmp(cmd, "reset-high-score") == 0) {
	     io->high_score->set(0);
	     high_score = 0;
	     return fatal_strdup("ok high score reset");
	} else if (strcmp(cmd, "mean-mode 1") == 0 || strcmp(cmd, "mean-mode red") == 0) {
	     //mean_mode[PLAYER_1] = true;
	     return fatal_strdup("ok mean mode active for player 1");
	} else if (strcmp(cmd, "mean-mode 2") == 0 || strcmp(cmd, "mean-mode green") == 0) {
	     //mean_mode[PLAYER_2] = true;
	     return fatal_strdup("ok mean mode active for player 2");
	}
	return fatal_strdup("error cmd not recognized\n");
    }
};

class EarthInvadersListener : public NetListener {
public:
    EarthInvadersListener() { start(); }

    void accepted(int fd) {
	new EarthInvadersConnection(new NetReader(fd), new NetWriter(fd));
    }
};

int main(int argc, char **argv)
{
    pi_init();
    seed_random();

    io = new EarthInvadersIO();

    io->motor->speed(0);
    io->motor->direction(true);

if (argc > 1) return(0);

    game_over_track = wav_open("game-over.wav");
    hit_track[0] = wav_open("hit1.wav");
    hit_track[1] = wav_open("hit2.wav");
    start_track = wav_open("ready-set-go.wav");
    winner_track = wav_open("high-score.wav");

    new MotorThread("motor");
    new PlayerThread("player 1", 1);
    new PlayerThread("player 2", 2);

    new EarthInvadersListener();

    while (1) {
	const int flash_ms = 500;
	int flash_ms_left = 0;
	bool light = false;

	while (io->start_button->get() != BUTTON_PUSHED) { 
	    if (flash_ms_left <= 0) {
		flash_ms_left = flash_ms;
		light = !light;
		io->start_light->set(light);
	    }
	    ms_sleep(10);
	    flash_ms_left -= 10;
	}
	io->start_light->set(false);
	start_pushed();
    }
}
