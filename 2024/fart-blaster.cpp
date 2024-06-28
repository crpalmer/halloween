#include "pi.h"
#include "neopixel-pico.h"
#include "pi-threads.h"
#include "time-utils.h"

static const int neo_gpio = 28;
static const int n_main_leds = 35;
static const int n_bubble_leds = 7;
static const int total_n_leds = n_main_leds + n_bubble_leds;

#define FADE_INC	2
#define FADE_SLEEP_MS	7
#define VORTEX_ROTATION_MS 100
#define ON_VALUE	254
#define OFF_VALUE	50

class LightAction {
public:
    LightAction(NeoPixelPico *neo, int first_led, int n_leds) : neo(neo), first_led(first_led), n_leds(n_leds) {
	nano_gettime(&next);
    }

    virtual void step() = 0;

    bool is_ready() {
	return nano_now_is_later_than(&next);
    }

    void schedule_in(int ms) {
	nano_gettime(&next);
	nano_add_ms(&next, ms);
    }

protected:
    void set_all(int r, int g, int b) {
	for (int i = 0; i < n_leds; i++) set_led(i, r, g, b);
    }

    void set_all(int colour) { set_all(colour, colour, colour); }

    void set_led(int led, int r, int g, int b) {
	neo->set_led(led + first_led, r, g, b);
    }

    void set_led(int led, int colour) { set_led(led, colour, colour, colour); }

    int get_n_leds() { return n_leds; }

private:
    NeoPixelPico *neo;
    int first_led;
    int n_leds;

    struct timespec next;
};

class LightVortex : public LightAction {
public:
    LightVortex(NeoPixelPico *neo, int first_led, int n_leds, int n_active = 1) : LightAction(neo, first_led, n_leds), n_active(n_active) {
    }

    void step() override {
	int n_leds = get_n_leds();

	set_all(0);
	set_led(0, ON_VALUE);
	for (int i = it; i < it + n_active; i++) {
	    set_led(i % n_leds, ON_VALUE);
	}
	schedule_in(VORTEX_ROTATION_MS / n_leds);	// To do accelerate
	if (++it >= n_leds) it = 1;
    }

private:
    int it = 1;
    int n_active;
};

class PulsingLight : public LightAction {
public:
    PulsingLight(NeoPixelPico *neo, int first_led, int n_leds, int n_groups = 1) : LightAction(neo, first_led, n_leds), n_groups(n_groups) {
    }

    void step() override {
	set_all(0);
	for (int i = cur_group; i < get_n_leds(); i += n_groups) {
	    set_led(i, colour);
	}
	//cur_group = (cur_group + 1) % n_groups;

	if (is_entering) {
	    colour += FADE_INC;
	    if (colour > ON_VALUE) {
		colour = ON_VALUE;
		is_entering = false;
	    }
	} else {
	    colour -= FADE_INC;
	    if (colour < OFF_VALUE) {
		colour = OFF_VALUE;
		is_entering = true;
	    }
	}
	schedule_in(FADE_SLEEP_MS);
    }

private:
    bool is_entering = true;
    int colour = OFF_VALUE;
    int n_groups;
    int cur_group = 0;
};

#define A 0
static void threads_main(int argc, char **argv) {
    NeoPixelPico *neo = new NeoPixelPico(neo_gpio);
    neo->set_n_leds(total_n_leds);

    LightAction *bubble_vortex = new LightVortex(neo, n_main_leds, n_bubble_leds);
    LightAction *bubble_pulse = new PulsingLight(neo, n_main_leds, n_bubble_leds);
    LightAction *main_vortex = new LightVortex(neo, 0, n_main_leds, 5);
    LightAction *main_pulse = new PulsingLight(neo, 0, n_main_leds, 4);

    int steps = 0;
    LightAction *bubble = bubble_pulse;
    LightAction *main = main_vortex;

    while (1) {
	bool dirty = false;

	if (bubble->is_ready()) {
	    bubble->step();
	    dirty = true;
	}

	if (main->is_ready()) {
	    main->step();
	    dirty = true;
	}

	if (dirty) neo->show();

	ms_sleep(1);
	steps++;

	if (steps % 5000 == 0) {
	    if (bubble == bubble_pulse) bubble = bubble_vortex;
	    else bubble = bubble_pulse;
	}

	if (steps % 3456 == 0) {
	    if (main == main_pulse) main = main_vortex;
	    else main = main_pulse;
	}
    }
}

int
main(int argc, char **argv)
{
    pi_init_with_threads(threads_main, argc, argv);
}
