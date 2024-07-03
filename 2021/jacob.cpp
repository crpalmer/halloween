#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pi.h"
#include "pi-threads.h"
#include <string.h>
#include "audio.h"
#include "audio-player.h"
#include "mcp23017.h"
#include "maestro.h"
#include "pi-usb.h"
#include "time-utils.h"
#include "random-utils.h"
#include "wav.h"
#include "ween-hours.h"

#define GAUGE 0
#define GAUGE_LOW   10
#define GAUGE_HIGH  70

#define VOLUME 85

static Audio *audio = Audio::create_instance();
static AudioPlayer *player = new AudioPlayer(audio);

static void
gauge(void *unused)
{
    maestro_t *m = maestro_new();
    double pos = (GAUGE_HIGH - GAUGE_LOW) / 2.0;

    maestro_set_servo_range(m, GAUGE, HITEC_HS425);
    while (1) {
	unsigned ms = random_number_in_range(100, 400);
	double new_pos = random_number_in_range(GAUGE_LOW, GAUGE_HIGH) / 1.0;
	double speed = fabs(new_pos - pos) / 100 * ms;
	maestro_set_servo_speed(m, GAUGE, speed);
	maestro_set_servo_pos(m, GAUGE, new_pos);
	pos = new_pos;
	ms_sleep(ms);

	while (! ween_hours_is_valid()) sleep(1);
    }
}

int main(int argc, char **argv)
{
    gpioInitialise();
    seed_random();
    pi_usb_init();

    pi_thread_create("gauge", gauge, NULL);

    AudioBuffer *wav = wav_open("jacob.wav");
    //audio->set_volume(t, VOLUME);

    MCP23017 *mcp = new MCP23017();
    Output *light[8];

    for (int i = 0; i < 8; i++) {
	light[i] = mcp->get_output(i);
        light[i]->set(0);
    }

    while (1) {
	int h = random_number_in_range(0, 15);
	h = h == 0 ? 5 : h < 3 ? 6 : h < 7 ? 7 : 8;

        while (! ween_hours_is_valid()) sleep(1);

	for (int i = 0; i < h; i++) {
	    int flashes = random_number_in_range(2, 5);
	    int mean = 120 / flashes;
	    for (int j = 0; j < flashes; j++) {
		if (! player->is_active()) player->play(wav);
		light[i]->set(1);
		ms_sleep(random_number_in_range(mean-15, mean+15));
		light[i]->set(0);
		ms_sleep(random_number_in_range(mean/2-10, mean/2+10));
	    }
	    ms_sleep(50);
	}
    }
}
