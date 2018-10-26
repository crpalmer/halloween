#include <stdio.h>
#include <stdlib.h>
#include "maestro.h"
#include "pi-usb.h"
#include "track.h"
#include "util.h"
#include "ween2018.h"

#define ORC_HEAD	0
#define ORC_PERIOD_LOW	5000
#define ORC_PERIOD_HIGH 6000
#define ORC_SPEED_SLOW	2500
#define ORC_DEFAULT_POS	65
#define ORC_TURNED_POS	25
#define ORC_PAUSE_LOW	500
#define ORC_PAUSE_HIGH  1000

int
main(int argc, char **argv)
{
    maestro_t *m;
    track_t *t;

    pi_usb_init();

    if ((m = maestro_new()) == NULL) {
	exit(1);
    }

    if ((t = track_new("warthog.wav")) == NULL) {
	exit(1);
    }

    maestro_set_servo_range(m, ORC_HEAD, HITEC_HS425);
    maestro_set_servo_pos(m, ORC_HEAD, ORC_DEFAULT_POS);

    while (true) {
	ween2018_wait_until_valid();
	ms_sleep(random_number_in_range(ORC_PERIOD_LOW, ORC_PERIOD_HIGH));
	maestro_set_servo_speed(m, ORC_HEAD, 0);
	maestro_set_servo_pos(m, ORC_HEAD, ORC_TURNED_POS);
	ms_sleep(250);
	track_play(t);
	ms_sleep(random_number_in_range(ORC_PAUSE_LOW, ORC_PAUSE_HIGH));
	maestro_set_servo_speed(m, ORC_HEAD, ORC_SPEED_SLOW);
	maestro_set_servo_pos(m, ORC_HEAD, ORC_DEFAULT_POS);
	ms_sleep(ORC_SPEED_SLOW/2);
    }

    return 0;
}
