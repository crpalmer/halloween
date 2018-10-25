#ifndef __WEEN2018_H__
#define __WEEN2018_H__

#include "audio.h"
#include "ween-time.h"
 
static inline void ween2018_wait_until_valid()
{
    static ween_time_constraint_t ween_time_constraints[] = {
        { 0,        12, 00,         23, 00 },
        { 5,        15, 30,         20, 00 },
        { -1,       15, 30,         17, 00 },
        { -1,       18, 30,         20, 00 },
    };
    const int n_ween_time_constraints = sizeof(ween_time_constraints) / sizeof(ween_time_constraints[0]);

    ween_time_wait_until_valid(ween_time_constraints, n_ween_time_constraints);
}

static inline void ween2018_set_volume(track_t *t)
{
    static ween_time_constraint_t ween_time_constraints[] = {
        { 0,        12, 00,         23, 00 },
    };

    if (ween_time_is_valid(ween_time_constraints, 1)) {
	track_set_volume(t, 100);
    } else {
	track_set_volume(t, 50);
    }
}

#endif     
