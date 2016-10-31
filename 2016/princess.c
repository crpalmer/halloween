#include <stdio.h>
#include "maestro.h"
#include "pi-usb.h"
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

    m = maestro_new();

    maestro_set_servo_range_pct(m, PRINCE_SERVO, PRINCE_HEAD_MIN, PRINCE_HEAD_MAX);

    while(1) {
	drop_anvil();
	ms_sleep(ANVIL_DROP_MS);
	shake_head();
	ms_sleep(POST_SHAKE_MS);
	retract_anvil();
	ms_sleep(INTER_RUN_MS);
    }
}

