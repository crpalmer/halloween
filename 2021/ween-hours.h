#ifndef __WEEN_HOURS_H__
#define __WEEN_HOURS_H__

#include <sys/stat.h>
#include "audio.h"
#include "ween-time.h"

static inline int ween_hours_is_ignored()
{
    struct stat s;

    return (stat("/tmp/ween-ignore", &s) >= 0);
}

static bool ween_hours_is_primetime() {
    static ween_time_constraint_t ween_time_constraints[] = {
        { 0,        15, 00,         22, 00 },
        { 3,        15, 30,         18, 00 },
        { -1,       15, 30,         16, 30 },
    };
    const int n_ween_time_constraints = sizeof(ween_time_constraints) / sizeof(ween_time_constraints[0]);

    return ween_hours_is_ignored() || ween_time_is_valid(ween_time_constraints, n_ween_time_constraints);
}

static bool ween_hours_is_valid() {
    static ween_time_constraint_t ween_time_constraints[] = {
        { 0,        15, 00,         22, 00 },
        { 3,        15, 00,         21, 00 },
        { -1,       15, 00,         17, 30 },
        { -1,       18, 30,         20, 00 },
    };
    const int n_ween_time_constraints = sizeof(ween_time_constraints) / sizeof(ween_time_constraints[0]);

    return ween_hours_is_ignored() || ween_time_is_valid(ween_time_constraints, n_ween_time_constraints);
}

static inline void ween_hours_wait_until_valid()
{
    while (! ween_hours_is_valid()) {
	ms_sleep(1000);
    }
}

static inline bool ween_hours_is_trick_or_treating()
{
    static ween_time_constraint_t ween_time_constraints[] = {
        { 0,        16, 30,         20, 00 },
    };
    const int n_ween_time_constraints = sizeof(ween_time_constraints) / sizeof(ween_time_constraints[0]);

    return ween_time_is_valid(ween_time_constraints, n_ween_time_constraints);
}

#endif
