#include <stdio.h>
#include <stdlib.h>
#include "lights.h"
#include "util.h"
#include "wb.h"

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
    do_lights(NULL);
}
