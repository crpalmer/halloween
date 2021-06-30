#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "audio.h"
#include "mcp23017.h"
#include "time-utils.h"
#include "track.h"
#include "util.h"

int main(int argc, char **argv)
{
    gpioInitialise();
    seed_random();

    track_t *t = track_new_usb_out("jacob.wav");
    track_play_loop(t, NULL);

    MCP23017 *mcp = new MCP23017();
    output_t *light[8];

    for (int i = 0; i < 8; i++) {
	light[i] = mcp->get_output(i);
        light[i]->set(0);
    }

    while (1) {
	int h = random_number_in_range(0, 15);
	h = h == 0 ? 5 : h < 3 ? 6 : h < 7 ? 7 : 8;
	for (int i = 0; i < h; i++) {
	    int flashes = random_number_in_range(2, 5);
	    int mean = 120 / flashes;
	    for (int j = 0; j < flashes; j++) {
		light[i]->set(1);
		ms_sleep(random_number_in_range(mean-15, mean+15));
		light[i]->set(0);
		ms_sleep(random_number_in_range(mean/2-10, mean/2+10));
	    }
	    ms_sleep(50);
	}
    }
}
