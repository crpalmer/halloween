#ifndef __ANIMATION_ACTIONS_H__
#define __ANIMATION_ACTIONS_H__

#include "animation-lights.h"

typedef void (*action_fn_t)(void *action_data, lights_t *l, unsigned pin);

typedef struct {
    const char *cmd;
    action_fn_t action;
    void    *action_data;
    pthread_mutex_t *lock;
} action_t;

extern action_t actions[];

void actions_init(void);

#endif
