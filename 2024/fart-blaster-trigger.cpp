#include <stdio.h>
#include "pi.h"
#include "time-utils.h"
#include "gp-input.h"
#include "gp-output.h"

int
main(int argc, char **argv) {
    pi_init();

    GPOutput *go = new GPOutput(3);
    GPInput *ready = new GPInput(4);
    ready->set_pullup_up();
    ready->set_is_inverted();

    printf("Waiting for the fart blaster to be ready.\n");
    while (! ready->get()) ms_sleep(1);

    printf("Triggering fart and then waiting for the fart blaster to respond.\n");
    go->on();
    while (ready->get()) ms_sleep(1);

    printf("Stop triggering and waiting for the fart to complete.\n");
    go->off();
    while (! ready->get()) ms_sleep(1);

    exit(0);
}
