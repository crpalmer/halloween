#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "lights.h"
#include "maestro.h"
#include "pi-usb.h"
#include "tentacle.h"
#include "util.h"
#include "wb.h"

maestro_t *m;

static tentacle_t tentacles[2] = {
    {
	"upper", { 0, 33}, {1, 43}, 15,  180
    },
    {
	"lower", { 2, 54}, {3, 47}, 15, -30
    }
};

static void *
do_stirring(void *unused)
{
    while (1) {
	for (int deg = 0; deg < 360; deg += 2) {
	    int delta = 0;
	    for (tentacle_t &t : tentacles) {
		tentacle_goto(&t, m, deg + delta, 1);
		delta += 90;
	    }
	    ms_sleep(20);
	}
    }

    return NULL;
}

static void *
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

    return NULL;
}

int main(int argc, char **argv)
{
    pthread_t lights, stirring;

    wb_init();
    pi_usb_init();
    if ((m = maestro_new()) == NULL) {
	perror("maestro");
	exit(1);

    }

    pthread_create(&lights, NULL, do_lights, NULL);
    pthread_create(&stirring, NULL, do_stirring, NULL);

    while (1) { sleep(60); }
}
