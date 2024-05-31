#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "neopixel-pico.h"
#include "pi.h"
#include "time-utils.h"

#define N_LEDS 52
#define MS	10
#define LOW	50
#define HIGH	255

int main(int argc, char **argv)
{
    pi_init();

    NeoPixelPico *neo = new NeoPixelPico(0);

    neo->set_n_leds(N_LEDS);

    for (;;) {
	int c = HIGH;
	while (c > LOW) {
	    neo->set_all(c, c, c);
	    neo->show();
	    ms_sleep(MS);
	    c--;
	}
	while (c < HIGH) {
	    neo->set_all(c, c, c);
	    neo->show();
	    ms_sleep(MS);
	    c++;
	}
    }
}
