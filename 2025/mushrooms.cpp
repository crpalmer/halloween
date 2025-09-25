#include "pi.h"
#include "gp-input.h"
#include "gp-output.h"
#include "stepper.h"

const int DIR_GPIO = 0;
const int STEP_GPIO = 1;
const double STEPS_FOR_FULL_ROTATION = 360/1.8;
const double MM_PER_FULL_ROTATION = 80;
const double STEPS_PER_MM = MM_PER_FULL_ROTATION / STEPS_FOR_FULL_ROTATION;

void thread_main(int argc, char **argv) {
    ms_sleep(1000);
    printf("Starting\n");
    printf("dir %d step %d steps/mm %f\n", DIR_GPIO, STEP_GPIO, STEPS_PER_MM);
    Stepper *stepper = new Stepper(new GPOutput(DIR_GPIO), new GPOutput(STEP_GPIO), STEPS_PER_MM);
    while (1) {
	printf("high\n");
	stepper->go(72*254);
	printf("low\n");
	stepper->go(0);
    }
}

int main(int argc, char **argv) {
    pi_init_with_threads(thread_main, argc, argv);
}
