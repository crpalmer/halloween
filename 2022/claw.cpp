#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include "mcp23017.h"
#include "util.h"
#include "ween-hours.h"

#define DUET	"/dev/ttyACM0"
#define BAUD	B230400

static int duet;

static void
open_duet()
{
    struct termios tty;

    if ((duet = open(DUET, O_RDWR))  < 0) {
	perror(DUET);
	exit(1);
    }

    if (tcgetattr(duet, &tty) != 0) {
	perror("tcgetattr");
	exit(2);
    }

    cfsetispeed(&tty, BAUD);
    cfsetospeed(&tty, BAUD);
}

int main(int argc, char **argv)
{
    gpioInitialise();
    seed_random();

//    MCP23017 *mcp = new MCP23017();

    open_duet();

    char c;
    write(duet, "M552\n", 5);
    while (read(duet, &c, 1) == 1) {
	putc(c, stdout);
    }

    return 0;
}
