#include <stdio.h>
#include <string.h>
#include "pi.h"
#include "pico-slave.h"

int
main(int argc, char **argv) {
    static char buf[1024];

    pi_init();

    PicoSlave *slave = new PicoSlave();

    while (1) {
	printf("Hit enter to fart:");
	if (fgets(buf, sizeof(buf), stdin) == NULL) break;
	slave->writeline("fart");
	do {
	    if (! slave->readline(buf, sizeof(buf))) {
		printf("failed to get a response from the fart blaster.\n");
		break;
	    }
	    printf("pico says: %s\n", buf);
	} while (strcmp(buf, "done") != 0);
	printf("Ready for next fart.\n");
    }

    exit(0);
}
