#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "externals/PIGPIO/pigpio.h"
#include "animation-station.h"
#include "util.h"
#include "wb.h"

class Button : public AnimationStationAction {
public:
    Button(output_t *light, input_t *button) {
	this->light = light;
	this->button = button;
    }

    output_t *get_light() override { return light; }
    bool is_triggered() override { return button->get(); }
    virtual void act(Lights *lights) { }

private:
    output_t *light;
    input_t *button;
};

class Button1 : public Button {
public:
    Button1() : Button(wb_get_output(1, 1), wb_get_input(1)) {
    }

    void act(Lights *lights) {
	printf("prop 1\n");
    }
};

class Button2 : public Button {
public:
    Button2() : Button(wb_get_output(1, 2), wb_get_input(2)) {
    }

    void act(Lights *lights) {
	printf("prop 2\n");
    }
};

class Button3 : public Button {
public:
    Button3() : Button(wb_get_output(1, 3), wb_get_input(3)) {
    }

    void act(Lights *lights) {
	printf("prop 3\n");
    }
};

class Button4 : public Button {
public:
    Button4() : Button(wb_get_output(1, 4), wb_get_input(4)) {
    }

    void act(Lights *lights) {
	printf("prop 4\n");
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
    as->add_action(new Button1());
    as->add_action(new Button2());
    as->add_action(new Button3());
    as->add_action(new Button4());

    AnimationStationController *asc = new AnimationStationController();
    asc->add_station(as);
    asc->main();
}

