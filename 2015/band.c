#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "pi-usb.h"
#include "maestro.h"
#include "piface.h"
#include "talking-skull.h"
#include "track.h"
#include "wav.h"

#define SERVO_ID 0

#define EYES 7
#define INTER_SONG_MS 2000

static double gain = 1;
static maestro_t *maestro;
static piface_t *piface;

static void
update_servo(void *unused, double pos)
{
    pos *= gain;
    if (pos > 100) pos = 100;
    maestro_set_servo_pos(maestro, SERVO_ID, pos);
    piface_set(piface, EYES, pos > 50);
}

int
main(int argc, char **argv)
{
    track_t *song;
    wav_t *servo;
    audio_meta_t meta_servo;
    talking_skull_t *talking_skull;
    unsigned char *servo_data;
    size_t n_servo_data;

    pi_usb_init();

    piface = piface_new();
    if ((maestro = maestro_new()) == NULL) {
	fprintf(stderr, "couldn't find a recognized device.\n");
	exit(1);
    }

    maestro_set_servo_is_inverted(maestro, SERVO_ID, 1);
    maestro_set_range(maestro, SERVO_ID, TALKING_SKULL);

    while (argc > 1 && argv[1][0] == '-' && argv[1][1] == '-') {
	if (strcmp(argv[1], "--") == 0) {
	    argc -= 1;
	    argv += 1;
	    break;
	}
	if (argc > 2 && strcmp(argv[1], "--gain") == 0) {
	    gain = atof(argv[2]);
	    argc -= 2;
	    argv += 2;
	} else {
	    fprintf(stderr, "usage: [--gain X] audio.wav servo.wav\n");
	}
    }

    song = track_new("under-the-sea.wav");
    if (! song) {
	perror("under-the-sea.wav");
	exit(1);
    }

    servo = wav_new("under-the-sea-singer.wav");
    if (! servo) {
	perror("under-the-sea-singer.wav");
	exit(1);
    }

    meta_servo = wav_get_meta(servo);
    servo_data = wav_get_raw_data(servo, &n_servo_data);
    talking_skull = talking_skull_new(&meta_servo, true, update_servo, NULL);

    while (true) {
	talking_skull_play(talking_skull, servo_data, n_servo_data);
	track_play(song);
	ms_sleep(INTER_SONG_MS);
    }

    return 0;
}
