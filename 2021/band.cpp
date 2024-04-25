#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "audio.h"
#include "audio-player.h"
#include "maestro.h"
#include "pi-threads.h"
#include "pi-usb.h"
#include "server.h"
#include "talking-skull.h"
#include "talking-skull-from-audio.h"
#include "time-utils.h"
#include "util.h"
#include "wav.h"
#include "wb.h"
#include "ween-hours.h"

#define EYE_BANK	2
#define VOCALS_EYES	8

#define LIGHTS		1,1

#define VOCALS_SERVO	0
#define DRUM_SERVO0	1
#define GUITAR_SERVO	3
#define KEYBOARD_SERVO0	4

#define BETWEEN_SONG_MS	(ween_hours_is_trick_or_treating() ? 5000 : 30000)

#define SONG_WAV	"pour-some-sugar.wav"
#define VOCALS_OPS	"pour-some-sugar-vocals.wav"
#define DRUM_OPS	"pour-some-sugar-drums.wav"
#define GUITAR_OPS	"pour-some-sugar-guitar.wav"
#define KEYBOARD_OPS	"pour-some-sugar-vocals.wav"

#define GUITAR_UP_STATE_MS	300
#define GUITAR_DOWN_STATE_MS	40
#define GUITAR_MULT		1

static maestro_t *m;

static Audio *audio = new AudioPi();
static AudioPlayer *player = new AudioPlayer(audio);

static Wav *song;
static bool force = true;

class BandServo : public Servo {
public:
    BandServo(int servo, int eyes_pin = -1, bool eyes_inverted = false, int eyes_on_pct = 50) : servo(servo), eyes_pin(eyes_pin), eyes_inverted(eyes_inverted) { }

    bool move_to(double pos) override {
        bool new_eyes;

        if (pos > 100) pos = 100;
        maestro_set_servo_pos(m, servo, pos);

        new_eyes = pos >= eyes_on_pct;
        if (eyes_inverted) new_eyes = !new_eyes;

        if (eyes != new_eyes) {
	    if (eyes_pin >= 0) wb_set(EYE_BANK, eyes_pin, new_eyes);
	    eyes = new_eyes;
        }
	return true;
    }

private:
    int servo;
    int eyes_pin;
    bool eyes_inverted;
    int eyes_on_pct;
    bool eyes = false;
};

class VocalsTalkingSkull : public TalkingSkull, public BandServo {
public:
    VocalsTalkingSkull(TalkingSkullOps *ops) : BandServo(VOCALS_SERVO, VOCALS_EYES), TalkingSkull(ops, "vocals") {}
    void update_pos(double pos) override { move_to(pos); }
};

class DrumTalkingSkull : public TalkingSkull, public Servo {
public:
     DrumTalkingSkull(TalkingSkullOps *ops) : TalkingSkull(ops) {
	servo0 = new BandServo(DRUM_SERVO0);
	servo1 = new BandServo(DRUM_SERVO0+1);
     }

     void update_pos(double pos) override {
	move_to(pos);
     }

     bool move_to(double pos) override {
	return servo0->move_to(pos) && servo1->move_to(pos);
     }

private:
     BandServo *servo0;
     BandServo *servo1;
};

class GuitarTalkingSkull : public TalkingSkull, public BandServo {
public:
    GuitarTalkingSkull(TalkingSkullOps *ops) : TalkingSkull(ops, "guitar"), BandServo(GUITAR_SERVO) { }

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

class KeyboardTalkingSkull : public TalkingSkull, public Servo {
public:
    KeyboardTalkingSkull(TalkingSkullOps *ops) : TalkingSkull(ops, "keyboard") {
	servo0 = new BandServo(KEYBOARD_SERVO0);
        servo1 = new BandServo(KEYBOARD_SERVO0+1);
    }

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

    bool move_to(double pos) override {
	return servo0->move_to(pos) && servo1->move_to(pos);
    }

private:
    BandServo *servo0, *servo1;
    int hold = 0;
    bool is_down = false;
    int n_in_down = 0;
};

static VocalsTalkingSkull *vocals;
static DrumTalkingSkull *drum;
static GuitarTalkingSkull *guitar;
static KeyboardTalkingSkull *keyboard;

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
    vocals = new VocalsTalkingSkull(new TalkingSkullAudioOps(VOCALS_OPS));
    drum = new DrumTalkingSkull(new TalkingSkullAudioOps(DRUM_OPS));
    guitar = new GuitarTalkingSkull(new TalkingSkullAudioOps(GUITAR_OPS));
    keyboard = new KeyboardTalkingSkull(new TalkingSkullAudioOps(KEYBOARD_OPS));

    maestro_set_servo_physical_range(m, VOCALS_SERVO, 976, 1296);
    maestro_set_servo_physical_range(m, VOCALS_SERVO, 976, 1400);
    maestro_set_servo_is_inverted(m, VOCALS_SERVO, 1);
    maestro_set_servo_physical_range(m, DRUM_SERVO0, 1696, 2000);
    maestro_set_servo_physical_range(m, DRUM_SERVO0+1, 1696, 2000); // TBD
    maestro_set_servo_physical_range(m, GUITAR_SERVO, 1200, 1800);
    maestro_set_servo_physical_range(m, KEYBOARD_SERVO0, 1400, 1700);
    maestro_set_servo_physical_range(m, KEYBOARD_SERVO0+1, 1400, 1700);

    rest_servos();
}

static char *
remote_event(void *unused, const char *cmd, struct sockaddr_in *addr, size_t size)
{
    if (strcmp(cmd, "play") == 0) {
	force = true;
	return strdup("ok");
    }
    return NULL;
}

static void
start_server()
{
    static server_args_t server_args;

    server_args.port = 5555;
    server_args.command = remote_event;
    server_args.state = NULL;

    fprintf(stderr, "starting server on port %d\n", server_args.port);

    pi_thread_create("server", server_thread_main, &server_args);
}

int
main(int argc, char **argv)
{
    pi_usb_init();
    wb_init();

    if ((m = maestro_new()) == NULL) {
	fprintf(stderr, "Failed to initialize servo controller\n");
	exit(1);
    }

    song = new Wav(new BufferFile(SONG_WAV));

    init_servos();

    wb_set(LIGHTS, 0);
    rest_servos();

    start_server();

    while (1) {
        while (! ween_hours_is_valid() && ! force) {
	    ms_sleep(1000);
        }

        wb_set(LIGHTS, 1);

        vocals->play();
        drum->play();
        guitar->play();
        //keyboard->play();

        player->play(song->to_audio_buffer());
	player->wait_done();

        wb_set(LIGHTS, 0);

        ms_sleep(BETWEEN_SONG_MS);
    }

    return 0;
}
