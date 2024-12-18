#ifndef __WEEN2018_H__
#define __WEEN2018_H__

#include <sys/stat.h>
#include "time-utils.h"
#include "ween-time.h"

static inline int ween2020_is_ignored()
{
    struct stat s;

    return (stat("/tmp/ween-ignore", &s) >= 0);
}

static bool ween2020_is_valid() {
    static ween_time_constraint_t ween_time_constraints[] = {
        { 0,        15, 00,         22, 00 },
        { 3,        15, 00,         21, 00 },
        { -1,       15, 00,         17, 30 },
        { -1,       18, 30,         20, 00 },
    };
    const int n_ween_time_constraints = sizeof(ween_time_constraints) / sizeof(ween_time_constraints[0]);

    return ween2020_is_ignored() || ween_time_is_valid(ween_time_constraints, n_ween_time_constraints);
}

static inline void ween2020_wait_until_valid()
{
    while (! ween2020_is_valid()) {
	ms_sleep(1000);
    }
}

static inline bool ween2020_is_trick_or_treating()
{
    static ween_time_constraint_t ween_time_constraints[] = {
        { 0,        16, 30,         20, 00 },
    };
    const int n_ween_time_constraints = sizeof(ween_time_constraints) / sizeof(ween_time_constraints[0]);

    return ween_time_is_valid(ween_time_constraints, n_ween_time_constraints);
}

#endif
