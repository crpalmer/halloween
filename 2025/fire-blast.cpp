#include "pi.h"
#include "neopixel-pico.h"
#include "random-utils.h"
#include "time-utils.h"

typedef struct {
    int r, g, b;
} color_t;

static const color_t orange = { 223,  56,  25 };
const color_t purple = { 131,  56, 154 };
const color_t red    = { 255,  15,  15 };

static const int N_LEDS = 6;

static const int FIRE_HIGH = 55;
static const int PURPLE_PCT = 2;
static const int RED_PCT = 100-2-12;

static const int SLEEP_LOW = 10;
static const int SLEEP_HIGH = 100;

static NeoPixelPico *neo;

static void flicker_led(NeoPixelPico *neo, int led, color_t c, int flicker_high) {
    int r = c.r - random_number_in_range(0, flicker_high);
    int g = c.g - random_number_in_range(0, flicker_high);
    int b = c.b - random_number_in_range(0, flicker_high);

    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;

    neo->set_led(led, r, g, b);
}

int main() {
    pi_init();
    neo = new NeoPixelPico(0);
    neo->set_mode(neopixel_mode_GRB);
    neo->set_n_leds(N_LEDS);
    while (1) {
	for (int i = 0; i < N_LEDS; i++) {
	    int pct = random_number_in_range(0, 99);
	    color_t c;

	    if (pct < PURPLE_PCT) c = purple;
	    else if (pct < RED_PCT + PURPLE_PCT) c = red;
	    else c = orange;

	    flicker_led(neo, i, c, FIRE_HIGH);
	}
	neo->show();
	ms_sleep(random_number_in_range(SLEEP_LOW, SLEEP_HIGH));
    }
}
