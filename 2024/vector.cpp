#include <stdio.h>
#include "pi.h"
#include "i2c.h"
#include "neopixel-pico.h"
#include "ssd1306.h"
#include "time-utils.h"
#include "vector-display.h"

#define N_LEDS 21
const unsigned char brightness[N_LEDS] = {
   90, 90, 90, 90, 90, 90, 90, 90, 90, 90,
   90, 90, 90, 90, 90, 90, 90, 90, 90, 90,
   90
};

NeoPixelPico *neo;

void setup_display() {
    i2c_init_bus(1, 2, 3, 400*1000);
    Display *display = new SSD1306(1);
    Canvas *canvas = display->create_canvas();

    for (int i = 0; vector_display[i]; i++) {
	uint8_t rgb = vector_display[i] != ' ' ? 0xff : 0;
	canvas->set_pixel(i%128, i/128, rgb, rgb, rgb);
    }

    canvas->flush();
}

static void set(int step) {
    for (int i = 0; i < N_LEDS; i++) {
	neo->set_led(i, brightness[i] + step);
    }

    neo->show();
    ms_sleep(40);
}

int main(int argc, char **argv) {
    pi_init();

    setup_display();

    neo = new NeoPixelPico(4);
    neo->set_n_leds(N_LEDS);

    while (true) {
	for (int i = 0; i < 20; i++) set(i);
	for (int i = 19; i >= 0; i--) set(i);
    }
}
