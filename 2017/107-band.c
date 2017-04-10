#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "maestro.h"
#include "pi-usb.h"
#include "talking-skull.h"
#include "time-utils.h"
#include "track.h"
#include "util.h"
#include "wb.h"
#include "107-utils.h"

#define BANJO_SERVO 0
#define BANJO_PCT_LOW	40
#define BANJO_PCT_HIGH	55
#define BANJO_NOTE_MS	150
#define BANJO_INTER_NOTE_LOW_MS 30
#define BANJO_INTER_NOTE_HIGH_MS 100

#define SINGER_SERVO	1
#define SINGER_WHO	TALKING_SKULL2
#define SINGER_EYES	2, 1

#define BETWEEN_SONG_MS	1000

#define BANJO_WAV	"bayou-song.wav"
#define SONG_WAV	"bayou-song.wav"
#define SINGER_WAV	"bayou-servo.wav"

static maestro_t *m;

static track_t *song;
static talking_skull_actor_t *banjo;
static talking_skull_actor_t *singer;

static struct timespec start;

static void
banjo_update(void *unused, double pos)
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
	unsigned delta = random_number_in_range(0, BANJO_INTER_NOTE_HIGH_MS - BANJO_INTER_NOTE_LOW_MS);
	unsigned ms = BANJO_INTER_NOTE_LOW_MS + delta * (100 - pos) / 100;
	nano_add_ms(&next_change, BANJO_NOTE_MS + ms);
	maestro_set_servo_pos(m, BANJO_SERVO, 100);
    } else {
	nano_add_ms(&next_change, BANJO_NOTE_MS);
	maestro_set_servo_pos(m, BANJO_SERVO, 0);
    }
}

static void
banjo_init(void)
{
    banjo = talking_skull_actor_new_with_n_to_avg(BANJO_WAV, banjo_update, NULL, 100);
    if (! banjo) {
	perror(BANJO_WAV);
	exit(1);
    }

    maestro_set_servo_range_pct(m, BANJO_SERVO, BANJO_PCT_LOW, BANJO_PCT_HIGH);
    maestro_set_servo_is_inverted(m, BANJO_SERVO, 1);
    maestro_set_servo_speed(m, BANJO_SERVO, BANJO_NOTE_MS);
}

static void
banjo_rest(void)
{
    maestro_set_servo_pos(m, BANJO_SERVO, 100);
}

static void
singer_update(void *unused, double pos)
{
    static int eyes = -1;
    int new_eyes;

    pos = pos * 3;
    if (pos > 100) pos = 100;

    new_eyes = pos > 40;

    maestro_set_servo_pos(m, SINGER_SERVO, pos);
    if (eyes != new_eyes) {
	wb_set(SINGER_EYES, new_eyes);
	eyes = new_eyes;
    }
}

static void
singer_init(void)
{
    singer = talking_skull_actor_new_with_n_to_avg(SINGER_WAV, singer_update, NULL, 500);
    if (! banjo) {
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

    room_107_die_thread(0);

    if ((m = maestro_new()) == NULL) {
	fprintf(stderr, "Failed to initialize servo controller\n");
	exit(1);
    }

    song = track_new(SONG_WAV);
    if (! song) {
	perror(SONG_WAV);
	exit(1);
    }

    printf("Initializing actors.\n");
    banjo_init();
    singer_init();

    printf("Starting.\n");

    while (true) {
	banjo_rest();
	singer_rest();

	ms_sleep(BETWEEN_SONG_MS);

	nano_gettime(&start);
	talking_skull_actor_play(banjo);
	talking_skull_actor_play(singer);
	track_play(song);
    }

    return 0;
}
