#ifndef __TENTACLE_H__
#define __TENTACLE_H__

#include "maestro.h"

typedef struct {
     int	servo;
     int	mid;
} tentacle_servo_t;

typedef struct {
     const char *name;
     tentacle_servo_t left, right;
     int	delta;
     int	angle_delta;
} tentacle_t;

void
tentacle_goto(tentacle_t *t, maestro_t *m, int deg, double magnitude);

void
tentacle_servo_init(tentacle_servo_t *s, maestro_t *m, int delta);

#endif
