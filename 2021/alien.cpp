#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pi-threads.h"
#include "lights.h"
#include "maestro.h"
#include "pi-usb.h"
#include "tentacle.h"
#include "util.h"
#include "wb.h"
#include "ween-hours.h"

maestro_t *m;

#define UPPER "upper"
#define LOWER "lower"

#define LOWER_MAGNITUDE 1
#define UPPER_MAGNITUDE 1

static tentacle_t tentacles[2] = {
    {
	UPPER, { 0, 33}, {1, 43}, 15,  180
    },
    {
	LOWER, { 2, 54}, {3, 47}, 15, -30
    }
};

static void
do_stirring(void *unused)
{
    while (1) {
	while (! ween_hours_is_primetime()) sleep(1);

	for (int deg = 0; deg < 360; deg += 2) {
	    int delta = 0;
	    for (tentacle_t &t : tentacles) {
		tentacle_goto(&t, m, deg + delta, strcmp(t.name, LOWER) == 0 ? LOWER_MAGNITUDE : UPPER_MAGNITUDE);
		delta += 90;
	    }
	    ms_sleep(20);
	}
    }
}

static void
do_lights(void *unused)
{
    class Lights *l[2];
    int last = -1;

    for (int bank = 0; bank < 2; bank++) {
	l[bank] = new Lights();
	for (unsigned i = 0; i < 5; i++) {
	    l[bank]->add(wb_get_output(bank+1, i+1));
	}
    }

    while (1) {
	int low = 2000, high = 8000;
	int action = random_number_in_range(0, 2);

	while (! ween_hours_is_primetime()) sleep(1);

	if (action != last) {
	    for (int bank = 0; bank < 2; bank++) {
		l[bank]->off();
		switch(action) {
		case 0: l[bank]->blink_random(); break;
		case 1: l[bank]->chase(); break;
		case 2: l[bank]->blink_all(); low = 500; high = 1500; break;
		}
	    }
	    ms_sleep(random_number_in_range(low, high));
	    last = action;
	}
    }
}

int main(int argc, char **argv)
{
    wb_init();
    pi_usb_init();

    if ((m = maestro_new()) == NULL) {
	perror("maestro");
	exit(1);

    }

    for (tentacle_t &t : tentacles) tentacle_init(&t, m);

    pi_thread_create_anonymous(do_lights, NULL);
    pi_thread_create_anonymous(do_stirring, NULL);

    while (1) { sleep(60); }
}
