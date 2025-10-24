#include "pi.h"
#include "gp-input.h"
#include "riser-prop.h"

static const bool TEST = true;

static GPInput *get_gpio(int gpio) {
    GPInput *gp = new GPInput(gpio);
    gp->set_pullup_up();
    return gp;
}

int main(int argc, char **argv) {
    pi_init();

    Motor *motor = NULL;

    Input *low_es = get_gpio(2);
    Input *high_es = get_gpio(3);
    Input *trigger = get_gpio(4);

    RiserProp *riser = new RiserProp(motor, low_es, high_es);

    if (TEST) riser->test();

    while (true) {
	while (! trigger->get()) ms_sleep(100);
	riser->up();
	ms_sleep(500);
	riser->down();
	while (trigger->get()) ms_sleep(100);
    }
}
