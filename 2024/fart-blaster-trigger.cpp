#include <stdio.h>
#include "pi.h"
#include "gp-output.h"

int
main(int argc, char **argv) {
    pi_init();
    GPOutput *out = new GPOutput(3);
    out->set(0);
    ms_sleep(1000);
    out->set(1);
    exit(0);
}
