#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pico/bootrom.h>
#include <tusb.h>
#include "neopixel-pico.h"
#include "pi.h"
#include "time-utils.h"
#include "util.h"

#define N_NEO		4

#define ALL_P		0.05
#define FADE_INC	1
#define FADE_SLEEP_MS	10

static NeoPixelPico *neo[N_NEO];
static int n_leds[N_NEO] = { 14, 13, 8, 47 };
static bool lit[N_NEO] = { 0, };

const char *names[] = { "left-eye", "right-eye", "nose", "mouth" };

static void
fade_in(int *set, int n_set, int final_r, int final_g, int final_b)
{
    int r = 0, g = 0, b = 0;

    while (r < final_r || g < final_g || b < final_b) {
	if (r < final_r) r += FADE_INC;
	if (g < final_g) g += FADE_INC;
	if (b < final_b) b += FADE_INC;
	for (int i = 0; i < n_set; i++) {
	    neo[set[i]]->set_all(r, g, b);
	    neo[set[i]]->show();
	}
	ms_sleep(FADE_SLEEP_MS);
    }
}

static void
fade_out(int *set, int n_set, int r, int g, int b)
{
    while (r > 0 || g > 0 || b > 0) {
	if (r > 0) r -= FADE_INC;
	if (g > 0) g -= FADE_INC;
	if (b > 0) b -= FADE_INC;
	for (int i = 0; i < n_set; i++) {
	    neo[set[i]]->set_all(r, g, b);
	    neo[set[i]]->show();
	}
	ms_sleep(FADE_SLEEP_MS);
    }
}

static int
select_all(int *set)
{
    int n = 0;

    for (int i = 0; i < N_NEO; i++) {
	if (! lit[i]) {
	    set[n] = i;
	    lit[n] = true;
	    n++;
	}
    }
    return n;
}

static int
select_set(int *set, bool desired_state, int max_n)
{
    int n = 0;

    for (int i = 0; i < N_NEO; i++) {
	if (lit[i] == desired_state) {
	    set[n++] = i;
	}
    }

    if (n) {
	int n_needed = random_number_in_range(1, max_n-1);
	while (n > n_needed) {
	    int reject = random_number_in_range(0, n-1);
	    set[reject] = set[n-1];
	    n--;
	}
    }

    for (int i = 0; i < n; i++) {
	lit[set[i]] = ! lit[set[i]];
    }

    return n;
}

static void
dump_set(const char *msg, int *set, int n_set)
{
    printf("%s:", msg);
    for (int i = 0; i < n_set; i++) {
	printf(" %s", names[set[i]]);
    }
    printf("\n");
}

static void
dump_state()
{
    printf("lit:");
    for (int i = 0; i < N_NEO; i++) {
	if (lit[i]) printf(" %s", names[i]);
    }
    printf("\n");
}

int main(int argc, char **argv)
{
    double all_P = ALL_P;

    seed_random();

    pi_init_no_reboot();

    for (int i = 0; i < N_NEO; i++) {
        neo[i] = new NeoPixelPico(i, true);
        neo[i]->set_n_leds(n_leds[i]);
	neo[i]->set_all(255, 0, 0);
	neo[i]->show();
	lit[i] = true;
    }

    printf("Sleeping before starting\n");
    ms_sleep(2000);
    printf("Starting\n");

    for (;;) {
	int set[N_NEO];
	int n_set;

	if (random_number() < all_P) {
	    n_set = select_all(set);
	    dump_set("all", set, n_set);
	    all_P = ALL_P;
	} else {
	    all_P += ALL_P;
	    n_set = select_set(set, false, N_NEO);
	    dump_set(" in", set, n_set);
        }
	if (n_set) {
	    fade_in(set, n_set, 255, 0, 0);
	    dump_state();

	    ms_sleep(1000);
	}

	n_set = select_set(set, true, N_NEO);
	if (n_set) {
	    dump_set("out", set, n_set);
	    fade_out(set, n_set, 255, 0, 0);

	    dump_state();

	    ms_sleep(1000);
	}

	printf("\n");
    }
}
