#include <stdio.h>
#include <stdlib.h>
#include "maestro.h"
#include "pi-usb.h"
#include "talking-skull.h"
#include "time-utils.h"
#include "track.h"
#include "util.h"

#define RECORDER_SERVO 0

#define RECORDER_WAV	"band-recorder.wav"
#define SONG_WAV	"band-song.wav"

static maestro_t *m;

static track_t *song;
static talking_skull_actor_t *recorder;

static struct timespec start;

static void
recorder_up(void)
{
    maestro_set_servo_speed(m, RECORDER_SERVO, 0);
    maestro_set_servo_pos(m, RECORDER_SERVO, 100);
}

static void
recorder_down(void)
{
    maestro_set_servo_speed(m, RECORDER_SERVO, 400);
    maestro_set_servo_pos(m, RECORDER_SERVO, 0);
}

static void
recorder_update(void *unused, double pos)
{
    static int last_is_up = 0;
    static int is_up = 0;
    int cur_is_up = (pos > 50);

    if (cur_is_up && last_is_up != cur_is_up) {
	is_up = ! is_up;
	if (is_up) {
	    recorder_up();
	} else {
	    recorder_down();
	}
    }
    last_is_up = cur_is_up;
}

static void
recorder_init(void)
{
    recorder = talking_skull_actor_new(RECORDER_WAV, recorder_update, NULL);
    if (! recorder) {
	perror(RECORDER_WAV);
	exit(1);
    }

    maestro_set_servo_range_pct(m, RECORDER_SERVO, 25, 35);
    maestro_set_servo_is_inverted(m, RECORDER_SERVO, 1);
    recorder_down();
}

int
main(int argc, char **argv)
{
    pi_usb_init();

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

    while (1) {
	nano_gettime(&start);
	talking_skull_actor_play(recorder);
	track_play(song);
    }

    return 0;
}
