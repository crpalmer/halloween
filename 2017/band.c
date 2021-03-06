#include <stdio.h>
#include <stdlib.h>
#include "maestro.h"
#include "pi-usb.h"
#include "talking-skull.h"
#include "time-utils.h"
#include "track.h"
#include "util.h"
#include "wb.h"

#define MANDOLIN_SERVO 4
#define MANDOLIN_PCT_LOW	32
#define MANDOLIN_PCT_HIGH	58
#define MANDOLIN_NOTE_MS	100
#define MANDOLIN_INTER_NOTE_LOW_MS 150
#define MANDOLIN_INTER_NOTE_HIGH_MS 250

#define SINGER_SERVO	0
#define SINGER_WHO	BAXTER_MOUTH
#define SINGER_LEFT_EYE	1, 5
#define SINGER_RIGHT_EYE 1, 6
#define SINGER_SCALE	2.0

#define HEAD_SERVO	1
#define HEAD_WHO	BAXTER_HEAD
#define HEAD_MS		1000

#define TAIL_SERVO	2
#define TAIL_WHO	BAXTER_TAIL
#define TAIL_MS		500

#define BACKUP_SERVO	3
#define BACKUP_WHO	TALKING_DEER
#define BACKUP_EYES	1, 8
#define BACKUP_SCALE	1.0

#define BETWEEN_SONG_MS	1000

#define SONG_WAV	"monster-mash.wav"
#define SINGER_WAV	"monster-mash-singer.wav"
#define BACKUP_WAV	"monster-mash-backup.wav"

static maestro_t *m;

static track_t *song;
static talking_skull_actor_t *singer;
static talking_skull_actor_t *backup;

static struct timespec start;

static void
mandolin_update(void)
{
    static int is_up = 1;
    static struct timespec next_change = { 0 };
    struct timespec now;

    nano_gettime(&now);
    if (! nano_later_than(&now, &next_change)) return;

    next_change = now;
    is_up = ! is_up;

    if (is_up) {
	unsigned ms = random_number_in_range(MANDOLIN_INTER_NOTE_LOW_MS, MANDOLIN_INTER_NOTE_HIGH_MS);
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
tail_update(void)
{
    static struct timespec next_change = { 0 };
    static int is_high = 0;
    struct timespec now;

    nano_gettime(&now);
    if (nano_later_than(&now, &next_change)) {
	is_high = !is_high;
	next_change = now;
	nano_add_ms(&next_change, TAIL_MS);
	maestro_set_servo_pos(m, TAIL_SERVO, is_high ? 100 : 0);
    }
}

static void
head_update(void)
{
    static struct timespec next_change = { 0 };
    static int is_high = 0;
    struct timespec now;

    nano_gettime(&now);
    if (nano_later_than(&now, &next_change)) {
	is_high = !is_high;
	next_change = now;
	nano_add_ms(&next_change, HEAD_MS);
printf("%d %d\n", HEAD_SERVO, is_high ? 100 : 0);
	maestro_set_servo_pos(m, HEAD_SERVO, is_high ? 100 : 0);
    }
}

static void
singer_update(void *unused, double new_pos)
{
    static int eyes = -1;
    int new_eyes;
    double pos;

    tail_update();
    head_update();
    mandolin_update();

    pos = new_pos * SINGER_SCALE;
    if (pos > 100) pos = 100;
    
    maestro_set_servo_pos(m, SINGER_SERVO, pos);

    new_eyes = pos > 30;
    if (eyes != new_eyes) {
	wb_set(SINGER_LEFT_EYE, new_eyes);
	wb_set(SINGER_RIGHT_EYE, new_eyes);
	eyes = new_eyes;
    }
}

static void
singer_init(void)
{
    singer = talking_skull_actor_new(SINGER_WAV, singer_update, NULL);
    if (! singer) {
	perror(SINGER_WAV);
	exit(1);
    }

    maestro_set_servo_range(m, SINGER_SERVO, SINGER_WHO);
    maestro_set_servo_range(m, TAIL_SERVO, TAIL_WHO);
    maestro_set_servo_range(m, HEAD_SERVO, HEAD_WHO);
    singer_update(NULL, 0);
}

static void
singer_rest(void)
{
    singer_update(NULL, 0);
    maestro_set_servo_pos(m, TAIL_SERVO, 50);
    mandolin_rest();
}

static void
backup_update(void *unused, double new_pos)
{
    static int eyes = -1;
    int new_eyes;
    double pos;

    pos = new_pos > 50 ? 100 : 0;

    maestro_set_servo_pos(m, BACKUP_SERVO, pos);

    new_eyes = pos > 30;
    if (eyes != new_eyes) {
	wb_set(BACKUP_EYES, new_eyes);
	eyes = new_eyes;
    }
}

static void
backup_init(void)
{
    backup = talking_skull_actor_new(BACKUP_WAV, backup_update, NULL);
    if (! backup) {
	perror(BACKUP_WAV);
	exit(1);
    }

    maestro_set_servo_range(m, BACKUP_SERVO, BACKUP_WHO);
    backup_update(NULL, 0);
}

static void
backup_rest(void)
{
    backup_update(NULL, 0);
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

    mandolin_init();
    singer_init();
    backup_init();

    while (1) {
	singer_rest();
	backup_rest();

	ms_sleep(BETWEEN_SONG_MS);

	nano_gettime(&start);
	talking_skull_actor_play(singer);
	talking_skull_actor_play(backup);
	track_play(song);
    }

    return 0;
}
