#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pi-threads.h"
#include "neopixel-pico.h"
#include "pi.h"
#include "time-utils.h"

static char line[100*1024];

#if 1
#define NEO_PIXEL_GPIO 0
#define N_LEDS_PER_SIDE	18
#define N_LEDS (N_LEDS_PER_SIDE*4)
#else
#define NEO_PIXEL_GPIO 4
#define N_LEDS_PER_SIDE	5
#define N_LEDS (N_LEDS_PER_SIDE*2)
#endif

#define STRNCMP(a, b) strncmp(a, b, strlen(b))

typedef enum {
   off_mode = 0, insert_coin_mode, hit_start_mode, game_mode, time_low_mode,
   time_a_bit_low_mode, time_really_low_mode, game_over_mode, drop_mode
} pico_mode_t;

static pico_mode_t requested_mode = off_mode;
static PiMutex *mutex;

static void inc(NeoPixelPico *neo, int *at)
{
    (*at)++;
    if (*at > N_LEDS_PER_SIDE) *at = 0;
}

#define INSERT_COIN_COLOR	0, 0, 255
#define INSERT_COIN_FADE	0, 0,  32
static int insert_coin_at;

static void set_led_per_side(NeoPixelPico *neo, int at, unsigned char r, unsigned char g, unsigned char b)
{
    for (int side = 0; side < 4; side++) {
	neo->set_led(at+side*N_LEDS_PER_SIDE, r, g, b);
    }
}
static void insert_coin_step(NeoPixelPico *neo)
{
    set_led_per_side(neo, insert_coin_at, INSERT_COIN_FADE);
    neo->show();
    ms_sleep(20);
    set_led_per_side(neo, insert_coin_at, INSERT_COIN_COLOR);
    inc(neo, &insert_coin_at);
}

#define HIT_START_COLOR		0, 255, 0
#define HIT_START_PCT_DELTA	0.01
#define HIT_START_PCT_MIN	0.5

static double hit_start_pct = 1;
static int hit_start_dir = -1;

static void hit_start_step(NeoPixelPico *neo)
{
    hit_start_pct += hit_start_dir * HIT_START_PCT_DELTA;
    if (hit_start_pct < HIT_START_PCT_MIN) {
	hit_start_pct = HIT_START_PCT_MIN;
	hit_start_dir = +1;
    } else if (hit_start_pct > 1) {
	hit_start_pct = 1;
	hit_start_dir = -1;
    }
    neo->set_brightness(hit_start_pct);
    neo->show();
    ms_sleep(10);
}

//#define TIME_LOW_COLOR_1	253, 2, 178 
//#define TIME_LOW_COLOR_2	248, 3, 51
#define TIME_LOW_COLOR_1	253, 2, 2 
#define TIME_LOW_COLOR_2	229, 12, 12
#define TIME_LOW_PCT_DROP	0.05
#define TIME_LOW_PCT_MIN	0.1

#define TIME_LOW_SLEEP		15
#define TIME_REALLY_LOW_SLEEP	10

static double time_low_pct = 1;
static int time_low_sleep = TIME_LOW_SLEEP;

static void time_low_step(NeoPixelPico *neo)
{
    time_low_pct -= TIME_LOW_PCT_DROP;
    if (time_low_pct <= TIME_LOW_PCT_MIN) time_low_pct = 1;
    neo->set_brightness(time_low_pct);
    neo->show();
    ms_sleep(time_low_sleep);
}

#define GAME_COLOR	255, 255, 255
#define GAME_OVER_COLOR	252, 2, 2
#define DROP_COLOR	13, 12, 229

class LightsThread : public PiThread {
public:
    LightsThread() : PiThread("lights-main") { start(); }
    void main(void) override {
	NeoPixelPico *neo = new NeoPixelPico(NEO_PIXEL_GPIO);
	pico_mode_t mode = off_mode;

	neo->set_n_leds(N_LEDS);
	neo->set_all(0, 0, 0);
	neo->show();

	for (;;) {
	    pico_mode_t new_mode;
	    
	    mutex->lock();
	    new_mode = requested_mode;
	    mutex->unlock();

	    if (new_mode != mode) {
		mode = new_mode;
		neo->set_all(0, 0, 0);
		switch(mode) {
		case off_mode:
		    neo->show();
		    break;
		case insert_coin_mode:
		    neo->set_all(INSERT_COIN_COLOR);
		    insert_coin_at = 0;
		    break;
		case hit_start_mode:
		    neo->set_all(HIT_START_COLOR);
		    hit_start_pct = 1;
		    hit_start_dir = -1;
		    break;
		case game_mode:
		    neo->set_all(GAME_COLOR);
		    neo->show();
		    break;
		case time_a_bit_low_mode:
		case time_really_low_mode:
		case time_low_mode:
		    for (int led = 0; led < N_LEDS; led++) {
			if (led % 2 == 0) neo->set_led(led, TIME_LOW_COLOR_1);
			else              neo->set_led(led, TIME_LOW_COLOR_2);
		    }
		    time_low_pct = 1;
		    time_low_sleep = (mode == time_low_mode) ? TIME_LOW_SLEEP : TIME_REALLY_LOW_SLEEP;
		    if (mode == time_a_bit_low_mode) neo->show();
		    break;
		case game_over_mode:
		    neo->set_all(GAME_OVER_COLOR);
		    neo->show();
		    break;
		case drop_mode:
		    neo->set_all(DROP_COLOR);
		    neo->show();
		    break;
		}
	    }

	    switch(mode) {
	    case drop_mode: break;
	    case off_mode: break;
	    case insert_coin_mode: insert_coin_step(neo); break;
	    case hit_start_mode: hit_start_step(neo); break;
	    case game_mode: break;
	    case time_a_bit_low_mode: break;
	    case time_really_low_mode: time_low_step(neo); break;
	    case time_low_mode: time_low_step(neo); break;
	    case game_over_mode: break;
	    }
	}
    }
};

static void
set_new_mode(pico_mode_t new_mode)
{
    mutex->lock();
    requested_mode = new_mode;
    mutex->unlock();
}

static void threads_main(int argc, char **argv) {
    mutex = new PiMutex();
    new LightsThread();

printf("starting\n");

    for (;;) {
	pi_readline(line, sizeof(line));

	int space = 0;
	while (line[space] && line[space] != ' ') space++;

	if (strcmp(line, "bootsel") == 0) {
	    printf("Rebooting into bootloader mode...\n");
	    pi_reboot_bootloader();
	} else if (strcmp(line, "off") == 0) {
	    set_new_mode(off_mode);
	} else if (strcmp(line, "insert-coin") == 0) {
	    set_new_mode(insert_coin_mode);
	} else if (strcmp(line, "hit-start") == 0) {
	    set_new_mode(hit_start_mode);
	} else if (strcmp(line, "game") == 0) {
	    set_new_mode(game_mode);
	} else if (strcmp(line, "time-low") == 0) {
	    set_new_mode(time_low_mode);
	} else if (strcmp(line, "time-a-bit-low") == 0) {
	    set_new_mode(time_a_bit_low_mode);
	} else if (strcmp(line, "time-really-low") == 0) {
	    set_new_mode(time_really_low_mode);
	} else if (strcmp(line, "game-over") == 0) {
	    set_new_mode(game_over_mode);
	} else if (strcmp(line, "drop") == 0) {
	    set_new_mode(drop_mode);
	} else {
	    printf("Usage: bootsel | off | insert-coint | hit-start | game | time-low | time-a-bit-low | time-really-low | game-over | drop\n");
	}
    }
}

int
main(int argc, char **argv)
{
    pi_init_with_threads(threads_main, argc, argv);
}
