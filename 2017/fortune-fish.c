#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "animation-actions.h"
#include "animation-common.h"
#include "track.h"
#include "util.h"
#include "wb.h"

#define HAND_PIN  1

#define FLASHING_LIGHTS_MASK 0x0f1f
#define FLASHING_LIGHTS_MIN_MS 2000
#define FLASHING_LIGHTS_MAX_MS 3500

#define FLASHING_LIGHTS_FLASH_MIN_MS 100
#define FLASHING_LIGHTS_FLASH_MAX_MS 150

#define NUM_BLINKS	5

static const struct {
    int bank, pin;
} chase_map[] = {
    {1, 1}, {1, 2}, {1, 3}, {1, 4},
    {1, 5},
    {2, 4}, {2, 3}, {2, 2}, {2, 1}
};
static const int n_chase_map = sizeof(chase_map) / sizeof(chase_map[0]);

static int chase_i;

static void flash_lights(int ms)
{
    int blink;

    fprintf(stderr, "  Flashing lights %d ms\n", ms);

    while (ms > 0) {
	unsigned sleep_ms = random_number_in_range(FLASHING_LIGHTS_FLASH_MIN_MS, FLASHING_LIGHTS_FLASH_MAX_MS);
	unsigned state = random_number_in_range(0, FLASHING_LIGHTS_MASK);

	wb_set_outputs(FLASHING_LIGHTS_MASK, state);
	ms_sleep(sleep_ms);
	ms -= sleep_ms;
    }
    for (blink = 0; blink <= NUM_BLINKS; blink++) {
	wb_set_outputs(FLASHING_LIGHTS_MASK, FLASHING_LIGHTS_MASK);
	if (blink < NUM_BLINKS) {
	    ms_sleep(random_number_in_range(FLASHING_LIGHTS_FLASH_MAX_MS, FLASHING_LIGHTS_FLASH_MAX_MS*2));
	    wb_set_outputs(FLASHING_LIGHTS_MASK, 0);
	    ms_sleep(random_number_in_range(FLASHING_LIGHTS_FLASH_MAX_MS, FLASHING_LIGHTS_FLASH_MAX_MS*2));
	}
    }
}

static void fortune(void *unused, lights_t *l, unsigned pin)
{
    fprintf(stderr, "Fortune time!\n");

    flash_lights(random_number_in_range(FLASHING_LIGHTS_MIN_MS, FLASHING_LIGHTS_MAX_MS));

    ms_sleep(2000);

    fprintf(stderr, "  Done.\n");
    wb_set_outputs(FLASHING_LIGHTS_MASK, 0);

    chase_i = 0;
}

static void waiting_for_button(unsigned ms)
{
    int last = chase_i ? chase_i - 1 : n_chase_map-1;

    wb_set(chase_map[last].bank, chase_map[last].pin, 0);
    wb_set(chase_map[chase_i].bank, chase_map[chase_i].pin, 1);

    chase_i = (chase_i + 1) % n_chase_map;
}

static pthread_mutex_t lock;

static action_t actions[] = {
    { "fortune",  fortune, NULL },
    { NULL, NULL, NULL }
};

static station_t stations[] = {
    { false, actions, &lock, waiting_for_button, 500 },
    { NULL, NULL }
};

int
main(int argc, char **argv)
{
    if (wb_init() < 0) {
	fprintf(stderr, "Failed to initialize wb\n");
	exit(1);
    }

    pthread_mutex_init(&lock, NULL);

    wb_set_pull_up(HAND_PIN, WB_PULL_UP_DOWN);

    animation_main(stations);

    return 0;
}
