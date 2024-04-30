#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "audio.h"
#include "audio-player.h"
#include "digital-counter.h"
#include "earth-invaders-io.h"
#include "l298n.h"
#include "mcp23017.h"
#include "pca9685.h"
#include "pi-threads.h"
#include "time-utils.h"
#include "random-utils.h"
#include "wav.h"
#include "animation-station.h"

#define SCORE_INC	10

#define GAME_PLAY_MS	(15*1000)
#define SPEED	1

static Audio *audio = new AudioPi();
static AudioPlayer *player = new AudioPlayer(audio);
static double speed = 0;

static earth_invaders_io_t *io;

static Wav *game_over_track;
static Wav *hit_track[2];
static Wav *start_track;
static Wav *winner_track;
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
		    player->play(hit_track[p]->to_audio_buffer());
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

    player->play_sync(start_track->to_audio_buffer());

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
	player->play_sync(winner_track->to_audio_buffer());
    } else {
	player->play_sync(game_over_track->to_audio_buffer());
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
	     return strdup("ok high score reset");
	}
	return NULL;
    }
};

class MeanMode : public AnimationStationAction {
    char *handle_remote_cmd(const char *cmd) override {
	printf("cmd: [%s]\n", cmd);
	if (strcmp(cmd, "mean-mode 1") == 0 || strcmp(cmd, "mean-mode red") == 0) {
	     //mean_mode[PLAYER_1] = true;
	     return strdup("ok mean mode active for player 1");
	} else if (strcmp(cmd, "mean-mode 2") == 0 || strcmp(cmd, "mean-mode green") == 0) {
	     //mean_mode[PLAYER_2] = true;
	     return strdup("ok mean mode active for player 2");
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

    game_over_track = new Wav(new BufferFile("game-over.wav"));
    hit_track[0] = new Wav(new BufferFile("hit1.wav"));
    hit_track[1] = new Wav(new BufferFile("hit2.wav"));
    start_track = new Wav(new BufferFile("ready-set-go.wav"));
    winner_track = new Wav(new BufferFile("high-score.wav"));

    new MotorThread("motor");
    new PlayerThread("player 1", 1);
    new PlayerThread("player 2", 2);

    AnimationStation *as = new AnimationStation();
    as->add_action(new StartButton());
    as->add_action(new ResetHighScore());
    as->add_action(new MeanMode());
    AnimationStationController *asc = new AnimationStationController();
    asc->add_station(as);
    asc->main();
}
