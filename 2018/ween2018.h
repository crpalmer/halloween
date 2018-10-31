#ifndef __WEEN2018_H__
#define __WEEN2018_H__

#include <sys/stat.h>
#include "audio.h"
#include "ween-time.h"

static inline int ween2018_is_ignored()
{
    struct stat s;

    return (stat("/tmp/ween-ignore", &s) >= 0);
}

static inline void ween2018_wait_until_valid()
{
    static ween_time_constraint_t ween_time_constraints[] = {
        { 0,        15, 30,         23, 00 },
        { 5,        15, 30,         20, 00 },
        { -1,       15, 30,         17, 00 },
        { -1,       18, 30,         20, 00 },
    };
    const int n_ween_time_constraints = sizeof(ween_time_constraints) / sizeof(ween_time_constraints[0]);

    while (! (ween2018_is_ignored() || ween_time_is_valid(ween_time_constraints, n_ween_time_constraints))) {
	ms_sleep(1000);
    }
}

#endif
