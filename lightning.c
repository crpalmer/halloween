#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "track.h"
#include "random-utils.h"
#include "wb.h"
#include "ween-time.h"

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

static ween_time_constraint_t ween_time_constraints[] = {
    { 0,	12, 00,		23, 00 },
    { 3,	17, 00,		23, 00 },
    { -1,	17, 00,		20, 00 },
};

static int n_ween_time_constraints = sizeof(ween_time_constraints) / sizeof(ween_time_constraints[0]);

static ween_time_constraint_t is_halloween_constraint = { 0, 0, 0, 23, 59 };

static int always = 0;

static void
load_tracks(void)
{
    int track;

    for (track = 0; track < N_TRACKS; track++) {
	char fname[500];

	sprintf(fname, "thunder/%d.wav", track);
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
set_flash(bool on)
{
    unsigned bank, pin;

    for (bank = 1; bank <= 2; bank++) {
	for (pin = 1; pin <= 8; pin++) {
	    wb_set(bank, pin, on);
	}
    }
}

int
main(int argc, char **argv)
{
    int last_track = -1;

    if (argc == 2 && strcmp(argv[1], "--always") == 0) {
	always = 1;
    } else if (argc > 1) {
	fprintf(stderr, "usage: [--always]\n");
	exit(1);
    }

    if (wb_init() < 0) {
	fprintf(stderr, "failed to initialize wb\n");
	exit(1);
    }

    set_flash(false);

    load_tracks();

    while (1) {
	int track;
	int bolt;

	if (! always) {
	    ween_time_wait_until_valid(ween_time_constraints, n_ween_time_constraints);
	}

	ms_sleep(random_number_in_range(2000, 5000));

	do {
	    track = random_number_in_range(0, N_TRACKS-1);
	} while (track == last_track);
	last_track = track;

	track_set_volume(tracks[track], ween_time_is_valid(&is_halloween_constraint, 1) ? 100 : 75);

	track_play_asynchronously(tracks[track], NULL);

	for (bolt = 0; bolt < N_BOLTS && bolt_times[track][bolt]; bolt++) {
	    int flashes = random_number_in_range(1, 2);
	    int ms_left = bolt_times[track][bolt];

	    while (flashes-- && ms_left) {
		set_flash(true);
		ms_left -= do_sleep(200, ms_left);
		set_flash(false);
		ms_left -= do_sleep(50, ms_left);
	    }
	    if (ms_left) ms_sleep(ms_left);
	}
    }
}
