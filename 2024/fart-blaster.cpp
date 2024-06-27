#include "pi.h"
#include "neopixel-pico.h"
#include "pi-threads.h"
#include "time-utils.h"

static const int n_leds = 7;
static const int neo_gpio = 28;
static NeoPixelPico *neo;

#define FADE_INC	2
#define FADE_SLEEP_MS	7
#define RING_SLEEP_MS	20
#define ON_VALUE	254
#define OFF_VALUE	50

static void ring_one(int i) {
    neo->set_all(0, 0, 0);
    neo->set_led(0, ON_VALUE, ON_VALUE, ON_VALUE);
    neo->set_led(i, ON_VALUE, ON_VALUE, ON_VALUE);
    neo->show();
}

static void ring() {
    for (int i = 1; i < n_leds; i++) {
	ring_one(i);
	ms_sleep(RING_SLEEP_MS);
    }
}

static void speed_up_ring() {
    for (int it = 0; it < (RING_SLEEP_MS-5)*3; it++) {
	for (int i = 1; i < n_leds; i++) {
	    ring_one(i);
	    ms_sleep(RING_SLEEP_MS - it/3);
	}
    }
}

static void fade_in() {
    int r = OFF_VALUE, g = OFF_VALUE, b = OFF_VALUE;

    while (r < ON_VALUE || g < ON_VALUE || b < ON_VALUE) {
	if (r < ON_VALUE) r += FADE_INC;
	if (g < ON_VALUE) g += FADE_INC;
	if (b < ON_VALUE) b += FADE_INC;
	neo->set_all(r, g, b);
	neo->show();
	ms_sleep(FADE_SLEEP_MS);
    }
}

static void fade_out() {
    int r = ON_VALUE, g = ON_VALUE, b = ON_VALUE;

    while (r > OFF_VALUE || g > OFF_VALUE || b > OFF_VALUE) {
	if (r > OFF_VALUE) r -= FADE_INC;
	if (g > OFF_VALUE) g -= FADE_INC;
	if (b > OFF_VALUE) b -= FADE_INC;
	neo->set_all(r, g, b);
	neo->show();
	ms_sleep(FADE_SLEEP_MS);
    }
}

static void threads_main(int argc, char **argv) {
    neo = new NeoPixelPico(neo_gpio);
    neo->set_n_leds(n_leds);

    neo->set_all(0, 0, 0);
    neo->show();

    while (true) {
	for (int i = 0; i < 5; i++) {
	    speed_up_ring();
	}
	for (int i = 0; i < 50; i++) {
	    ring();
	}
	for (int i = 0; i < 10; i++) {
	    fade_in();
 	    fade_out();
	}
    }
}

int
main(int argc, char **argv)
{
    pi_init_with_threads(threads_main, argc, argv);
}
