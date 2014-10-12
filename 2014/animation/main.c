#include <stdio.h>
#include "piface.h"
#include "util.h"
#include "animation/lights.h"
#include "animation/names.h"

#define SPIDER_ID   0
#define ZOMBIE_ID   1
#define GATER_ID    2
#define SNAKE_ID    3
#define QUESTION_ID 4

#define N_BUTTONS 5

#if 0
static gpio_table_t lights_gpio_table[N_BUTTONS] = {
    [SPIDER_ID]   = { SPIDER,    5, 1 },
    [ZOMBIE_ID]   = { ZOMBIE,    6, 1 },
    [GATER_ID]    = { GATER,    13, 1 },
    [SNAKE_ID]    = { SNAKE,    19, 1 },
    [QUESTION_ID] = { QUESTION, 26, 1},
};
#endif

#define N_LIGHTS_GPIO_TABLE (sizeof(lights_gpio_table) / sizeof(lights_gpio_table[0]))

int
main(int argc, char **argv)
{
    lights_t *lights;
    piface_t *piface;
    unsigned i, button;

    piface = piface_new();
    lights = lights_new(piface);

    while (true) {
	lights_chase(lights);
	button = piface_wait_for_input(piface);
printf("button = %x\n", button);
	for (i = 0; i < N_BUTTONS; i++) {
	    if (PIFACE_IS_SELECTED(button, i)) {
		lights_select(lights, i);
		//do_prop(i);
	    }
	}
	ms_sleep(1000);
    }

    lights_destroy(lights);
    piface_destroy(piface);

    return 0;
}
