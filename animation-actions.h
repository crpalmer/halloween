#ifndef __ANIMATION_ACTIONS_H__
#define __ANIMATION_ACTIONS_H__

#include "animation-lights.h"

typedef void (*action_fn_t)(void *action_data, lights_t *l, unsigned pin);

typedef struct {
    const char *cmd;
    action_fn_t action;
    void    *action_data;
} action_t;

typedef struct {
    action_t *actions;
    pthread_mutex_t *lock;
} station_t;

#endif