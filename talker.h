#ifndef __TALKER_H__
#define __TALKER_H__

#if 0

#include "audio.h"
#include "io.h"
#include "wb.h"

typedef struct {
    maestro_t *m;
    int servo_id;
    Output *eyes;
    const char *idle_track_prefix;
    unsigned idle_ms;
    audio_device_t out_dev;
    audio_device_t in_dev;
    unsigned port;
    int no_input;
    int	     mic_vol, remote_vol, track_vol;
    bool (*is_valid)(void);
} talker_args_t;

void
talker_args_init(talker_args_t *args);

void
talker_main(void *talker_args);

void
talker_run_in_background(talker_args_t *args);

#endif

#endif
