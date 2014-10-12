#ifndef __LIGHTS_H__
#define __LIGHTS_H__

#include "gpio.h"

typedef struct lightsS lights_t;

lights_t *
lights_new(gpio_table_t *gpios, size_t n_gpios);

void
lights_chase(lights_t *lights);

void
lights_on(lights_t *lights);

void
lights_select(lights_t *lights, const char *name);

void
lights_destroy(lights_t *lights);

#endif
