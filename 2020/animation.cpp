#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "externals/PIGPIO/pigpio.h"
#include "animation-station.h"
#include "track.h"
#include "util.h"
#include "wb.h"

#define MAX_TRACKS	10

class Button : public AnimationStationAction {
public:
    Button(output_t *light, input_t *button) {
	this->light = light;
	this->button = button;
	this->pin = NULL;
	this->cmd = NULL;
	this->n_tracks = 0;
	this->stop = stop_new();
    }

    void set_pin(output_t *pin) {
	this->pin = pin;
    }

    void set_cmd(const char *cmd) {
	if (this->cmd) free(this->cmd);
	this->cmd = strdup(cmd);
    }

    void add_track(const char *wav) {
	if (n_tracks >= MAX_TRACKS) {
	    fprintf(stderr, "too many tracks: %s\n", wav);
	    exit(1);
	}

	track_t *t = track_new(wav);

	if (! t) {
	    perror(wav);
	    exit(1);
	}
	tracks[n_tracks++] = t;
    }

    output_t *get_light() override { return light; }
    bool is_triggered() override { return button->get(); }
    virtual void act(void) { }
    virtual void act(Lights *lights) { act(); }

    virtual char *handle_remote_cmd(const char *cmd) {
	if (this->cmd && strcmp(cmd, this->cmd) == 0) {
	    act();
	    return strdup("ok");
	}
	return NULL;
    }

protected:
    void attack_without_audio(double up, double down) {
	unsigned i;

	for (i = 0; i < 3; i++) {
fprintf(stderr, "up\n");
	    pin->set(true);
	    ms_sleep(up_ms(up));
fprintf(stderr, "down\n");
	    pin->set(false);
	    ms_sleep(down_ms(down));
	}
    }

    void attack_with_audio(track_t *t, double up, double down) {
	track_set_volume(t, 0);
	stop_reset(stop);
	track_play_asynchronously(t, stop);
	while (! stop_is_stopped(stop)) {
fprintf(stderr, "up\n");
	    pin->set(true);
	    ms_sleep(up_ms(up));
fprintf(stderr, "down\n");
	    pin->set(false);
	    if (! stop_is_stopped(stop)) ms_sleep(down_ms(down));
	}
    }

    void attack(double up, double down) {
        struct timespec start;

	if (! pin) {
	    fprintf(stderr, "attack: pin not defined\n");
	    exit(1);
	}

        nano_gettime(&start);

	if (n_tracks == 0) attack_without_audio(up, down);
	else attack_with_audio(tracks[random_number_in_range(0, n_tracks-1)], up, down);

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
    output_t *light;
    input_t *button;
    output_t *pin;
    char *cmd;
    track_t *tracks[MAX_TRACKS];
    int n_tracks;
    stop_t *stop;
};

class CandyCorn : public Button {
public:
    CandyCorn() : Button(wb_get_output(1, 1), wb_get_input(1)) {
	set_pin(wb_get_output(2, 1));
	set_cmd("candy corn");
	add_track("candy corn 1.wav");
	add_track("candy corn 2.wav");
	add_track("candy corn 3.wav");
	add_track("candy corn 4.wav");
    }

    void act(void) {
	fprintf(stderr, "candy corn\n");
	attack(1, 1);
    }
};

class PopTots : public Button {
public:
    PopTots() : Button(wb_get_output(1, 2), wb_get_input(2)) {
	set_pin(wb_get_output(2, 2));
	set_cmd("pop tots");
    }

    void act() {
	fprintf(stderr, "pop tots\n");
	attack(1, 1);
    }
};

class Twizzler : public Button {
public:
    Twizzler() : Button(wb_get_output(1, 3), wb_get_input(3)) {
	set_pin(wb_get_output(2, 3));
	set_cmd("twizzler");
    }

    void act() {
	fprintf(stderr, "twizzler\n");
	attack(1, 1);
    }
};

class KitKat : public Button {
public:
    KitKat() : Button(wb_get_output(1, 4), wb_get_input(4)) {
	set_pin(wb_get_output(2, 4));
	set_cmd("kit kat");
    }

    void act() {
	fprintf(stderr, "kit-kat\n");
	attack(1, 1);
    }
};

int
main(int argc, char **argv)
{
    gpioInitialise();
    seed_random();
    wb_init();

    AnimationStation *as = new AnimationStation();
    as->set_blink_ms(500);
    as->add_action(new CandyCorn());
    as->add_action(new PopTots());
    as->add_action(new Twizzler());
    as->add_action(new KitKat());

    AnimationStationController *asc = new AnimationStationController();
    asc->add_station(as);
    asc->main();
}

