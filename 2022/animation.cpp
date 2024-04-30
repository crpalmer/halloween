#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "animation-station.h"
#include "audio.h"
#include "audio-player.h"
#include "pi-gpio.h"
#include "random-wavs.h"
#include "random-utils.h"
#include "wb.h"

static Audio *audio;
static AudioPlayer *player;

static bool
file_exists(const char *fname)
{
    struct stat s;

    return stat(fname, &s) >= 0;
}

class Button : public AnimationStationAction {
public:
    Button(output_t *light, input_t *button) {
	this->light = light;
	this->button = button;
	button->set_pullup_down();
	button->set_debounce(1);
	pin = NULL;
	cmd = NULL;
    }

    void set_pin(output_t *pin) {
	this->pin = pin;
    }

    void set_cmd(const char *cmd) {
	if (this->cmd) free(this->cmd);
	this->cmd = strdup(cmd);
    }

    output_t *get_light() override { return light; }
    bool is_triggered() override { return button->get(); }
    virtual void act(void) { }
    virtual void act(Lights *lights) { act(); }

    virtual char *handle_remote_cmd(const char *cmd, Lights *lights) {
	if (this->cmd && strcmp(cmd, this->cmd) == 0) {
	    act(lights);
	    return strdup("ok");
	}
	return NULL;
    }

protected:
    void attack_without_audio(double up, double down) {
	unsigned i;

	for (i = 0; i < 3; i++) {
	    pin->set(true);
	    ms_sleep(up_ms(up));
	    pin->set(false);
	    ms_sleep(down_ms(down));
	}
    }

    void attack_with_audio(double up, double down) {
	while (player->is_active()) {
	    pin->set(true);
	    ms_sleep(up_ms(up));
	    pin->set(false);
	    if (player->is_active()) ms_sleep(down_ms(down));
	}
    }

    void attack(double up, double down) {
        struct timespec start;

	if (! pin) {
	    fprintf(stderr, "attack: pin not defined\n");
	    exit(1);
	}

        nano_gettime(&start);

	if (random_wavs->is_empty()) {
	    attack_without_audio(up, down);
	} else {
	    random_wavs->play_random(player);
	    attack_with_audio(up, down);
	}

	fprintf(stderr, "total time: %d ms\n", nano_elapsed_ms_now(&start));
    }

    RandomWavs *random_wavs = new RandomWavs();

private:
    unsigned up_ms(double up) {
        return (500 + random_number_in_range(0, 250) - 125)*up;
    }

    unsigned down_ms(double down) {
	return (200 + random_number_in_range(0, 100) - 50)*down;
    }

private:
    output_t *light;
    input_t *button;
    output_t *pin;
    char *cmd;
};

class Mosquito : public Button {
public:
    Mosquito() : Button(wb_get_output(1, 1), wb_get_input(1)) {
	set_pin(wb_get_output(2, 1));
	set_cmd("mosquito");
    }

    void act() {
	fprintf(stderr, "mosquito\n");
	if (! file_exists("disable-mosquito")) attack(0.75, 2.5);
    }
};

class Gater : public Button {
public:
    Gater() : Button(wb_get_output(1, 2), wb_get_input(2)) {
	set_pin(wb_get_output(2, 2));
	set_cmd("gater");
    }

    void act() {
	fprintf(stderr, "gater\n");
	if (! file_exists("disable-gater")) attack(1, 1);
    }
};

class Question : public Button {
public:
    Question() : Button(wb_get_output(1, 3), wb_get_input(3)) {
	head = wb_get_output(2, 3);
	random_wavs->add("laugh.wav");
	set_cmd("question");
    }

    void act(Lights *l) {
	fprintf(stderr, "question\n");
        l->blink_all();
        head->set(true);
	ms_sleep(200);
	random_wavs->play_random(player);
	player->wait_done();
	head->set(false);
    }

private:
    output_t *head;
};

class Spider : public Button {
public:
    Spider() : Button(wb_get_output(1, 4), wb_get_input(4)) {
	set_pin(wb_get_output(2, 4));
	set_cmd("spider");
    }

    void act(void) {
	fprintf(stderr, "spider\n");
	if (! file_exists("disable-spider")) attack(1, 0.75);
    }
};

class Snake : public Button {
public:
    Snake() : Button(wb_get_output(1, 5), wb_get_input(5)) {
	set_pin(wb_get_output(2, 5));
	set_cmd("snake");
    }

    void act() {
	fprintf(stderr, "snake\n");
	if (! file_exists("disable-snake")) attack(1, 3);
    }
};

int
main(int argc, char **argv)
{
    gpioInitialise();
    seed_random();
    wb_init();

    audio = new AudioPi();
    player = new AudioPlayer(audio);

    AnimationStation *as = new AnimationStation();
    as->set_blink_ms(500);
    as->add_action(new Mosquito());
    as->add_action(new Gater());
    as->add_action(new Question());
    as->add_action(new Spider());
    as->add_action(new Snake());

    AnimationStationController *asc = new AnimationStationController();
    asc->add_station(as);
    asc->main();
}

