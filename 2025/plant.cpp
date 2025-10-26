#include "pi.h"
#include "gp-input.h"
#include "gp-output.h"
#include "riser-prop.h"
#include "tb6612.h"

static const bool TEST = false;

static GPInput *get_gpio(int gpio) {
    GPInput *gp = new GPInput(gpio);
    gp->set_pullup_up();
    return gp;
}

int main(int argc, char **argv) {
    pi_init();

    ms_sleep(1000);

    Output *standby = new GPOutput(3);
    Output *dir1 = new GPOutput(2);
    Output *dir2 = new GPOutput(1);
    Output *pwm = new GPOutput(0);
    Motor *motor = new TB6612(standby, dir1, dir2, pwm);

    Input *low_es = get_gpio(4);
    Input *high_es = get_gpio(5);
    Input *trigger = get_gpio(15);

    RiserProp *riser = new RiserProp(motor, low_es, high_es);

    if (TEST) riser->test();

    while (true) {
	printf(".... waiting to be activated ....\n");
	while (! trigger->get()) ms_sleep(100);
	printf("GO\n");
	riser->up();
	ms_sleep(500);
	riser->down();
	printf("DONE\n");
	while (trigger->get()) ms_sleep(100);
    }
}
