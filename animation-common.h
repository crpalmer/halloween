#ifndef __ANIMATION_COMMON_H__
#define __ANIMATION_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#define LIGHTS_OUTPUT_BANK      1
#define ANIMATION_OUTPUT_BANK   2

#define N_STATION_BUTTONS       5

#include "animation-actions.h"

void animation_main(station_t *);
void animation_main_with_pin0(station_t *, unsigned pin0);

#ifdef __cplusplus
};
#endif

#ifdef __cplusplus
#include "io.h"
void animation_main_with_inputs(station_t *, input_t **inputs);
#endif

#endif
