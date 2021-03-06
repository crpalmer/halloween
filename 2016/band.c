#include <stdio.h>
#include <stdlib.h>
#include "maestro.h"
#include "pi-usb.h"
#include "talking-skull.h"
#include "time-utils.h"
#include "track.h"
#include "util.h"
#include "wb.h"

#define RECORDER_SERVO_1 0
#define RECORDER_SERVO_1_LOW 27
#define RECORDER_SERVO_1_HIGH 37
#define RECORDER_SERVO_2 1
#define RECORDER_SERVO_2_LOW 45
#define RECORDER_SERVO_2_HIGH 60
#define RECORDER_UP_SPEED	100
#define RECORDER_DOWN_SPEED	500

#define MANDOLIN_SERVO 2
#define MANDOLIN_PCT_LOW	32
#define MANDOLIN_PCT_HIGH	58
#define MANDOLIN_NOTE_MS	150
#define MANDOLIN_INTER_NOTE_LOW_MS 500
#define MANDOLIN_INTER_NOTE_HIGH_MS 1250

#define SINGER_SERVO	3
#define SINGER_WHO	TALKING_DEER
#define SINGER_EYES	1, 1

#define BETWEEN_SONG_MS	1000

#define MANDOLIN_WAV	"band-mandolin.wav"
#define RECORDER_WAV	"band-recorder.wav"
#define SONG_WAV	"band-song.wav"
#define SINGER_WAV	"band-vocals.wav"

static maestro_t *m;

static track_t *song;
static talking_skull_actor_t *mandolin;
static talking_skull_actor_t *recorder;
static talking_skull_actor_t *singer;

static struct timespec start;

static void
recorder_update(void *unused, double pos)
{
    static int last_is_trigger = 0;
    static int state = 0;
    int cur_is_trigger = (pos > 50);

    if (cur_is_trigger && last_is_trigger != cur_is_trigger) {
	int new_state;

	do {
	    new_state = random_number_in_range(0, 3);
	} while (new_state == state);
	state = new_state;
	if (state & 1) {
	    maestro_set_servo_speed(m, RECORDER_SERVO_1, RECORDER_UP_SPEED);
	    maestro_set_servo_pos(m, RECORDER_SERVO_1, 100);
	} else {
	    maestro_set_servo_speed(m, RECORDER_SERVO_1, RECORDER_DOWN_SPEED);
	    maestro_set_servo_pos(m, RECORDER_SERVO_1, 0);
	}
	if (state & 2) {
	    maestro_set_servo_speed(m, RECORDER_SERVO_2, RECORDER_UP_SPEED);
	    maestro_set_servo_pos(m, RECORDER_SERVO_2, 100);
	} else {
	    maestro_set_servo_speed(m, RECORDER_SERVO_2, RECORDER_DOWN_SPEED);
	    maestro_set_servo_pos(m, RECORDER_SERVO_2, 0);
	}
    }
    last_is_trigger = cur_is_trigger;
}

static void
recorder_init(void)
{
    recorder = talking_skull_actor_new(RECORDER_WAV, recorder_update, NULL);
    if (! recorder) {
	perror(RECORDER_WAV);
	exit(1);
    }

    maestro_set_servo_range_pct(m, RECORDER_SERVO_1, RECORDER_SERVO_1_LOW, RECORDER_SERVO_1_HIGH);
    maestro_set_servo_range_pct(m, RECORDER_SERVO_2, RECORDER_SERVO_2_LOW, RECORDER_SERVO_2_HIGH);
    maestro_set_servo_is_inverted(m, RECORDER_SERVO_1, 1);
}

static void
recorder_rest(void)
{
    maestro_set_servo_speed(m, RECORDER_SERVO_1, RECORDER_DOWN_SPEED);
    maestro_set_servo_pos(m, RECORDER_SERVO_1, 0);
    maestro_set_servo_speed(m, RECORDER_SERVO_2, RECORDER_DOWN_SPEED);
    maestro_set_servo_pos(m, RECORDER_SERVO_2, 0);
}

static void
mandolin_update(void *unused, double pos)
{
    static int is_up = 1;
    static struct timespec next_change = { 0 };
    struct timespec now;

    nano_gettime(&now);
    if (pos < 5) return;
    if (! nano_later_than(&now, &next_change)) return;

    next_change = now;
    is_up = ! is_up;

    if (is_up) {
	unsigned delta = random_number_in_range(0, MANDOLIN_INTER_NOTE_HIGH_MS - MANDOLIN_INTER_NOTE_LOW_MS);
	unsigned ms = MANDOLIN_INTER_NOTE_LOW_MS + delta * (100 - pos) / 100;
	nano_add_ms(&next_change, MANDOLIN_NOTE_MS + ms);
	maestro_set_servo_pos(m, MANDOLIN_SERVO, 100);
    } else {
	nano_add_ms(&next_change, MANDOLIN_NOTE_MS);
	maestro_set_servo_pos(m, MANDOLIN_SERVO, 0);
    }
}

static void
mandolin_init(void)
{
    mandolin = talking_skull_actor_new(MANDOLIN_WAV, mandolin_update, NULL);
    if (! mandolin) {
	perror(MANDOLIN_WAV);
	exit(1);
    }

    maestro_set_servo_range_pct(m, MANDOLIN_SERVO, MANDOLIN_PCT_LOW, MANDOLIN_PCT_HIGH);
    maestro_set_servo_is_inverted(m, MANDOLIN_SERVO, 1);
    maestro_set_servo_speed(m, MANDOLIN_SERVO, MANDOLIN_NOTE_MS);
}

static void
mandolin_rest(void)
{
    maestro_set_servo_pos(m, MANDOLIN_SERVO, 100);
}

static void
singer_update(void *unused, double pos)
{
    static int eyes = -1;
    int new_eyes = pos > 30;

    maestro_set_servo_pos(m, SINGER_SERVO, pos);
    if (eyes != new_eyes) {
	wb_set(SINGER_EYES, new_eyes);
	eyes = new_eyes;
    }
}

static void
singer_init(void)
{
    singer = talking_skull_actor_new(SINGER_WAV, singer_update, NULL);
    if (! mandolin) {
	perror(SINGER_WAV);
	exit(1);
    }

    maestro_set_servo_range(m, SINGER_SERVO, SINGER_WHO);
}

static void
singer_rest(void)
{
    singer_update(NULL, 0);
}

int
main(int argc, char **argv)
{
    pi_usb_init();
    wb_init();

    if ((m = maestro_new()) == NULL) {
	fprintf(stderr, "Failed to initialize servo controller\n");
	exit(1);
    }

    song = track_new(SONG_WAV);
    if (! song) {
	perror(SONG_WAV);
	exit(1);
    }

    recorder_init();
    mandolin_init();
    singer_init();

    while (1) {
	recorder_rest();
	mandolin_rest();
	singer_rest();

	ms_sleep(BETWEEN_SONG_MS);

	nano_gettime(&start);
	talking_skull_actor_play(recorder);
	talking_skull_actor_play(mandolin);
	talking_skull_actor_play(singer);
	track_play(song);
    }

    return 0;
}
