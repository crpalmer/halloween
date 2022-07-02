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

#define BAUD	B57600

static struct timespec start;

static int duet;
static char duet_reply[100000];

static int last_x = -1, last_y = -1, last_z = -1;
static int duet_x = 100, duet_y = 100, duet_z = 100;
static int duet_x_state = 0, duet_y_state = 0, duet_z_state = 0;
input_t *forward, *backward, *left, *right, *up, *down, *opening, *closing;

#define SQRT_2 1.41421356237
#define STEP 4
#define EXTRA_STEP (STEP)

#define UPDATE_PERIOD 50
#define MOVE_FEED (STEP*1000*60 / UPDATE_PERIOD)
#define MAX_X 400
#define MAX_Y 360

#define ROUND_MS	(30*1000)

static void
open_duet()
{
    char *path = strdup("/dev/ttyACM0");

    struct termios tty;

    for (int i = 0; i < 10; i++) {
	path[strlen(path) - 1] = i + '0';
        if ((duet = open(path, O_RDWR))  < 0) {
	    perror(path);
	    if (i == 9) exit(1);
	} else {
	    free(path);
	    break;
	}
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
duet_update_position(int feed = 6000)
{
    char cmd[100];

    if (duet_x > MAX_X) duet_x = MAX_X;
    if (duet_x < 0) duet_x = 0;
    if (duet_y > MAX_Y) duet_y = MAX_Y;
    if (duet_y < 0) duet_y = 0;

    if (last_x != duet_x || last_y != duet_y || last_z != duet_z) {
	sprintf(cmd, "G1 X%d Y%d Z%d F%d", duet_x, duet_y, duet_z, feed);
	duet_cmd(cmd, true);
	last_x = duet_x;
	last_y = duet_y;
	last_z = duet_z;
    }
}

static void
calculate_position(int *pos, int *last_move, int this_move)
{
    if (*last_move == 0 && this_move != 0) {
	/* First move, add a step to give it extra room to be able to
	 * accelerate and receive and process the next command if we are
	 * going to keep moving in the same direction.
	 */
	(*pos) += this_move * EXTRA_STEP;
    } else if (*last_move != this_move) {
	/* Switched direction or stopped, cancel the extra step */
	// (*pos) += (*last_move) * -EXTRA_STEP;
    }

    (*pos) += this_move * STEP;
    *last_move = this_move;
}

static void
play_one_round()
{
    struct timespec sleep_until;

    nano_gettime(&start);
    nano_gettime(&sleep_until);

    while (nano_elapsed_ms_now(&start) < ROUND_MS) {
	int move_x = 0, move_y = 0, move_z = 0;

	nano_add_ms(&sleep_until, UPDATE_PERIOD);

	while (! nano_now_is_later_than(&sleep_until)) {
	    if (! forward->get())  move_y = +1;
	    if (! backward->get()) move_y = -1;
	    if (! left->get())     move_x = -1;
	    if (! right->get())    move_x = +1;
	    if (! up->get())       move_z = +1;
	    if (! down->get())     move_z = -1;
	}

//printf("move %d %d || %d %d\n", move_x, move_y, duet_x_state, duet_y_state);

	calculate_position(&duet_x, &duet_x_state, move_x);
	calculate_position(&duet_y, &duet_y_state, move_y);
	calculate_position(&duet_z, &duet_z_state, move_z);

	duet_update_position((move_x && move_y ? SQRT_2 : 1) * MOVE_FEED);

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
    printf("%s\n", duet_cmd("G92 X200 Z0"));
    printf("%s\n", duet_cmd("M201 X20000.00 Y20000.00 Z20000.00"));
    printf("%s\n", duet_cmd("G28 Y"));

    MCP23017 *mcp = new MCP23017();
    forward = mcp->get_input(0, 0);
    backward = mcp->get_input(0, 1);
    left = mcp->get_input(0, 2);
    right = mcp->get_input(0, 3);
    up = mcp->get_input(1, 4);
    down = mcp->get_input(1, 5);
    opening = mcp->get_input(1, 6);
    closing = mcp->get_input(1, 7);

    forward->set_pullup_up();
    backward->set_pullup_up();
    left->set_pullup_up();
    right->set_pullup_up();
    up->set_pullup_up();
    down->set_pullup_up();
    opening->set_pullup_up();
    closing->set_pullup_up();

    while (1) {
	duet_x = MAX_X / 2;
	duet_y = MAX_Y / 2;
	duet_z = 100;
	duet_update_position(12000);
	play_one_round();
	duet_x = duet_y = 0;
	duet_z = 100;
	duet_update_position(6000);
	ms_sleep(5000);
    }
}
