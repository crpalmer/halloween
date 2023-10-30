#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include "canvas_png.h"
#include "maestro.h"
#include "mcp23017.h"
#include "pi-usb.h"
#include "pico-slave.h"
#include "st7735s.h"
#include "track.h"
#include "util.h"

#define MANUAL_MODE 0

const bool test_offline = false;

#define VENDOR_ID 0x1d50
#define DUET_PRODUCT_ID 0x60ec

#define BAUD	B57600

static struct timespec start;

static int duet = -1;
static char duet_reply[100000];

static int last_x = -1, last_y = -1, last_z = -1;
static int duet_x = 100, duet_y = 100, duet_z = 100;
static int duet_x_state = 0, duet_y_state = 0, duet_z_state = 0;

static MCP23017 *mcp;
static input_t *forward, *backward, *left, *right;
#if MANUAL_MODE
static input_t *up, *down, *opening, *closing;
#endif
static input_t *start_button, *release_button, *coin_acceptor, *coin_override;
static output_t *start_light, *release_light, *coin_acceptor_power;

static ST7735S *display;
static ST7735S_Canvas *canvas;
static CanvasPNG *booting_png, *coin_png, *start_png;

#define CLAW_SERVO 0
#define CLAW_START_POS 25
#define CLAW_GRAB_POS	5
#define CLAW_OPEN_POS	100

static maestro_t *m;

static PicoSlave *pico;

static track_t *claw_music;
static stop_t *claw_music_stop;

static struct {
    const char *name;
    input_t **input;
    int value;
    output_t **output;
} inputs[] = {
    { "forward", &forward, 0, NULL },
    { "backward", &backward, 0, NULL },
    { "left", &left, 0, NULL },
    { "right", &right, 0, NULL },
#if MANUAL_MODE
    { "up", &up, 0, NULL },
    { "down", &down, 0, NULL },
    { "opening", &opening, 0, NULL },
    { "closing", &closing, 0, NULL },
#endif
    { "start", &start_button, 0, &start_light },
    { "release", &release_button, 0, &release_light },
    { "coin acceptor", &coin_acceptor, 0, NULL },
    { "coin override", &coin_override, 0, NULL },
};

const int n_inputs = sizeof(inputs) / sizeof(inputs[0]);

#define SQRT_2 1.41421356237
#define STEP 4
#define EXTRA_STEP (STEP)

#define UPDATE_PERIOD 50
#define MOVE_FEED (STEP*1000*60 / UPDATE_PERIOD)
#define MAX_X 390
#define MAX_Y 360
#define MAX_Z 500
#define START_Z	      100
#define END_OF_GAME_Z 100
// If it's low:
//#define GRAB_Z	      (MAX_Z-50)
// If it's full:
#define GRAB_Z	      (MAX_Z-75)

#define ROUND_MS	(test_offline ? 10*1000 : 15*1000)

static void
display_image(Canvas *img)
{
    canvas->blank();
    canvas->import(img);
    display->paint(canvas);
}

static void
init_lights()
{
    pico = new PicoSlave();
    pico->writeline("off");
}

static void
init_display()
{
    display = new ST7735S();
    canvas = display->create_canvas();

    booting_png = new CanvasPNG("booting.png");
    start_png = new CanvasPNG("hit-start.png");
    coin_png = new CanvasPNG("insert-token.png");

    if (! booting_png->is_valid() || ! coin_png->is_valid() || ! start_png->is_valid()) {
	fprintf(stderr, "Failed to load pngs\n");
	exit(1);
    }

    display_image(booting_png);
}

static void
init_servo()
{
    if ((m = maestro_new()) == NULL) {
	fprintf(stderr, "Failed to initialize the maestro servo controller\n");
	exit(1);
    }

    maestro_set_servo_range_pct(m, CLAW_SERVO, 5, 100);
    maestro_set_servo_pos(m, CLAW_SERVO, CLAW_START_POS);
}

static void
init_joysticks()
{
    left = mcp->get_input(0, 0);
    right = mcp->get_input(0, 1);
    forward = mcp->get_input(0, 2);
    backward = mcp->get_input(0, 3);

    forward->set_pullup_up();
    backward->set_pullup_up();
    left->set_pullup_up();
    right->set_pullup_up();

    forward->set_inverted();
    backward->set_inverted();
    left->set_inverted();
    right->set_inverted();

    forward->set_debounce(1);
    backward->set_debounce(1);
    left->set_debounce(1);
    right->set_debounce(1);

#if MANUAL_MODE
    opening = mcp->get_input(0, 4);
    closing = mcp->get_input(0, 5);
    down = mcp->get_input(0, 6);
    up = mcp->get_input(0, 7);

    up->set_pullup_up();
    down->set_pullup_up();
    opening->set_pullup_up();
    closing->set_pullup_up();

    up->set_inverted();
    down->set_inverted();
    opening->set_inverted();
    closing->set_inverted();

    up->set_debounce(1);
    down->set_debounce(1);
    opening->set_debounce(1);
    closing->set_debounce(1);
#endif
}

static void
init_buttons()
{
    coin_override = mcp->get_input(1, 0);
    coin_acceptor_power = mcp->get_output(1, 1);
    coin_acceptor = mcp->get_input(1, 2);
    start_button = mcp->get_input(1, 4);
    start_light = mcp->get_output(1, 5);
    release_button = mcp->get_input(1, 6);
    release_light = mcp->get_output(1, 7);

    start_button->set_pullup_up();
    release_button->set_pullup_up();
    coin_acceptor->set_pullup_up();
    coin_override->set_pullup_up();

    start_button->set_inverted();
    release_button->set_inverted();
    coin_acceptor->set_inverted();
    coin_override->set_inverted();

    //coin_acceptor->set_debounce(1);
    coin_override->set_debounce(5);
    start_button->set_debounce(50);
    release_button->set_debounce(10);

    coin_acceptor_power->off();
    start_light->off();
    release_light->off();
}

static void
test_inputs()
{
    coin_acceptor_power->on();
    while (1) {
	for (int i = 0; i < n_inputs; i++) {
	    int value = inputs[i].input[0]->get();
	    if (inputs[i].value != value) {
		inputs[i].value = value;
		printf("%s %d\n", inputs[i].name, value);
		if (value && inputs[i].output) {
		    inputs[i].output[0]->on();
		    ms_sleep(1000);
		    inputs[i].output[0]->off();
		}
	    }
	}
    }
}

static void
open_duet()
{
    struct termios tty;

    if ((duet = pi_usb_open_tty(VENDOR_ID, DUET_PRODUCT_ID)) < 0) {
	fprintf(stderr, "Failed to find the duet tty\n");
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

    if (duet < 0) return "no tty";

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
move_claw_to(double pos)
{
    if (m) maestro_set_servo_pos(m, CLAW_SERVO, pos);
}

void
duet_update_position(int feed = 6000)
{
    char cmd[100];

    if (duet_x > MAX_X) duet_x = MAX_X;
    if (duet_x < 0) duet_x = 0;
    if (duet_y > MAX_Y) duet_y = MAX_Y;
    if (duet_y < 0) duet_y = 0;
    if (duet_z > MAX_Z) duet_z = MAX_Z;
    if (duet_z < 0) duet_z = 0;

    if (last_x != duet_x || last_y != duet_y || last_z != duet_z) {
	sprintf(cmd, "G1 X%d Y%d Z%d F%d", duet_x, duet_y, duet_z, feed);
	duet_cmd(cmd, false);
	last_x = duet_x;
	last_y = duet_y;
	last_z = duet_z;
    }
}

static void
duet_wait_for_moves()
{
    duet_cmd("G4 P0");
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
    int last_time_shown = -1;
    struct timespec sleep_until;
    double servo_pos = 50;
    bool z_has_moved = false;
    bool claw_has_moved = false;
    enum { early, low } time_state = early;

    nano_gettime(&start);
    nano_gettime(&sleep_until);

    pico->writeline("game");
    release_light->on();

    stop_reset(claw_music_stop);
    track_play_asynchronously(claw_music, claw_music_stop);

    while (nano_elapsed_ms_now(&start) < ROUND_MS) {
	int move_x = 0, move_y = 0, move_z = 0, move_servo = 0;

	nano_add_ms(&sleep_until, UPDATE_PERIOD);

	while (! nano_now_is_later_than(&sleep_until)) {
	    if (release_button->get()) {
		pico->writeline("drop");
		goto end_of_round;
	    }

	    if (forward->get())  move_y = +1;
	    if (backward->get()) move_y = -1;
	    if (left->get())     move_x = -1;
	    if (right->get())    move_x = +1;
#if MANUAL_MODE
	    if (down->get())     move_z = +1;
	    if (up->get())       move_z = -1;
	    if (opening->get())  move_servo = 1;
	    if (closing->get())  move_servo = -1;
#endif

	    int time_left = (ROUND_MS - nano_elapsed_ms_now(&start)+500)/1000;
	    if (time_left != last_time_shown) {
		canvas->blank();
		canvas->nine_segment_2(time_left);
		display->paint(canvas);
	    }
	}

	servo_pos += move_servo * 2.0;
	if (servo_pos < 0) servo_pos = 0;
	if (servo_pos > 100) servo_pos = 100;
	move_claw_to(servo_pos);

	calculate_position(&duet_x, &duet_x_state, move_x);
	calculate_position(&duet_y, &duet_y_state, move_y);
	calculate_position(&duet_z, &duet_z_state, move_z);

	duet_update_position((move_x && move_y ? SQRT_2 : 1) * MOVE_FEED);

	if (move_z) z_has_moved = true;
	if (move_servo) claw_has_moved = true;

	if (time_state < low && nano_elapsed_ms_now(&start) >= ROUND_MS-5000) {
	    time_state = low;
	    pico->writeline("time-low");
	}
    }

    pico->writeline("game-over");

end_of_round:
    stop_stop(claw_music_stop);

    release_light->off();
    display_image(booting_png);

#if 0
    if (! z_has_moved || ! claw_has_moved || release_needed) {
	/* Do an old style claw game drop */
#endif
    if (! z_has_moved || ! claw_has_moved) {
	move_claw_to(CLAW_OPEN_POS);
    }

    duet_z = GRAB_Z;
    duet_update_position();
    duet_wait_for_moves();

    if (servo_pos > CLAW_GRAB_POS) {
	move_claw_to(CLAW_GRAB_POS);
        ms_sleep(500);
    }
#if 0
    }
#endif

    duet_z = END_OF_GAME_Z;
    duet_update_position();
    duet_x = duet_y = 0;
    duet_update_position();
    duet_wait_for_moves();

    move_claw_to(CLAW_OPEN_POS);
    ms_sleep(500);
    move_claw_to(CLAW_START_POS);

    pico->writeline("off");
}

int main(int argc, char **argv)
{
    gpioInitialise();
    seed_random();
    nano_gettime(&start);

    pi_usb_init();

    mcp = new MCP23017();

    init_display();
    init_joysticks();
    init_buttons();
    if (argc > 1 && strcmp(argv[1], "--test-inputs") == 0) {
	test_inputs();
	exit(0);
    }

    init_lights();

    if (! test_offline) {
	init_servo();
	open_duet();
    }

    claw_music = track_new("claw-music.wav");

    if (! claw_music) exit(1);

    claw_music_stop = stop_new();

    duet_cmd("M201 X20000.00 Y20000.00 Z20000.00");
    duet_cmd("G28 Z");		// get the claw out of the prizes first!
    duet_cmd("G28");

    int n_rounds= 0;

    while (1) {
	duet_x = MAX_X / 2;
	duet_y = MAX_Y / 2;
	duet_z = START_Z;
	duet_update_position(12000);
	duet_wait_for_moves();

	if (! coin_override->get()) {
	    coin_acceptor_power->on();
	    display_image(coin_png);
	    pico->writeline("insert-coin");
	    while (! coin_acceptor->get()) {}
	    coin_acceptor_power->off();
        }

	pico->writeline("hit-start");
	display_image(start_png);
	start_light->on();
	while (! start_button->get()) {}
	start_light->off();

	printf("Starting round %d\n", ++n_rounds);

	play_one_round();

	if (n_rounds >= 10) {
	    duet_z = 2;
	    duet_update_position(12000);
	    duet_cmd("G28 Z");

	    duet_wait_for_moves();
	    n_rounds = 0;
	}
    }
}
