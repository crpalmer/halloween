#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "pi.h"
#include "animation-station.h"
#include "audio.h"
#include "audio-player.h"
#include "pi-gpio.h"
#include "random-utils.h"
#include "wav.h"
#include "wb.h"

static Audio *audio;
static AudioPlayer *audio_player;

static bool
file_exists(const char *fname)
{
    struct stat s;

    return stat(fname, &s) >= 0;
}

class Button : public AnimationStationAction {
public:
    Button(Output *light, Input *button) {
	this->light = light;
	this->button = button;
	button->set_pullup_down();
	button->set_debounce(1);
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

    virtual char *handle_remote_cmd(const char *cmd, Lights *lights) {
	if (this->cmd && strcmp(cmd, this->cmd) == 0) {
	    act(lights);
	    return fatal_strdup("ok");
	}
	return NULL;
    }

protected:
    void attack(double up, double down) {
	unsigned i;

	for (i = 0; i < 3; i++) {
	    pin->set(true);
	    ms_sleep(up_ms(up));
	    pin->set(false);
	    ms_sleep(down_ms(down));
	}
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
};

class Bunny : public Button {
public:
    Bunny() : Button(wb_get_output(1, 1), wb_get_input(1)) {
	set_pin(wb_get_output(2, 1));
	set_cmd("bunny");
    }

    void act() {
	fprintf(stderr, "bunny\n");
	if (! file_exists("disable-bunny")) attack(1.0, 2.5);
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
	if (! file_exists("disable-gater")) attack(2, 2);
    }
};

class Question : public Button {
public:
    Question() : Button(wb_get_output(1, 3), wb_get_input(3)) {
	head = wb_get_output(2, 3);
	audio_buffer = wav_open("question.wav");
	set_cmd("question");
    }

    void act(Lights *l) {
	fprintf(stderr, "question\n");
	audio_player->play(audio_buffer);
        l->blink_all();
        head->set(true);
	audio_player->wait_done();
	head->set(false);
    }

private:
    Output *head;
    AudioBuffer *audio_buffer;
};

class Pillar : public Button {
public:
    Pillar() : Button(wb_get_output(1, 4), wb_get_input(4)) {
	set_pin(wb_get_output(2, 4));
	set_cmd("pillar");
    }

    void act(void) {
	fprintf(stderr, "pillar\n");
	if (! file_exists("disable-pillar")) attack(1, 0.75);
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

static void
main_thread(void *arg_unused)
{
    AnimationStation *as = new AnimationStation();
    as->set_blink_ms(500);
    as->add_action(new Bunny());
    as->add_action(new Gater());
    as->add_action(new Question());
    as->add_action(new Pillar());
    as->add_action(new Snake());

    AnimationStationController *asc = new AnimationStationController();
    asc->add_station(as);
    asc->main();
}

void threads_main(int argc, char **argv) {
    gpioInitialise();
    seed_random();
    wb_init();

#ifdef PLATFORM_pico
    audio = new AudioPico();
#else
    audio = new AudioPi();
#endif
    audio_player = new AudioPlayer(audio);

    pi_thread_create("main", main_thread, NULL);
}

int
main(int argc, char **argv)
{
    pi_init_with_threads(threads_main, argc, argv);
}
