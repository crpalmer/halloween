#ifndef __ANIMATION_LIGHTS_H__
#define __ANIMATION_LIGHTS_H__

typedef struct lightsS lights_t;

typedef struct {
    int bank, pin;
} lights_map_t;

lights_t *lights_new(unsigned min_pin, unsigned max_pin);
lights_t *lights_new_map(const lights_map_t *map, int n);
void lights_chase(lights_t *);
void lights_on(lights_t *);
void lights_off(lights_t *);
void lights_select(lights_t *, unsigned selected);
void lights_blink(lights_t *);
void lights_blink_one(lights_t *l, unsigned pin);

#endif
