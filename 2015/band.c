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
#define SINGER_SERVO_ID 0
#define BANJO_SERVO_ID 1

#define EYES 7

#define INTER_SONG_MS 2000

#define BANJO_GAIN	4
#define BANJO_MIN_TICKS (6*BANJO_GAIN)
#define BANJO_MAX_TICKS (18*BANJO_GAIN)
#define BANJO_POS_LOW   32
#define BANJO_POS_HIGH  58

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
    maestro_set_servo_pos(maestro, SINGER_SERVO_ID, pos);
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
update_banjo(void *unused, double pos)
{
    static int n_ticks_left = 1;
    static int is_low = 0;

    if (--n_ticks_left == 0) {
	n_ticks_left = random_number_in_range(BANJO_MIN_TICKS, BANJO_MAX_TICKS) * pos;
	if (n_ticks_left == 0) n_ticks_left = 1;

	is_low = !is_low;
	if (! maestro_set_servo_pos(maestro, BANJO_SERVO_ID, is_low ? BANJO_POS_LOW : BANJO_POS_HIGH)) {
	     printf("set_target failed.\n");
	}
    }
	
    pos *= gain;
    if (pos > 100) pos = 100;
    maestro_set_servo_pos(maestro, SINGER_SERVO_ID, pos);
    piface_set(piface, EYES, pos > 50);
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
    actor_t singer, a_left, a_right, banjo;
    drum_t left, right;

    pi_usb_init();

    piface = piface_new();
    if ((maestro = maestro_new()) == NULL) {
	fprintf(stderr, "couldn't find a recognized device.\n");
	exit(1);
    }

    maestro_set_servo_is_inverted(maestro, SINGER_SERVO_ID, 1);
    maestro_set_range(maestro, SINGER_SERVO_ID, TALKING_SKULL);

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
    actor_init(&banjo, "under-the-sea-banjo.wav", update_banjo, NULL);

    printf("Starting!\n");

    while (true) {
	actor_play(&singer);
	actor_play(&a_left);
	actor_play(&a_right);
	actor_play(&banjo);
	track_play(song);
	ms_sleep(INTER_SONG_MS);
    }

    return 0;
}
