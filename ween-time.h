#ifndef __WEEN_TIME_H__
#define __WEEN_TIME_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int        within_days_of;
    unsigned   start_h, start_m;
    unsigned   end_h, end_m;
} ween_time_constraint_t;

int ween_time_is_valid(ween_time_constraint_t *constraints, size_t n);

void ween_time_wait_until_valid(ween_time_constraint_t *constraints, size_t n);

#ifdef __cplusplus
};
#endif

#endif
