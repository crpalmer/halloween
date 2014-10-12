#include <stdio.h>
#include <stdlib.h>
#include "maestro.h"
#include "util.h"
#include "pi-usb.h"

#define STEP_MS   5
#define STEP_SIZE (100.0 / 5 / 1000 * STEP_MS)

int
main(int argc, char **argv)
{
    maestro_t *m;
    char buf[10];

    pi_usb_init();
    if ((m = maestro_new()) == NULL) {
	fprintf(stderr, "couldn't find a recognized device.\n");
	exit(1);
    }

#define GO 100
#define STOP 50.5
    maestro_set_servo_pos(m, 0, 0);
    maestro_set_servo_pos(m, 1, STOP);

    while (true) {
	printf("Hit enter to drop: "); fflush(stdout);
	fgets(buf, sizeof(buf), stdin);
	maestro_set_servo_pos(m, 0, 100);
	printf("Hit enter to retract: "); fflush(stdout);
	fgets(buf, sizeof(buf), stdin);
	maestro_set_servo_pos(m, 0, 0);
	sleep(1);
	maestro_set_servo_pos(m, 1, GO);
	sleep(30);
	maestro_set_servo_pos(m, 1, STOP);
    }

    return 0;
}
