#include "pi.h"
#include "audio.h"
#include "audio-player.h"
#include "gp-input.h"
#include "gp-output.h"
#include "string-utils.h"
#include "neopixel-pico.h"
#include "net-listener.h"
#include "net-reader.h"
#include "net-writer.h"
#include "pi-threads.h"
#include "random-audio.h"
#include "stdin-reader.h"
#include "stdout-writer.h"
#include "threads-console.h"
#include "time-utils.h"
#include "wifi.h"

static const int neo_gpio = 4;
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
	neo->set_led((led % n_leds) + first_led, r, g, b);
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
    LightVortex(NeoPixelPico *neo, int first_led, int n_leds, int rotation_ms = VORTEX_ROTATION_MS, int n_active = 1) : LightAction(neo, first_led, n_leds), n_active(n_active) {
    }

    void step() override {
	int n_leds = get_n_leds();

	set_all(0);
	set_led(0, ON_VALUE);
	for (int i = it; i < it + n_active; i++) {
	    set_led(i, ON_VALUE);
	}
	schedule_in((rotation_ms + n_leds/2) / n_leds);	// To do accelerate
	if (++it >= n_leds) it = 1;
    }

private:
    int it = 1;
    int n_active;
    int rotation_ms;
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

static void light_step(NeoPixelPico *neo, LightAction *bubble, LightAction *main) {
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
}

class FartConsole : public ThreadsConsole, public PiThread {
public:
    FartConsole(const char *name, Reader *r, Writer *w) : ThreadsConsole(r, w), PiThread(name) {
	start();
    }

    void main() {
	Console::main();
    }

    void process_cmd(const char *cmd) override {
	if (is_command(cmd, "fart")) {
	} else {
	    ThreadsConsole::process_cmd(cmd);
	}
    }

    void usage() override {
	ThreadsConsole::usage();
    }
};

class FartListener : public NetListener {
public:
    FartListener() : NetListener() {
	start();
    }

    void accepted(int fd) override {
	new FartConsole("net-console", new NetReader(fd), new NetWriter(fd));
    }
};

static void threads_main(int argc, char **argv) {
    new FartConsole("console", new StdinReader(), new StdoutWriter());

    wifi_init("fart");
    new FartListener();

    Audio *audio = new AudioPico();
    AudioPlayer *player = new AudioPlayer(audio);
    RandomAudio *random_audio = new RandomAudio();
    
    bool exists = true;
    for (int i = 0; exists; i++) {
	char *fname = maprintf("fart-%d.wav", i);
	exists = random_audio->add(fname);
	printf("%s %s\n", fname, exists ? "exists" : "doesn't exist, stopping");
	free(fname);
    }

    GPInput *go = new GPInput(9);
    go->set_pullup_up();
    GPOutput *ready = new GPOutput(10);

    NeoPixelPico *neo = new NeoPixelPico(neo_gpio);
    neo->set_n_leds(total_n_leds);

    LightAction *bubble_vortex = new LightVortex(neo, n_main_leds, n_bubble_leds, 125);
    LightAction *bubble_vortex_fast = new LightVortex(neo, n_main_leds, n_bubble_leds, 80);
    LightAction *bubble_pulse = new PulsingLight(neo, n_main_leds, n_bubble_leds);
    LightAction *main_vortex = new LightVortex(neo, 0, n_main_leds, 91, 5);
    LightAction *main_vortex_fast = new LightVortex(neo, 0, n_main_leds, 80, 5);
    LightAction *main_pulse = new PulsingLight(neo, 0, n_main_leds, 4);

    while (1) {
	ready->on();
	while (! go->get()) {
	    light_step(neo, bubble_pulse, main_pulse);
	    ms_sleep(1);
	}

	ready->off();

	for (int i = 0; i < 2000; i++) {
	    light_step(neo, bubble_vortex, main_vortex);
	    ms_sleep(1);
	}

	random_audio->play_random(player);

	while (player->is_active()) {
	    light_step(neo, bubble_vortex_fast, main_vortex_fast);
	    ms_sleep(1);
	}
    }
}

int
main(int argc, char **argv)
{
    pi_init_with_threads(threads_main, argc, argv);
}
