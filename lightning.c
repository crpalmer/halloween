#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "track.h"
#include "util.h"
#include "wb.h"

#define N_TRACKS	6
#define N_BOLTS		5

static track_t *tracks[N_TRACKS];

static int bolt_times[N_TRACKS][N_BOLTS] = {
    { 1100,  600, 1500,    0,  0 },
    { 1500,    0,    0,    0,  0 },
    {  800,  500, 1600,    0,  0 },
    { 2100, 2200, 2100,  600,  0 },
    { 2200,    0,    0,    0,  0 },
    { 1700, 1400,    0,    0,  0 },
};

#define START_HOUR	16
#define START_MINUTES   15
#define END_HOUR	20
#define END_MINUTES	00
#define HALLOWEEN_END_HOUR 23
#define HALLOWEEN_END_MINUTES 00
#define NORMAL_END	MINUTES(END_HOUR, END_MINUTES)
#define HALLOWEEN_END	MINUTES(HALLOWEEN_END_HOUR, HALLOWEEN_END_MINUTES)

#define MINUTES(hour, minutes) ((hour)*60 + (minutes))

static void
load_tracks(void)
{
    int track;

    for (track = 0; track < N_TRACKS; track++) {
	char fname[500];

	sprintf(fname, "/home/crpalmer/halloween/sound-effects/thunder/%d.wav", track);
	if ((tracks[track] = track_new(fname)) == NULL) {
	    fprintf(stderr, "failed to open %s\n", fname);
	}
    }
}

static int
do_sleep(int ms, int ms_left)
{
    int this_ms = random_number_in_range(50, ms+50);
    if (this_ms > ms_left) this_ms = ms_left;
    ms_sleep(this_ms);
    return this_ms;
}

static void
set_flash(wb_t *wb, bool on)
{
    int i;

    for (i = 0; i < 16; i++) wb_set(wb, i, on);
}

#define OCTOBER	9	/* 0 based month number */
#define WEEN_DAY 31

static bool
is_halloween_within_days(unsigned days)
{
    time_t now = time(NULL);
    struct tm tm;

    if (localtime_r(&now, &tm)) {
	return (tm.tm_mon == OCTOBER && (tm.tm_mday >= WEEN_DAY - days && tm.tm_mday <= WEEN_DAY));
    }
    return false;
}

static bool
is_halloween(void)
{
    return is_halloween_within_days(0);
}

static bool
is_valid_time_of_day(void)
{
    time_t now = time(NULL);
    struct tm tm;

    if (localtime_r(&now, &tm)) {
	unsigned min = MINUTES(tm.tm_hour, tm.tm_min);
	unsigned end = is_halloween() ? HALLOWEEN_END : NORMAL_END;
	return min >= MINUTES(START_HOUR, START_MINUTES) && min < end;
    }
    return true;
}

int
main(int argc, char **argv)
{
    wb_t *wb = wb_new();
    int last_track = -1;

    if (! wb) {
	fprintf(stderr, "failed to initialize wb\n");
	exit(1);
    }

    set_flash(wb, false);

    load_tracks();

    while (1) {
	int track;
	int bolt;

	if (! is_valid_time_of_day()) {
	    printf("Exiting valid time of day, waiting for %d:%02d\n", START_HOUR, START_MINUTES);
	    while (! is_valid_time_of_day()) {
		ms_sleep(60*1000);
	    }
	    printf("Entering valid time of day at %d:%02d\n", END_HOUR, END_MINUTES);
	}

	ms_sleep(random_number_in_range(2000, 5000));

	do {
	    track = random_number_in_range(0, N_TRACKS-1);
	} while (track == last_track);
	last_track = track;

	track_set_volume(tracks[track], is_halloween_within_days(1) ? 100 : 75);

	track_play_asynchronously(tracks[track]);

	for (bolt = 0; bolt < N_BOLTS && bolt_times[track][bolt]; bolt++) {
	    int flashes = random_number_in_range(1, 2);
	    int ms_left = bolt_times[track][bolt];

	    while (flashes-- && ms_left) {
		set_flash(wb, true);
		ms_left -= do_sleep(200, ms_left);
		set_flash(wb, false);
		ms_left -= do_sleep(50, ms_left);
	    }
	    if (ms_left) ms_sleep(ms_left);
	}
    }
}
