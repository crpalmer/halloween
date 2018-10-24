#ifndef __WEEN_TIME_H__
#define __WEEN_TIME_H__

typedef struct {
    int        within_days_of;
    unsigned   start_h, start_m;
    unsigned   end_h, end_m;
} ween_time_constraint_t;

int ween_time_is_valid(ween_time_constraint_t *constraints, size_t n);

void ween_time_wait_until_valid(ween_time_constraint_t *constraints, size_t n);

static const inline int ween_time_is_halloween() {
    ween_time_constraint_t is_halloween_constraint = { 0, 0, 0, 23, 59 };

    return ween_time_is_valid(&is_halloween_constraint, 1);
}


#endif
