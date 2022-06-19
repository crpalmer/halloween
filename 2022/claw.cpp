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
#define BAUD	B57600

static struct timespec start;

static int duet;
static char duet_reply[100000];

static int last_x = -1, last_y = -1, last_z = -1;
static int duet_x = 100, duet_y = 100, duet_z = 100;
input_t *up, *down, *left, *right;

#define STEP 10
#define MOVE_FEED (STEP*10*60)
#define MAX_X 230
#define MAX_Y 230

#define ROUND_MS	(30*1000)

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

#if 1
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
duet_cmd(const char *cmd, bool echo = true)
{
    int len = 0, got;

    if (echo) printf("%5d %s\n", nano_elapsed_ms_now(&start), cmd);
    write(duet, cmd, strlen(cmd));
    write(duet, "\n", 1);
    while ((got = read(duet, &duet_reply[len], sizeof(duet_reply) - len)) > 0) {
        len += got;
 	duet_reply[len] = '\0';
	if ((len == 3 && strcmp(&duet_reply[len-3], "ok\n") == 0) ||
	    (len  > 3 && strcmp(&duet_reply[len-4], "\nok\n") == 0)) {
	    if (echo) printf("%5d %s", nano_elapsed_ms_now(&start), duet_reply);
	    duet_reply[len-1] = '\0';
	    return duet_reply;
	}
    }

    if (echo) printf("duet_cmd didn't return a complete response.\n");
    return NULL;
}

void
duet_update_position(int feed = 600)
{
    char cmd[100];

    if (duet_x > MAX_X) duet_x = MAX_X;
    if (duet_x < 0) duet_x = 0;
    if (duet_y > MAX_Y) duet_y = MAX_Y;
    if (duet_y < 0) duet_y = 0;

    if (last_x != duet_x || last_y != duet_y || last_z != duet_z) {
	sprintf(cmd, "G1 X%d Y%d Z%d F%d", duet_x, duet_y, duet_z, feed);
	duet_cmd(cmd, false);
	last_x = duet_x;
	last_y = duet_y;
	last_z = duet_z;
    }
}

static void
play_one_round()
{
    nano_gettime(&start);

    while (nano_elapsed_ms_now(&start) < ROUND_MS) {
	struct timespec sleep_until;

	nano_gettime(&sleep_until);
	nano_add_ms(&sleep_until, 100);

	if (up->get()) duet_y -= STEP;
	if (down->get()) duet_y += STEP;
	if (left->get()) duet_x -= STEP;
	if (right->get()) duet_x += STEP;

	duet_update_position(MOVE_FEED);

 	nano_sleep_until(&sleep_until);
    }
}

int main(int argc, char **argv)
{
    gpioInitialise();
    seed_random();

    nano_gettime(&start);

    open_duet();

    //printf("INIT: %s\n-----\n", duet_cmd(""));
//    printf("%s\n", duet_cmd("M552"));
    printf("%s\n", duet_cmd("M122"));
    printf("%s\n", duet_cmd("G92 X100 Y100 Z100"));

    MCP23017 *mcp = new MCP23017();
    up = mcp->get_input(0, 0);
    down = mcp->get_input(0, 1);
    left = mcp->get_input(0, 2);
    right = mcp->get_input(0, 3);

    up->set_pullup_up();
    down->set_pullup_up();
    left->set_pullup_up();
    right->set_pullup_up();

    while (1) {
	duet_x = duet_y = duet_z = 100;
	duet_update_position(12000);
	play_one_round();
	duet_x = duet_y = 230;
	duet_z = 100;
	duet_update_position(6000);
	ms_sleep(1000);
    }
}
