#include <stdio.h>
#include <stdlib.h>
#include "maestro.h"
#include "pi-usb.h"
#include "talking-skull.h"
#include "time-utils.h"
#include "track.h"
#include "util.h"
#include "wb.h"

#define SINGER_SERVO	0
#define SINGER_LEFT_EYE	1, 5
#define SINGER_RIGHT_EYE 1, 6
#define SINGER_SCALE	1.0

#define BETWEEN_SONG_MS	1000

#define SONG_WAV	"brush-test.wav"
#define SINGER_VSA	"brush-test.csv"

static maestro_t *m;

static track_t *song;
static talking_skull_actor_t *singer;

static struct timespec start;

static void
singer_update(void *unused, double new_pos)
{
    static int eyes = -1;
    int new_eyes;
    double pos;

    pos = new_pos * SINGER_SCALE;
    if (pos > 100) pos = 100;
    
    maestro_set_servo_pos(m, SINGER_SERVO, pos);

    new_eyes = pos < 50;
    if (eyes != new_eyes) {
	wb_set(SINGER_LEFT_EYE, new_eyes);
	wb_set(SINGER_RIGHT_EYE, new_eyes);
	eyes = new_eyes;
    }
}

static void
singer_init(void)
{
    singer = talking_skull_actor_new_vsa(SINGER_VSA, singer_update, NULL);
    if (! singer) {
	exit(1);
    }

    maestro_set_servo_physical_range(m, SINGER_SERVO, 896, 1248);
    singer_update(NULL, 100);
}

static void
singer_rest(void)
{
    singer_update(NULL, 100);
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

    singer_init();

    while (1) {
	singer_rest();

	ms_sleep(BETWEEN_SONG_MS);

	nano_gettime(&start);
	talking_skull_actor_play(singer);
	track_play(song);
    }

    return 0;
}
