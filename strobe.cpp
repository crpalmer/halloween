#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "random-utils.h"
#include "wb.h"

int
main(int argc, char **argv)
{
    int state = 0;

    pi_init();
    seed_random();
    wb_init();

    while (1) {
	ms_sleep(random_number_in_range(50, 100));
	state = ~state;
	wb_set_outputs(0xff, state);
    }
}

