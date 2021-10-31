#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "fogger.h"
#include "maestro.h"
#include "pi-usb.h"
#include "talker.h"
#include "util.h"
#include "wb.h"
#include "ween-hours.h"

#define FOGGER_PIN	2, 8
#define EYES_PIN	2, 1
#define SERVO		0

static bool
fogger_is_valid()
{
    return ween_hours_is_valid();
}

int
main(int argc, char **argv)
{
    fogger_args_t fogger_args = { 0.025, 0.025, fogger_is_valid };
    talker_args_t talker_args;
    maestro_t *m;

    pi_usb_init();

    if (wb_init() < 0) {
	fprintf(stderr, "failed to initialize wb\n");
	exit(1);
    }

    if ((m = maestro_new()) == NULL) {
	perror("maestro_new");
	exit(2);
    }

    maestro_set_servo_range(m, SERVO, TALKING_SKULL);

    fogger_run_in_background(FOGGER_PIN, &fogger_args);

    talker_args_init(&talker_args);
    talker_args.m = m;
    talker_args.no_input = 1;
    talker_args.eyes = wb_get_output(EYES_PIN);
    talker_args.idle_ms = 60 * 1000;
    talker_args.track_vol = 90;
    
    audio_device_init_playback(&talker_args.out_dev);

    talker_run_in_background(&talker_args);

    while (1) ms_sleep(1000*1000);

    return 0;
}
