#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "pi-gpio.h"
#include "animation-station.h"
#include "util.h"
#include "wb.h"

#define MAX_TRACKS	10

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
    output_t *light;
    input_t *button;
    output_t *pin;
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
	set_cmd("question");
    }

    void act(Lights *l) {
	fprintf(stderr, "question\n");
        l->blink_all();
        head->set(true);
	ms_sleep(200);
	head->set(false);
    }

private:
    output_t *head;
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

int
main(int argc, char **argv)
{
    pi_threads_init();

    gpioInitialise();
    seed_random();
    wb_init();

    pi_thread_create_anonymous(main_thread, NULL);
    pi_threads_start_and_wait();
}
