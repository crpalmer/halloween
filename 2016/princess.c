#include <stdio.h>
#include <pthread.h>
#include "maestro.h"
#include "pi-usb.h"
#include "time-utils.h"
#include "track.h"
#include "util.h"
#include "wb.h"

#include "animation-actions.h"
#include "animation-common.h"
#include "animation-lights.h"

#define PRINCE_SERVO		0
#define PRINCE_HEAD_MIN		30
#define PRINCE_HEAD_MAX		50

#define ANVIL_OUTPUT		2,1

#define PAUSE_MS		1000
#define ANVIL_DROP_MS		1500
#define SAVE_ME_MS		29000
#define SAVE_ME_REPEAT_MS	14000

static maestro_t *m;
static track_t *save_me_track, *save_you_track, *attack_track, *shake_track, *mean_track;
static stop_t *save_me_stop;
static pthread_mutex_t lock;
static struct timespec last_save_me;

void
shake_head(void)
{
    int i;

    track_play_asynchronously(shake_track, NULL);

    for (i = 0; i < 3; i++) {
	maestro_set_servo_pos(m, PRINCE_SERVO, 0);
	ms_sleep(200);
	maestro_set_servo_pos(m, PRINCE_SERVO, 100);
	ms_sleep(200);
    }
    maestro_set_servo_pos(m, PRINCE_SERVO, 0);
}

static void
drop_anvil(void)
{
    wb_set(ANVIL_OUTPUT, 1);
}

static void
retract_anvil(void)
{
    wb_set(ANVIL_OUTPUT, 0);
}

static void
action(void *unused, lights_t *l, unsigned pin)
{
    struct timespec now;

    lights_off(l);
    stop_stop(save_me_stop);

    nano_gettime(&now);
    if (nano_elapsed_ms(&now, &last_save_me) > SAVE_ME_REPEAT_MS) {
	track_play(save_me_track);
    }

    ms_sleep(PAUSE_MS);
    track_play(save_you_track);
    ms_sleep(PAUSE_MS);
    track_play(attack_track);
    drop_anvil();
    ms_sleep(ANVIL_DROP_MS);
    shake_head();
    ms_sleep(PAUSE_MS);
    track_play(mean_track);
    ms_sleep(PAUSE_MS);
    retract_anvil();
}

static void
play_save_me(unsigned ms_unused)
{
    track_play_asynchronously(save_me_track, save_me_stop);
    nano_gettime(&last_save_me);
}

static action_t actions[] = {
    { "go",	action,		NULL },
    { NULL,	NULL,		NULL },
};

static station_t station[] = {
    { true, actions, &lock, play_save_me, SAVE_ME_MS},
    { 0, }
};

int main(int argc, char **argv)
{
    pi_usb_init();
    wb_init();

    pthread_mutex_init(&lock, NULL);

    save_me_track = track_new_fatal("princess-save-me.wav");
    save_you_track = track_new_fatal("prince-save-you.wav");
    attack_track = track_new_fatal("princess-attack.wav");
    shake_track = track_new_fatal("prince-shake.wav");
    mean_track = track_new_fatal("prince-youre-so-mean.wav");

    m = maestro_new();
    save_me_stop = stop_new();
    stop_stopped(save_me_stop);

    maestro_set_servo_range_pct(m, PRINCE_SERVO, PRINCE_HEAD_MIN, PRINCE_HEAD_MAX);

    animation_main(station);

    return 0;
}

