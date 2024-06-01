#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pi.h"
#include "audio.h"
#include "audio-player.h"
#include "net-listener.h"
#include "net-reader.h"
#include "net-writer.h"
#include "pi-threads.h"
#include "servos.h"
#include "sntp.h"
#include "stdin-reader.h"
#include "stdout-writer.h"
#include "talking-skull.h"
#include "talking-skull-from-audio.h"
#include "threads-console.h"
#include "time-utils.h"
#include "wav.h"
#include "ween-hours.h"

#ifdef PLATFORM_pi
#include "maestro.h"
#include "pi-usb.h"
#include "wb.h"
#else
#include "gp-input.h"
#include "gp-output.h"
#include "servo-gpio.h"
#include "wifi.h"
#endif

#define EYES_BANK		2
#define VOCALS_EYES_PI_PIN	8
#define VOCALS_EYES_PICO_PIN	7

#define LIGHTS_PI_PIN		1,1
#define LIGHTS_PICO_PIN		8

#define VOCALS_SERVO	0
#define DRUM_SERVO0	1
#define GUITAR_SERVO	3
#define KEYBOARD_SERVO0	4

#define BETWEEN_SONG_MS	(ween_hours_is_trick_or_treating() ? 5000 : 30000)

#if 1
#define SONG_WAV	"pour-some-sugar.wav"
#define VOCALS_WAV	"pour-some-sugar-vocals.wav"
#define DRUM_WAV	"pour-some-sugar-drums.wav"
#define GUITAR_WAV	"pour-some-sugar-guitar.wav"
#define KEYBOARD_WAV	"pour-some-sugar-vocals.wav"
#else
#define SONG_WAV	"pour-some-sugar-30.wav"
#define VOCALS_WAV	"pour-some-sugar-vocals-30.wav"
#define DRUM_WAV	"pour-some-sugar-drums-30.wav"
#define GUITAR_WAV	"pour-some-sugar-guitar-30.wav"
#define KEYBOARD_WAV	"pour-some-sugar-vocals-30.wav"
#endif

#ifdef PLATFORM_pi
#define TALKING_SKULL_BYTES_PER_OP 2
#else
#define TALKING_SKULL_BYTES_PER_OP 1
#endif

#define GUITAR_UP_STATE_MS	300
#define GUITAR_DOWN_STATE_MS	40
#define GUITAR_MULT		1

static ServoFactory *servo_factory;

static Audio *audio;
static AudioPlayer *player;

static Output *vocals_eyes;
static Output *lights;

static bool force = true;

class BandServo : public Servo {
public:
    BandServo(unsigned servo_id, Output *eyes_output = NULL, bool eyes_inverted = false, int eyes_on_pct = 50) : servo(servo_factory->get_servo(servo_id)), eyes_output(eyes_output), eyes_inverted(eyes_inverted) {
    }

    bool move_to(double pos) override {
        bool new_eyes;

        if (pos > 100) pos = 100;
        servo->move_to(pos);

        new_eyes = pos >= eyes_on_pct;
        if (eyes_inverted) new_eyes = !new_eyes;

        if (eyes != new_eyes) {
	    if (eyes_output) eyes_output->set(new_eyes);
	    eyes = new_eyes;
        }
	return true;
    }

private:
    Servo *servo;
    Output *eyes_output;
    bool eyes_inverted;
    int eyes_on_pct;
    bool eyes = false;
};

class BandTalkingSkull : public TalkingSkull, public Servos {
public:
    BandTalkingSkull(const char *wav_fname, const char *name = "band-ts") : TalkingSkull(name, TALKING_SKULL_BYTES_PER_OP) {
	TalkingSkullOps *ops = TalkingSkullAudioOps::open_wav(wav_fname);
        set_ops(ops);
	delete ops;
    }

    void update_pos(double pos) override { move_to(pos); }
};

class GuitarTalkingSkull : public BandTalkingSkull {
public:
    GuitarTalkingSkull(const char *wav_fname) : BandTalkingSkull(wav_fname, "guitar") { }

    void update_pos(double new_pos) override {
        bool new_up = new_pos > 10;
        struct timespec now;
        int ms;

        nano_gettime(&now);
        ms = nano_elapsed_ms(&now, &at);

        if (new_up != wants_up) {
	    wants_up = new_up;
	    at = now;
	    is_up = new_up;
        } else if (wants_up && ms > (is_up ? GUITAR_UP_STATE_MS : GUITAR_DOWN_STATE_MS)) {
	    is_up = ! is_up;
	    at = now;
        }
        move_to(wants_up && ! is_up ? 0 : new_pos * GUITAR_MULT);
    }

private:
    bool wants_up = false;
    bool is_up = false;
    struct timespec at = { 0, };
};

class KeyboardTalkingSkull : public BandTalkingSkull {
public:
    KeyboardTalkingSkull(const char *wav_fname) : BandTalkingSkull(wav_fname, "keyboard") { }

    void update_pos(double new_pos) override {
	bool new_down = new_pos > 5;
    
	if (hold > 1) {
	    hold--;
	    return;
	}

	if (is_down && n_in_down > 20) {
	    hold = 20;
	    new_down = 0;
	}

	if (new_down != is_down) {
	    is_down = new_down;
	    n_in_down = 0;
	    move_to(is_down ? 100 : 0);
	}

	if (is_down) n_in_down++;
    }

private:
    int hold = 0;
    bool is_down = false;
    int n_in_down = 0;
};

static BandTalkingSkull *vocals;
static BandTalkingSkull *drum;
static BandTalkingSkull *guitar;
static BandTalkingSkull *keyboard;

static void
rest_servos(void)
{
    vocals->move_to(0);
    drum->move_to(100);
    guitar->move_to(0);
    keyboard->move_to(100);
}

static void
init_servos(void)
{
    printf("Generating vocals.\n");
    vocals = new BandTalkingSkull(VOCALS_WAV, "vocals");
    vocals->add(new BandServo(VOCALS_SERVO, vocals_eyes));
    vocals->set_range(976, 1296);
    vocals->set_is_inverted(true);

    printf("Generating drum.\n");
    drum = new BandTalkingSkull(DRUM_WAV, "drums");
    drum->add(new BandServo(DRUM_SERVO0));
    drum->add(new BandServo(DRUM_SERVO0+1));
    drum->set_range(1696, 2000);

    printf("Generating guitar.\n");
    guitar = new GuitarTalkingSkull(GUITAR_WAV);
    guitar->add(new BandServo(GUITAR_SERVO));
    guitar->set_range(1200, 1800);

    printf("Generating keyboard.\n");
    keyboard = new KeyboardTalkingSkull(KEYBOARD_WAV);
    keyboard->add(new BandServo(KEYBOARD_SERVO0));
    keyboard->add(new BandServo(KEYBOARD_SERVO0+1));
    keyboard->set_range(1400, 1700);

    rest_servos();
}

class BandConsole : public ThreadsConsole, public PiThread {
public:
   BandConsole(int fd) : BandConsole(new NetReader(fd), new NetWriter(fd)) {}
   //BandConsole(int fd) : ThreadsConsole(new NetReader(fd), new NetWriter(fd)), PiThread("console") { start(0); }
   BandConsole(Reader *r, Writer *w) : ThreadsConsole(r, w), PiThread("console") { start(0); }

    void main(void) override {
	ThreadsConsole::main();
    }

    void process_cmd(const char *cmd) override {
	if (strcmp(cmd, "play") == 0) {
	    force = true;
	    printf("play: force mode enabled.\n");
        } else {
	    ThreadsConsole::process_cmd(cmd);
	}
    }

    void usage() override {
	ThreadsConsole::usage();
	printf("play - disable any ween hours processing.\n");
    }
};

class BandListener : public NetListener {
public:
   BandListener(uint16_t port = 5555) : NetListener(port) { start(); }

   void main() override {
	net_sntp_set_pico_rtc(NULL);
	NetListener::main();
   }

   void accepted(int fd) {
	new BandConsole(fd);
   }
};

static void platform_setup() {
#ifdef PLATFORM_pi
    WeenBoard *wb = new WeenBoard();
    pi_usb_init();
    servo_factory = new Maestro();
    audio = new AudioPi();
    vocals_eyes = wb->get_output(EYES_BANK, VOCALS_EYES_PI_PIN);
    lights = wb->get_output(LIGHTS_PI_PIN);
#else
    wifi_init("band");

    int pins[] = { 22, 2, 3, 4, 5, 6 };
    servo_factory = new GpioServoFactory(pins, sizeof(pins) / sizeof(pins[0]));
    audio = new AudioPico();
    vocals_eyes = new GPOutput(VOCALS_EYES_PICO_PIN);
    lights = new GPOutput(LIGHTS_PICO_PIN);
#endif
    player = new AudioPlayer(audio);
}

static void threads_main(int argc, char **argv)
{
    platform_setup();
    new BandListener();
    new BandConsole(new StdinReader(), new StdoutWriter());

    printf("Loading %s\n", SONG_WAV);
    AudioBuffer *song = wav_open(SONG_WAV);
    if (! song) {
	consoles_printf("COULDN'T OPEN %s\n", SONG_WAV);
	while (1) ms_sleep(1000);
    }

    init_servos();

    lights->set(0);
    rest_servos();

    while (1) {
        while (! ween_hours_is_valid() && ! force) {
	    ms_sleep(1000);
        }

        lights->set(1);

        vocals->play();
        drum->play();
        guitar->play();
        keyboard->play();

	consoles_printf("Playing %s\n", SONG_WAV);
	if (! player->play_sync(song)) {
	    consoles_fatal_printf("fatal: Audio player didn't complete playing the song.\n");
	}
	consoles_printf("Done playing\n");

        lights->set(0);

        ms_sleep(BETWEEN_SONG_MS);
    }
}

int main(int argc, char **argv) {
    pi_init_with_threads(threads_main, argc, argv);
    return 0;
}
