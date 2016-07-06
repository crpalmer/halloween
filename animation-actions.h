#ifndef __ANIMATION_ACTIONS_H__
#define __ANIMATION_ACTIONS_H__

typedef void (*action_fn_t)(void *action_data, unsigned pin);

typedef struct {
    const char *cmd;
    action_fn_t action;
    void    *action_data;
    pthread_mutex_t *lock;
} action_t;

extern action_t actions[];

void actions_init(void);

#endif
