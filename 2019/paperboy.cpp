#include <stdio.h>
#include <stdlib.h>
#include "audio.h"
#include "audio-player.h"
#include "io.h"
#include "pi-gpio.h"
#include "util.h"
#include "wav.h"
#include "wb.h"
#include "ween-time.h"

static ween_time_constraint_t halloween_night = { 0, 5+12, 0, 8+12 };
static ween_time_constraint_t halloween_day   = { 0, 7, 0, 5+12, 0 };

#define NIGHT_DELAY	30
#define DAY_DELAY	(5*60)

static Audio *audio = new AudioPi();
static AudioPlayer *player = new AudioPlayer(audio);

int
main(int argc, char **argv)
{
    Wav *wav;
    input_t *input;
    output_t *output;

    gpioInitialise();
    wb_init();

    input = wb_get_input(1);
    input->set_pullup_down();
    input->set_debounce(100);

    output = wb_get_output(1);
    output->on();

    wav = new Wav(new BufferFile("extra-extra-read-all-about-it.wav"));

    while (true) {
	unsigned delay = 0;

    	if (ween_time_is_valid(&halloween_night, 1)) {
	    delay = NIGHT_DELAY;
	} else if (ween_time_is_valid(&halloween_day, 1)) {
	    delay = DAY_DELAY;
	} else if (input->get()) {
	    delay = 5;
	}
	if (delay > 0) {
	    output->off();
	    player->play_sync(wav->to_audio_buffer());
	    ms_sleep(delay*1000);
	    output->on();
	}
    }
}
