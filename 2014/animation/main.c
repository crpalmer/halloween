#include <stdio.h>
#include "util.h"
#include "animation/lights.h"
#include "animation/names.h"

#define SPIDER_ID   0
#define ZOMBIE_ID   1
#define GATER_ID    2
#define NAKE_ID     3
#define QUESTION_ID 4

static gpio_table_t lights_gpio_table[5] = {
    [SPIDER_ID]   = { SPIDER,    5, 1 },
    [ZOMBIE_ID]   = { ZOMBIE,    6, 1 },
    [GATER_ID]    = { GATER,    13, 1 },
    [SNAKE_ID]    = { SNAKE,    19, 1 },
    [QUESTION_ID] = { QUESTION, 26, 1},
};

#define N_LIGHTS_GPIO_TABLE (sizeof(lights_gpio_table) / sizeof(lights_gpio_table[0]))

int
main(int argc, char **argv)
{
    lights_t *lights;

    lights = lights_new(lights_gpio_table, N_LIGHTS_GPIO_TABLE);
    while (true) {
	lights_chase(lights);
	ms_sleep(1000);
	lights_on(lights);
	ms_sleep(1000);
    }

    return 0;
}
