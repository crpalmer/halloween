#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pico/bootrom.h>
#include <tusb.h>
#include "neopixel-pico.h"
#include "pi.h"
#include "time-utils.h"
#include "util.h"

#define FADE_INC	1
#define FADE_SLEEP_MS	10

static void
fade_in(NeoPixelPico **neo, int n_neo, int final_r, int final_g, int final_b)
{
    int r = 0, g = 0, b = 0;

    while (r < final_r || g < final_g || b < final_b) {
	if (r < final_r) r += FADE_INC;
	if (g < final_g) g += FADE_INC;
	if (b < final_b) b += FADE_INC;
	for (int i = 0; i < n_neo; i++) {
	    neo[i]->set_all(r, g, b);
	    neo[i]->show();
	}
	ms_sleep(FADE_SLEEP_MS);
    }
    printf("faded in to %d,%d,%d\n", r, g, b);
}

static void
fade_out(NeoPixelPico **neo, int n_neo, int r, int g, int b)
{
    while (r > 0 || g > 0 || b > 0) {
	if (r > 0) r -= FADE_INC;
	if (g > 0) g -= FADE_INC;
	if (b > 0) b -= FADE_INC;
	for (int i = 0; i < n_neo; i++) {
	    neo[i]->set_all(r, g, b);
	    neo[i]->show();
	}
	ms_sleep(FADE_SLEEP_MS);
    }
    printf("fade out complete.\n");
}

int main(int argc, char **argv)
{
    NeoPixelPico *neo[4];

    pi_init_no_reboot();

    neo[0] = new NeoPixelPico(0);
    neo[0]->set_n_leds(14);
    neo[1] = new NeoPixelPico(1);
    neo[1]->set_n_leds(13);
    neo[2] = new NeoPixelPico(2);
    neo[2]->set_n_leds(8);
    neo[3] = new NeoPixelPico(3);
    neo[3]->set_n_leds(47);

    NeoPixelPico *eyes[2] = { neo[0], neo[1] };
    NeoPixelPico *nose = neo[2];
    NeoPixelPico *eyes_nose[3] = { eyes[0], eyes[1], nose };
    NeoPixelPico *mouth = neo[3];

    const char *names[] = { "left eye", "right eye", "nose", "mouth" };

    for (;;) {
	fade_in(neo, 4, 255, 0, 0);
	ms_sleep(1000);
	fade_out(eyes_nose, 3, 255, 0, 0);
        ms_sleep(500);
	fade_out(&mouth, 1, 255, 0, 0);
	ms_sleep(1000);

	fade_in(neo, 4, 255, 0, 0);
	ms_sleep(1000);
	fade_out(neo, 4, 255, 0, 0);
	ms_sleep(1000);
    }
}
