#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "animation-actions.h"
#include "animation-common.h"
#include "maestro.h"
#include "pi-usb.h"
#include "talking-skull.h"
#include "track.h"
#include "util.h"
#include "wb.h"

#define HAND_PIN  1
#define EYES	  2,5
#define SERVO	  0

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

static struct {
    const char            *fname;
    talking_skull_actor_t *actor;
    track_t	          *track;
} fortunes[] = {
    { "fortune-placeholder.wav" },
};
static const int n_fortunes = sizeof(fortunes) / sizeof(fortunes[0]);

static int chase_i;
static track_t *cogs;
static stop_t *cogs_stop;
static maestro_t *m;

static void flash_lights(int ms)
{
    fprintf(stderr, "  Flashing lights %d ms\n", ms);

    while (ms > 0) {
	unsigned sleep_ms = random_number_in_range(FLASHING_LIGHTS_FLASH_MIN_MS, FLASHING_LIGHTS_FLASH_MAX_MS);
	unsigned state = random_number_in_range(0, FLASHING_LIGHTS_MASK);

	wb_set_outputs(FLASHING_LIGHTS_MASK, state);
	ms_sleep(sleep_ms);
	ms -= sleep_ms;
    }
}

static void blink_lights(void)
{
    int blink;

    fprintf(stderr, "  Blinking lights\n");

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

    stop_reset(cogs_stop);
    track_play_asynchronously(cogs, cogs_stop);

    flash_lights(random_number_in_range(FLASHING_LIGHTS_MIN_MS, FLASHING_LIGHTS_MAX_MS));
    blink_lights();
    stop_stop(cogs_stop);
    ms_sleep(1000);

    talking_skull_actor_play(fortunes[0].actor);
    track_play(fortunes[0].track);

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

static void update_servo(void *unused, double pos)
{
    static int eyes = -1;
    int new_eyes = pos > 30;
static double last_pos = -1;
double fabs(double);

if (fabs(last_pos - pos) >= 1) {
fprintf(stderr, "    servo %.0f eyes %d\n", pos, new_eyes);
last_pos = pos;
}

    maestro_set_servo_pos(m, SERVO, pos);
    if (eyes != new_eyes) {
	wb_set(EYES, new_eyes);
	eyes = new_eyes;
    }
}

static void prepare_audio(void)
{
    int i;

    if ((cogs = track_new("cogs.wav")) == NULL) {
	exit(1);
    }
    cogs_stop = stop_new();
    for (i = 0; i < n_fortunes; i++) {
	fortunes[i].track = track_new(fortunes[i].fname);
	fortunes[i].actor = talking_skull_actor_new(fortunes[i].fname, update_servo, NULL);
    }
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

    pi_usb_init();

    if ((m = maestro_new()) == NULL) {
        fprintf(stderr, "Failed to initialize servo controller\n");
        exit(1);
    }

    maestro_set_servo_is_inverted(m, SERVO, 1);
    maestro_set_servo_range_pct(m, SERVO, 33, 53);

    pthread_mutex_init(&lock, NULL);

    wb_set_pull_up(HAND_PIN, WB_PULL_UP_DOWN);

    prepare_audio();

    animation_main(stations);

    return 0;
}
