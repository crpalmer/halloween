#ifndef __ANIMATION_ACTIONS_H__
#define __ANIMATION_ACTIONS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "animation-lights.h"

typedef void (*action_fn_t)(void *action_data, lights_t *l, unsigned pin);
typedef void (*waiting_fn_t)(unsigned total_wait_ms);

typedef struct {
    const char *cmd;
    action_fn_t action;
    void    *action_data;
} action_t;

typedef struct {
    bool has_lights;
    action_t *actions;
    pthread_mutex_t *lock;
    waiting_fn_t waiting;
    unsigned waiting_period_ms;
} station_t;

#ifdef __cplusplus
};
#endif

#endif
