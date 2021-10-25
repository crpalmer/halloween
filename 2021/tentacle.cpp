#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include "maestro.h"
#include "pi-usb.h"
#include "tentacle.h"
#include "track.h"
#include "util.h"

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
