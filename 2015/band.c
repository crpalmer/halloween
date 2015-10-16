#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "pi-usb.h"
#include "maestro.h"
#include "piface.h"
#include "talking-skull.h"
#include "time-utils.h"
#include "track.h"
#include "wav.h"

#define DRUM_LEFT_ID 0
#define DRUM_RIGHT_ID 1
#define SINGER_SERVO_ID 0
#define BANJO_SERVO_ID 1

#define EYES 7

#define INTER_SONG_MS 20000

#define SINGER_GAIN	1
#define SINGER_EYE_ON_PCT 40

#define BANJO_GAIN	2
#define BANJO_MIN_TICKS (6*BANJO_GAIN)
#define BANJO_MAX_TICKS (18*BANJO_GAIN)
#define BANJO_POS_LOW   32
#define BANJO_POS_HIGH  58

#define DRUM_MIN_STATE_MS 150
#define DRUM_P_CORRECTION 6

static struct timespec play_started;

typedef struct {
    wav_t *servo;
    audio_meta_t meta_servo;
    talking_skull_t *talking_skull;
    unsigned char *servo_data;
    size_t n_servo_data;
    servo_operations_t *ops;
} actor_t;

typedef struct {
    maestro_t *m;
    piface_t  *piface;
} singer_t;

typedef struct {
    int up[2];
    struct timespec next[2];
    piface_t *piface;
} drum_t;

typedef struct {
    maestro_t *m;
} banjo_t;

static void
update_singer(void *singer_as_vp, double pos)
{
    singer_t *singer = (singer_t *) singer_as_vp;

    pos *= SINGER_GAIN;
    if (pos > 100) pos = 100;
    maestro_set_servo_pos(singer->m, SINGER_SERVO_ID, pos);
    piface_set(singer->piface, EYES, pos > SINGER_EYE_ON_PCT);
}

static void
update_drum(void *drum_as_vp, double pos)
{
#if 0
    drum_t *drum = (drum_t *) drum_as_vp;
    struct timespec now;
    int which;

    nano_gettime(&now);

    for (which = 1; which < 2; which++) {
	if (nano_later_than(&now, &drum->next[which])) {
	     int delay = random_number_in_range(0, (100-pos)*4) + 200;
	     drum->up[which] = !drum->up[which];
	     piface_set(drum->piface, which, drum->up[which]);
	     drum->next[which] = now;
	     printf("drum %d delay %d\n", which, delay);
	     nano_add_ms(&drum->next[which], delay);
	}
    }
#endif
}

static void
update_banjo(void *banjo_as_vp, double pos)
{
    banjo_t *banjo = (banjo_t *) banjo_as_vp;
    static int n_ticks_left = 1;
    static int is_low = 0;

    if (--n_ticks_left == 0) {
	n_ticks_left = random_number_in_range(BANJO_MIN_TICKS, BANJO_MAX_TICKS) * pos;
	if (n_ticks_left == 0) n_ticks_left = 1;

	is_low = !is_low;
	if (! maestro_set_servo_pos(banjo->m, BANJO_SERVO_ID, is_low ? BANJO_POS_LOW : BANJO_POS_HIGH)) {
	     printf("set_target failed.\n");
	}
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

static void
singer_init(singer_t *singer)
{
    if ((singer->m = maestro_new()) == NULL) {
	fprintf(stderr, "couldn't find a recognized device.\n");
	exit(1);
    }
    maestro_set_servo_is_inverted(singer->m, SINGER_SERVO_ID, 1);
    maestro_set_range(singer->m, SINGER_SERVO_ID, TALKING_SKULL);

    singer->piface = piface_new();
}

static void
banjo_init(banjo_t *banjo)
{
    if ((banjo->m = maestro_new()) == NULL) {
	fprintf(stderr, "couldn't find a recognized device.\n");
	exit(1);
    }
}

static void
drum_init(drum_t *drum, int id)
{
    drum->up[0] = drum->up[1] = 0;
    nano_gettime(&drum->next[0]);
    nano_gettime(&drum->next[1]);
    drum->piface = piface_new();
}

int
main(int argc, char **argv)
{
    track_t *song;
    actor_t a_singer, a_drum, a_banjo;
    singer_t singer;
    banjo_t banjo;
    drum_t drum;

    pi_usb_init();

    singer_init(&singer);
    banjo_init(&banjo);
    drum_init(&drum, DRUM_LEFT_ID);

    song = track_new("under-the-sea.wav");
    if (! song) {
	perror("under-the-sea.wav");
	exit(1);
    }

    actor_init(&a_singer, "under-the-sea-singer.wav", update_singer, &singer);
    actor_init(&a_drum, "under-the-sea-drum.wav", update_drum, &drum);
    actor_init(&a_banjo, "under-the-sea-banjo.wav", update_banjo, &banjo);

    printf("Starting!\n");

    while (true) {
	actor_play(&a_singer);
	actor_play(&a_drum);
	actor_play(&a_banjo);
	nano_gettime(&play_started);
	track_play(song);
	ms_sleep(INTER_SONG_MS);
    }

    return 0;
}
