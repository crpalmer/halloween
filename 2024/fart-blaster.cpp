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

static const int neo_gpio = 28;
static const int n_main_leds = 35;
static const int n_bubble_leds = 7;
static const int total_n_leds = n_main_leds + n_bubble_leds;

#define FADE_INC	2
#define FADE_SLEEP_MS	7
#define VORTEX_ROTATION_MS 100
#define ON_VALUE	254
#define OFF_VALUE	50

static class Fart *fart;

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

    struct timespec *soonest_next(struct timespec *current_next = NULL) {
	if (! current_next || nano_later_than(current_next, &next)) {
	    return &next;
	}
	return current_next;
    }

protected:
    void set_all(int r, int g, int b) {
	for (int i = 0; i < n_leds; i++) set_led(i, r, g, b);
    }

    void set_all(int colour) { set_all(colour, colour, colour); }

    void set_led(int led, int r, int g, int b) {
	if (led >= 0 && led < n_leds) neo->set_led(led + first_led, r, g, b);
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
    LightVortex(NeoPixelPico *neo, int first_led, int n_leds, int rotation_ms = VORTEX_ROTATION_MS, int n_active = 1) : LightAction(neo, first_led, n_leds), rotation_ms(rotation_ms), n_active(n_active) {
    }

    void step() override {
	int n_leds = get_n_leds();

	set_all(0);
	set_led(0, ON_VALUE);
	for (int i = it; i < it + n_active; i++) {
	    set_led((i % n_leds), ON_VALUE);
	}
	schedule_in((rotation_ms + n_leds/2) / n_leds);
	if (++it >= n_leds) it = 1;
    }

private:
    int it = 1;
    int rotation_ms;
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

static int light_step(NeoPixelPico *neo, LightAction *bubble, LightAction *main) {
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

    struct timespec *next = bubble->soonest_next(main->soonest_next());
    int ms = - nano_elapsed_ms_now(next);
    if (ms < 1) ms = 1;
    return ms;
}

class Fart : public PiThread {
public:
    Fart() : PiThread("fart") {
	audio = Audio::create_instance();
	player = new AudioPlayer(audio);
	random_audio = new RandomAudio();

	bool exists = true;
	for (int i = 0; exists; i++) {
	    char *fname = maprintf("fart-%d.wav", i);
	    exists = random_audio->add(fname);
	    consoles_printf("%s %s\n", fname, exists ? "exists" : "doesn't exist, stopping");
	    free(fname);
	}

	ready = new GPOutput(10);

	neo = new NeoPixelPico(neo_gpio);
	neo->set_n_leds(total_n_leds);
	neo->set_all(0, 0, 0);
	neo->show();

	bubble_vortex = new LightVortex(neo, n_main_leds, n_bubble_leds, 125);
	bubble_vortex_fast = new LightVortex(neo, n_main_leds, n_bubble_leds, 80);
	bubble_pulse = new PulsingLight(neo, n_main_leds, n_bubble_leds);

	main_vortex = new LightVortex(neo, 0, n_main_leds, 91, 5);
	main_vortex_fast = new LightVortex(neo, 0, n_main_leds, 80, 5);
	main_pulse = new PulsingLight(neo, 0, n_main_leds, 4);

	lock = new PiMutex();

	start();
    }

    void main() override {
	ready->on();
	while (1) {
	    lock->lock();
	    bool fart_now = is_fart;
	    lock->unlock();

	    if (! fart_now) {
	        int ms = light_step(neo, bubble_pulse, main_pulse);
	        lock->unlock();
	        ms_sleep(ms);
	    } else {
		lock->unlock();

		consoles_printf("Fart loading\n");
		ready->off();

		for (int i = 0; i < 2000; ) {
		    int ms = light_step(neo, bubble_vortex, main_vortex);
		    ms_sleep(ms);
		    i += ms;
		}

		consoles_printf("Farting\n");
		random_audio->play_random(player);

		while (player->is_active()) {
		    int ms = light_step(neo, bubble_vortex_fast, main_vortex_fast);
		    ms_sleep(ms);
		}

		consoles_printf("Fart complete\n");
		ready->on();

		lock->lock();
		is_fart = false;
		lock->unlock();
	    }
	}
    }

    void dump_audio() {
	random_audio->dump();
    }

    void fart() {
	lock->lock();
	is_fart = true;
	lock->unlock();
    }

private:
    Audio *audio;
    AudioPlayer *player;
    RandomAudio *random_audio;

    NeoPixelPico *neo;
    GPOutput *ready;

    LightAction *bubble_vortex;
    LightAction *bubble_vortex_fast;
    LightAction *bubble_pulse;
    LightAction *main_vortex;
    LightAction *main_vortex_fast;
    LightAction *main_pulse;

    PiMutex *lock;
    bool is_fart = false;
};

class FartConsole : public ThreadsConsole, public PiThread {
public:
    FartConsole(const char *name, Reader *r, Writer *w) : ThreadsConsole(r, w), PiThread(name) {
	start();
    }

    void main() {
	Console::main();
    }

    void process_cmd(const char *cmd) override {
	if (is_command(cmd, "audio")) {
	    fart->dump_audio();
	} else if (is_command(cmd, "fart")) {
	    fart->fart();
	} else {
	    ThreadsConsole::process_cmd(cmd);
	}
    }

    void usage() override {
	ThreadsConsole::usage();
	consoles_printf("audio - dump the list of audio tracks\n");
	consoles_printf("fart - trigger a fart\n");
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

    fart = new Fart();

    GPInput *go = new GPInput(9);
    go->set_pullup_up();

    while (1) {
	while (! go->get()) ms_sleep(1);
	fart->fart();
    }
}

int
main(int argc, char **argv)
{
    pi_init_with_threads(threads_main, argc, argv);
}
