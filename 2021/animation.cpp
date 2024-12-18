#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "pi.h"
#include "animation-station.h"
#include "audio.h"
#include "audio-player.h"
#include "file.h"
#include "random-audio.h"
#include "random-utils.h"
#include "wb.h"

static Audio *audio = Audio::create_instance();
static AudioPlayer *player = new AudioPlayer(audio);
static WeenBoard *wb;

class Button : public AnimationStationAction {
public:
    Button(Output *light, Input *button) {
	this->light = light;
	this->button = button;
	button->set_pullup_down();
	button->set_debounce(100);
	pin = NULL;
	cmd = NULL;
    }

    void set_pin(Output *pin) {
	this->pin = pin;
    }

    void set_cmd(const char *cmd) {
	if (this->cmd) fatal_free(this->cmd);
	this->cmd = fatal_strdup(cmd);
    }

    Output *get_light() override { return light; }
    bool is_triggered() override { return button->get(); }
    virtual void act(void) { }
    virtual void act(Lights *lights) { act(); }

    virtual char *handle_remote_cmd(const char *cmd) {
	if (this->cmd && strcmp(cmd, this->cmd) == 0) {
	    act();
	    return fatal_strdup("ok");
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
	    if (! player->is_active()) ms_sleep(down_ms(down));
	}
    }

    void attack(double up, double down) {
        struct timespec start;

	if (! pin) {
	    fprintf(stderr, "attack: pin not defined\n");
	    exit(1);
	}

        nano_gettime(&start);

	if (random_audio->is_empty()) {
	    attack_without_audio(up, down);
	} else {
	    random_audio->play_random(player);
	    attack_with_audio(up, down);
	}

	fprintf(stderr, "total time: %d ms\n", nano_elapsed_ms_now(&start));
    }

private:
    unsigned up_ms(double up) {
        return (500 + random_number_in_range(0, 250) - 125)*up;
    }

    unsigned down_ms(double down) {
	return (200 + random_number_in_range(0, 100) - 50)*down;
    }

private:
    Output *light;
    Input *button;
    Output *pin;
    char *cmd;
    RandomAudio *random_audio = new RandomAudio();
};

class MandM : public Button {
public:
    MandM() : Button(wb->get_output(2, 1), wb->get_input(4)) {
	set_pin(wb->get_output(1, 1));
	set_cmd("M&M");
    }

    void act(void) {
	fprintf(stderr, "M&M\n");
	if (! file_exists("disable-m-and-m")) attack(1, 0.75);
    }
};

class Bunny : public Button {
public:
    Bunny() : Button(wb->get_output(2, 2), wb->get_input(3)) {
	set_pin(wb->get_output(1, 2));
	set_cmd("bunny");
    }

    void act() {
	fprintf(stderr, "bunny\n");
	if (! file_exists("disable-bunny")) attack(1, 1);
    }
};

class Demono : public Button {
public:
    Demono() : Button(wb->get_output(2, 3), wb->get_input(2)) {
	set_pin(wb->get_output(1, 3));
	set_cmd("demono");
    }

    void act() {
	fprintf(stderr, "demono\n");
	if (! file_exists("disable-demono")) attack(0.75, 2.5);
    }
};

class Snake : public Button {
public:
    Snake() : Button(wb->get_output(2, 4), wb->get_input(1)) {
	set_pin(wb->get_output(1, 4));
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
    pi_init();
    seed_random();

    wb = new WeenBoard();

    AnimationStation *as = new AnimationStation();
    as->set_blink_ms(500);
    as->add_action(new MandM());
    as->add_action(new Bunny());
    as->add_action(new Demono());
    as->add_action(new Snake());

    AnimationStationController *asc = new AnimationStationController();
    asc->add_station(as);
    asc->main();
}

