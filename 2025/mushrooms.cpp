#include "pi.h"
#include "gp-input.h"
#include "gp-output.h"
#include "stepper.h"

const int BASE_PIN[] = { 0, 2 };
const double STEPS_FOR_FULL_ROTATION = 360/1.8;
const double MM_PER_FULL_ROTATION = 120;
const double STEPS_PER_MM = STEPS_FOR_FULL_ROTATION / MM_PER_FULL_ROTATION;
const double DISTANCE_MM = 1900;
const double TIME_SEC = 3;

class Mushroom : public PiThread {
public:
    Mushroom(int base_pin) : PiThread("mushroom") {
	stepper = new Stepper(base_pin, STEPS_PER_MM);
	start();
    }

    void main(void) {
	while (1) {
	    stepper->go(DISTANCE_MM-1, DISTANCE_MM/TIME_SEC);
	    stepper->go(1, DISTANCE_MM/TIME_SEC);
	}
    }

private:
    Stepper *stepper;
};

void thread_main(int argc, char **argv) {
    ms_sleep(1000);
    printf("Starting\n");
    printf("dir %d step %d steps/mm %f\n", BASE_PIN[0], BASE_PIN[1], STEPS_PER_MM);
    new Mushroom(BASE_PIN[0]);
    new Mushroom(BASE_PIN[1]);
}

int main(int argc, char **argv) {
    pi_init_with_threads(thread_main, argc, argv);
}
