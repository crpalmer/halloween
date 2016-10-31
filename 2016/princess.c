#include <stdio.h>
#include "maestro.h"
#include "pi-usb.h"
#include "util.h"

#define PRINCE_SERVO		0
#define PRINCE_HEAD_MIN		40
#define PRINCE_HEAD_MAX		60

static maestro_t *m;

void
shake_head(void)
{
    int i;

    for (i = 0; i < 3; i++) {
	maestro_set_servo_pos(m, PRINCE_SERVO, 0);
	ms_sleep(200);
	maestro_set_servo_pos(m, PRINCE_SERVO, 100);
	ms_sleep(200);
    }
}

int main(int argc, char **argv)
{
    pi_usb_init();
    m = maestro_new();

    maestro_set_servo_range_pct(m, PRINCE_SERVO, PRINCE_HEAD_MIN, PRINCE_HEAD_MAX);

    while(1) {
	shake_head();
	ms_sleep(1000);
    }
}

