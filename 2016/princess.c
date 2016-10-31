#include <stdio.h>
#include "maestro.h"
#include "pi-usb.h"
#include "track.h"
#include "util.h"
#include "wb.h"

#define PRINCE_SERVO		0
#define PRINCE_HEAD_MIN		40
#define PRINCE_HEAD_MAX		60

#define ANVIL_OUTPUT		2,1

#define ANVIL_DROP_MS		1000
#define POST_SHAKE_MS		1000
#define INTER_RUN_MS		10000

static maestro_t *m;
static track_t *save_me_track, *save_you_track, *shake_track;

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

int main(int argc, char **argv)
{
    pi_usb_init();
    wb_init();

    save_me_track = track_new_fatal("princess-save-me.wav");
    save_you_track = track_new_fatal("prince-save-you.wav");
    shake_track = track_new_fatal("prince-shake.wav");

    m = maestro_new();

    maestro_set_servo_range_pct(m, PRINCE_SERVO, PRINCE_HEAD_MIN, PRINCE_HEAD_MAX);

    while(1) {
	track_play(save_me_track);
	ms_sleep(500);
	track_play(save_you_track);
	ms_sleep(500);
	// TODO: princess attack track
	drop_anvil();
	ms_sleep(ANVIL_DROP_MS);
	shake_head();
	ms_sleep(POST_SHAKE_MS);
	retract_anvil();
	ms_sleep(INTER_RUN_MS);
    }
}

