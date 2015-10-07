#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "pi-usb.h"
#include "maestro.h"
#include "piface.h"
#include "talking-skull.h"
#include "track.h"
#include "wav.h"

#define DRUM_LEFT_ID 2
#define DRUM_RIGHT_ID 3
#define SERVO_ID 0

#define EYES 7
#define INTER_SONG_MS 2000

static double gain = 1;
static maestro_t *maestro;
static piface_t *piface;

typedef struct {
    wav_t *servo;
    audio_meta_t meta_servo;
    talking_skull_t *talking_skull;
    unsigned char *servo_data;
    size_t n_servo_data;
    servo_operations_t *ops;
} actor_t;

typedef struct {
    int id;
    int up;
    piface_t *piface;
} drum_t;

static void
update_singer(void *unused, double pos)
{
    pos *= gain;
    if (pos > 100) pos = 100;
    maestro_set_servo_pos(maestro, SERVO_ID, pos);
    piface_set(piface, EYES, pos > 50);
}

static void
update_drum(void *drum_as_vp, double pos)
{
    drum_t *drum = (drum_t *) drum_as_vp;
    int this_up = pos > 0.5;

    if (this_up != drum->up) {
	drum->up = this_up;
	piface_set(drum->piface, drum->id, drum->up);
	printf("drum %d @ %d\n", drum->id, drum->up);
    }
}

static void
actor_init(actor_t *a, const char *fname, talking_skull_servo_update_t update, void *data)
{
    printf("Loading %s\n", fname);
    a->servo = wav_new(fname);
    if (! a->servo) {
	perror(fname);
	exit(1);
    }

    a->meta_servo = wav_get_meta(a->servo);
    a->servo_data = wav_get_raw_data(a->servo, &a->n_servo_data);
    a->talking_skull = talking_skull_new(&a->meta_servo, true, update, data);
    a->ops = talking_skull_prepare(a->talking_skull, a->servo_data, a->n_servo_data);
}

static void
actor_play(actor_t *a)
{
    talking_skull_play_prepared(a->talking_skull, a->ops);
}

int
main(int argc, char **argv)
{
    track_t *song;
    actor_t singer, a_left, a_right;
    drum_t left, right;

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

    left.id = DRUM_LEFT_ID;
    left.up = 0;
    left.piface = piface_new();
    right.id = DRUM_RIGHT_ID;
    right.up = 0;
    right.piface = piface_new();

    actor_init(&singer, "under-the-sea-singer.wav", update_singer, NULL);
    actor_init(&a_left, "under-the-sea-drum-left.wav", update_drum, &left);
    actor_init(&a_right, "under-the-sea-drum-right.wav", update_drum, &right);

    printf("Starting!\n");

    while (true) {
	actor_play(&singer);
	actor_play(&a_left);
	actor_play(&a_right);
	track_play(song);
	ms_sleep(INTER_SONG_MS);
    }

    return 0;
}
