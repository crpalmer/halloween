#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "audio.h"
#include "piface.h"
#include "util.h"
#include "wav.h"

#define BURP_WAV	"burp.wav"

#define BUTTON_INPUT	0
#define BUTTON_LED	7
#define PUMP_OUTPUT	0

#define BURP_PROB	0.2
#define RECHARGE_SECONDS 1
#define PUMP_MS		1500

static piface_t *p;
static bool last_was_burp = false;

static wav_t *burp_wav;
static audio_t *burp_audio;

static void
initialize_audio()
{
    audio_config_t audio_cfg;
    audio_device_t audio_dev;

    burp_wav = wav_new(BURP_WAV);
    if (! burp_wav) {
	perror(BURP_WAV);
	exit(1);
    }

    audio_device_init_playback(&audio_dev);
    audio_config_init_default(&audio_cfg);
    wav_configure_audio(burp_wav, &audio_cfg);
    burp_audio = audio_new(&audio_cfg, &audio_dev);
}

static void
wait_for_trigger()
{
    piface_set(p, BUTTON_LED, true);
    printf("Waiting for button %d: \n", BUTTON_INPUT); fflush(stdout);

    while (! piface_get(p, BUTTON_INPUT)) {}
    piface_set(p, BUTTON_LED, false);
}

static void
burp()
{
    printf("burp.\n");
    wav_play(burp_wav, burp_audio);
    last_was_burp = true;
}

static void
spit()
{
    printf("spit.\n");
    piface_set(p, PUMP_OUTPUT, true);
    ms_sleep(PUMP_MS);
    piface_set(p, PUMP_OUTPUT, false);

    last_was_burp = false;
}

static void
take_action()
{
    if (drand48() < BURP_PROB) {
	if (last_was_burp) {
	    printf("Too many burps, do a spit instead.\n");
	    spit();
	} else {
	    burp();
	}
    } else {
	spit();
    }
}

static void
wait_for_recharge()
{
    while (piface_get(p, BUTTON_INPUT)) {}
    printf("Waiting to recharge\n");
    sleep(RECHARGE_SECONDS);
}

int
main(int argc, char **argv)
{
    srand48(time(NULL));

    initialize_audio();

    p = piface_new();
    while(true) {
	wait_for_trigger();
	take_action();
	wait_for_recharge();
    }

    return 0;
}
