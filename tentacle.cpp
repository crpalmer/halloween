#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pi-threads.h"
#include <math.h>
#include "util.h"

#include "tentacle.h"

void
tentacle_goto(tentacle_t *t, maestro_t *m, int deg, double magnitude)
{
    deg += t->angle_delta;
    while (deg < 0) deg += 360;

    double rad = (deg % 90) * M_PI / 180.0;
    double c = cos(rad);
    double s = sin(rad);
    double p0 = 0, p1 = 0;

    switch ((deg % 360) / 90) {
    case 0: p0 = c; p1 = s; break;
    case 1: p0 = -s; p1 = c; break;
    case 2: p0 = -c; p1 = -s; break;
    case 3: p0 = s; p1 = -c; break;
    }

    p0 = 50 * magnitude * p0 + 50;
    p1 = 50 * magnitude * p1 + 50;

    maestro_set_servo_pos(m, t->left.servo, p0);
    maestro_set_servo_pos(m, t->right.servo, p1);
}

static void
tentacle_servo_init(tentacle_servo_t *s, maestro_t *m, int delta)
{
    maestro_set_servo_range(m, s->servo, SERVO_DS3218);
    maestro_set_servo_range_pct(m, s->servo, s->mid - delta, s->mid + delta);
    maestro_set_servo_pos(m, s->servo, 50);
}

void
tentacle_init(tentacle_t *t, maestro_t *m)
{
    tentacle_servo_init(&t->left, m, t->delta);
    tentacle_servo_init(&t->right, m, t->delta);
}
