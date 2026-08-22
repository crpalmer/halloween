#include <stdio.h>
#include <stdlib.h>
#include "pi.h"
#include "gp-input.h"
#include "gp-output.h"
#include "neopixel-pico.h"
#include "random-utils.h"
#include "util.h"

typedef struct {
    int r, g, b;
} color_t;

static color_t orange = { 223,  56,  25 };
static color_t purple = { 131,  56, 154 };
static color_t red    = { 255,  15,  15 };

#define SLEEP_LOW 20
#define SLEEP_HIGH 100

#define PIN 0
#define N_LEDS 9

static NeoPixelPico *neo;
static const int fire_high = 55;
static const int purple_pct = 0;
static const int red_pct = 12;

static void flicker_led(NeoPixelPico *neo, int led, color_t c, int flicker_high) {
    int r = c.r - random_number_in_range(0, flicker_high);
    int g = c.g - random_number_in_range(0, flicker_high);
    int b = c.b - random_number_in_range(0, flicker_high);

    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;

    neo->set_led(led, r, g, b);
}

static void flicker_fire(NeoPixelPico *neo) {
    for (int i = 0; i < N_LEDS; i++) {
	int pct = random_number_in_range(0, 99);
	color_t c;

	if (pct < purple_pct) c = purple;
	else if (pct < red_pct + purple_pct) c = red;
	else c = orange;

	flicker_led(neo, i, c, fire_high);
    }

#if PRINT_NEOPIXEL_SHOW_TIME
    static us_time_t show_us = 0;
    static int n_shows = 0;
    static int last_s = 0;

    us_time_t start = us_now();
#endif

    neo->show();

#if PRINT_NEOPIXEL_SHOW_TIME
    show_us += us_now() - start;
    n_shows ++;

    if ((int) (start / 1000000) != last_s) {
	last_s = start / 1000000;
	printf("%5d : %.2f ms\n", last_s, show_us / 1000.0 / n_shows);
    }
#endif
}

int
main(int argc, char **argv)
{
    pi_init_no_reboot();

    neo = new NeoPixelPico(PIN);
    neo->set_n_leds(N_LEDS);

    for (;;) {
	flicker_fire(neo);
        ms_sleep(random_number_in_range(SLEEP_LOW, SLEEP_HIGH));
    }
}
