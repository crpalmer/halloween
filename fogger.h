#ifndef __FOGGER_H__
#define __FOGGER_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double default_duty;
    double delta_duty;
} fogger_args_t;

void
fogger_run_in_background(unsigned bank, unsigned pin, fogger_args_t *args);

#ifdef __cplusplus
};
#endif

#endif
