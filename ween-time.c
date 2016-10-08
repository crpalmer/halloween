#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "time-utils.h"
#include "util.h"
#include "ween-time.h"

#define OCTOBER	9 /* 0 based month number */
#define WEEN_DAY 31

#define MINUTES(hour, minutes) ((hour)*60 + (minutes))

static bool
day_is_valid(struct tm *tm, int within_days_of)
{
    if (within_days_of < 0) return true;
    if (tm->tm_mon != OCTOBER) return false;
    return tm->tm_mday >= WEEN_DAY - within_days_of;
}

int
ween_time_is_valid(ween_time_constraint_t *c, size_t n)
{
    time_t now = time(NULL);
    struct tm tm;
    size_t i;

    if (! localtime_r(&now, &tm)) {
	return true;
    }

    for (i = 0; i < n; i++) {
	if (day_is_valid(&tm, c[i].within_days_of)) {
	    unsigned now = MINUTES(tm.tm_hour, tm.tm_min);
	    unsigned start = MINUTES(c[i].start_h, c[i].start_m);
	    unsigned end = MINUTES(c[i].end_h, c[i].end_m);
	    if (start <= now && now < end) {
		return true;
	    }
	}
    }

    return false;
}

static void
print_constraints(ween_time_constraint_t *c, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++) {
	fprintf(stderr, " %d %d:%02d .. %d:%02d\n", c[i].within_days_of, c[i].start_h, c[i].start_m, c[i].end_h, c[i].end_m);
    }
}

void
ween_time_wait_until_valid(ween_time_constraint_t *c, size_t n)
{
    int logged = false;

    while (! ween_time_is_valid(c, n)) {
	if (! logged) {
	    fprintf(stderr, "Waiting for a valid time:\n");
	    print_constraints(c, n);
	    logged = true;
	}
	ms_sleep(60*1000);
    }

    if (logged) {
	fprintf(stderr, "Ween time constraint met\n");
    }
}
