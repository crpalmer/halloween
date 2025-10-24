#include "pi.h"
#include "riser-prop.h"

void RiserProp::test_es(const char *name, Input *es) {
    if (es->get()) printf("  *** %s ENDSTOP ALREADY TRIGGERED!\n", name);
    printf("Trigger %s end-stop: ", name); fflush(stdout);
    while (! es->get()) {}
    printf("triggered\n");
}

void RiserProp::test() {
    test_es("low", low_es);
    test_es("high", high_es);

    char buf[1000];

    printf("center the motor: "); fflush(stdout);
    pi_readline(buf, sizeof(buf));
    printf("motor up\n");
    up();
    ms_sleep(2*1000);
    printf("motor down\n");
    down();
    ms_sleep(2*1000);
    motor->stop();
}

void RiserProp::up() {
    motor->change_motor(true, 1);
    wait_for_es(high_es, maximum_up_ms());
    motor->stop();
}

void RiserProp::down() {
    motor->change_motor(false, 1);
    wait_for_es(low_es, maximum_down_ms());
    motor->stop();
}

void RiserProp::wait_for_es(Input *es, int max_ms) {
    struct timespec start;
    nano_gettime(&start);
    while (nano_elapsed_ms_now(&start) < max_ms && ! es->get()) {}
}
