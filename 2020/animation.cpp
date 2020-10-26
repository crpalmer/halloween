#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "externals/PIGPIO/pigpio.h"
#include "animation-station.h"
#include "util.h"
#include "wb.h"

class Button : public AnimationStationAction {
public:
    Button(output_t *light, input_t *button, output_t *pin, const char *cmd = NULL) {
	this->light = light;
	this->button = button;
	this->pin = pin;
	this->cmd = cmd;
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
    void attack(double up, double down) {
	unsigned i;

	for (i = 0; i < 3; i++) {
	    pin->set(true);
printf("up\n");
	    ms_sleep((500 + random_number_in_range(0, 250) - 125)*up);
	    pin->set(false);
printf("down\n");
	    ms_sleep((200 + random_number_in_range(0, 100) - 50)*down);
	}
    }

private:
    output_t *light;
    input_t *button;
    output_t *pin;
    const char *cmd;
};

class CandyCorn : public Button {
public:
    CandyCorn() : Button(wb_get_output(1, 1), wb_get_input(1), wb_get_output(2, 1), "candy corn") {
    }

    void act(void) {
	printf("candy corn\n");
	attack(1, 1);
    }
};

class PopTots : public Button {
public:
    PopTots() : Button(wb_get_output(1, 2), wb_get_input(2), wb_get_output(2, 2), "pop tots") {
    }

    void act() {
	printf("pop tots\n");
	attack(1, 1);
    }
};

class Twizzler : public Button {
public:
    Twizzler() : Button(wb_get_output(1, 3), wb_get_input(3), wb_get_output(2, 3), "twizzler") {
    }

    void act() {
	printf("twizzler\n");
	attack(1, 1);
    }
};

class KitKat : public Button {
public:
    KitKat() : Button(wb_get_output(1, 4), wb_get_input(4), wb_get_output(2, 4), "kit kat") {
    }

    void act() {
	printf("kit-kat\n");
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

