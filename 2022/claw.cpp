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
static char duet_reply[100000];

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

#if 0
    cfmakeraw(&tty);
#else
    tty.c_cflag     &=  ~PARENB;            // No Parity
    tty.c_cflag     &=  ~CSTOPB;            // 1 Stop Bit
    tty.c_cflag     &=  ~CSIZE;
    tty.c_cflag     |=  CS8;                // 8 Bits
#endif
    tty.c_cc[VMIN]   = 1;		   // wait for atleast 1 byte
    tty.c_cc[VTIME]  = 0;		   // blocking

    tcsetattr(duet, TCSAFLUSH, &tty);

    cfsetispeed(&tty, BAUD);
    cfsetospeed(&tty, BAUD);

    //int flags = fcntl(duet, F_GETFL, 0);
    //fcntl(duet, F_SETFL, flags & (~O_NONBLOCK));

    for (int i = 0; i < 5; i++) {
	write(duet, "\n", 1);
	ms_sleep(100);
        tcflush(duet, TCIOFLUSH);
    }
}

const char *
duet_cmd(const char *cmd)
{
    int len = 0, got;

    write(duet, cmd, strlen(cmd));
    write(duet, "\n", 1);
    while ((got = read(duet, &duet_reply[len], sizeof(duet_reply) - len)) > 0) {
        len += got;
 	duet_reply[len] = '\0';
	if ((len == 3 && strcmp(&duet_reply[len-3], "ok\n") == 0) ||
	    (len  > 3 && strcmp(&duet_reply[len-4], "\nok\n") == 0)) {
	    return duet_reply;
	}
    }

duet_reply[len] = 0;
fprintf(stderr, "partial reply: %s\n", duet_reply);
    return NULL;
}

int main(int argc, char **argv)
{
    gpioInitialise();
    seed_random();

//    MCP23017 *mcp = new MCP23017();

    open_duet();

    //printf("INIT: %s\n-----\n", duet_cmd(""));
//    printf("%s\n", duet_cmd("M122"));
    printf("%s\n", duet_cmd("G92 X0 Y0 Z0"));
    printf("%s\n", duet_cmd("G1 X100 F600"));

    return 0;
}
