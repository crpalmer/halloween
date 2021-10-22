#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "maestro.h"
#include "pi-usb.h"
#include "server.h"
#include "talking-skull.h"
#include "time-utils.h"
#include "track.h"
#include "util.h"
#include "wb.h"
#include "ween-hours.h"

#define EYE_BANK	1
#define VOCALS_EYES	5

#define LIGHTS		1,1

#define VOCALS_SERVO	0
#define DRUM_SERVO0	1
#define GUITAR_SERVO	3
#define KEYBOARD_SERVO0	4

#define BETWEEN_SONG_MS	(ween_hours_is_trick_or_treating() ? 5000 : 30000)

#define SONG_WAV	"pour-some-sugar.wav"
#define VOCALS_OPS	"vocals.ops"
#define DRUM_OPS	"drums.ops"
#define GUITAR_OPS	"guitar.ops"
#define KEYBOARD_OPS	"piano.ops"

static maestro_t *m;

static track_t *song;
static talking_skull_actor_t *vocals, *drum, *guitar, *keyboard;
static bool force = false;

typedef struct {
    int		servo;
    int		eyes;
    int		eyes_high;
    int		eyes_on_pct;
    int		eye_pin;
} servo_state_t;

typedef struct {
    int		is_down;
    int		n_in_down;
    int         hold;
} keyboard_state_t;

static servo_state_t vocals_state = { VOCALS_SERVO, 0, 1, 30, VOCALS_EYES };
static servo_state_t drum_state[2] = { { DRUM_SERVO0, 0, 0, 50, -1 }, { DRUM_SERVO0+1, 0, 0, 50, -1 } };
static servo_state_t guitar_state = { GUITAR_SERVO, 0, 0, 50, -1 };
static keyboard_state_t keyboard_state = { 0, };

static void
servo_update(void *state_as_vp, double new_pos)
{
    servo_state_t *s = (servo_state_t *) state_as_vp;
    int new_eyes;
    double pos;

    pos = new_pos;
    if (pos > 100) pos = 100;
    
    maestro_set_servo_pos(m, s->servo, pos);

    new_eyes = pos >= s->eyes_on_pct;
    if (! s->eyes_high) new_eyes = !new_eyes;

    if (s->eyes != new_eyes) {
	if (s->eye_pin >= 0) wb_set(EYE_BANK, s->eye_pin, new_eyes);
	s->eyes = new_eyes;
    }
}

static void
drum_update(void *state_as_vp, double new_pos)
{
    servo_state_t *s = (servo_state_t *) state_as_vp;
    servo_update(&s[0], new_pos);
    servo_update(&s[1], new_pos);
}

static void
keyboard_update(void *state_as_vp, double new_pos)
{
    keyboard_state_t *s = (keyboard_state_t *) state_as_vp;
    int new_down = new_pos > 5;
    
    if (s->hold > 1) {
	s->hold--;
	return;
    }

    if (s->is_down && s->n_in_down > 20) {
	s->hold = 20;
	new_down = 0;
    }

    if (new_down != s->is_down) {
	s->is_down = new_down;
	s->n_in_down = 0;
	maestro_set_servo_pos(m, KEYBOARD_SERVO0, s->is_down ? 100 : 0);
	maestro_set_servo_pos(m, KEYBOARD_SERVO0+1, s->is_down ? 0 : 100);
    }

    if (s->is_down) s->n_in_down++;
}

static void
rest_servos(void)
{
    servo_update(&vocals_state, 0);
    servo_update(&drum_state[0], 100);
    servo_update(&drum_state[1], 100);
    servo_update(&guitar_state, 25);
    maestro_set_servo_pos(m, KEYBOARD_SERVO0, 100);
    maestro_set_servo_pos(m, KEYBOARD_SERVO0+1, 100);
}

static void
init_servos(void)
{
    vocals = talking_skull_actor_new_ops(VOCALS_OPS, servo_update, &vocals_state);
    drum = talking_skull_actor_new_ops(DRUM_OPS, drum_update, &drum_state);
    guitar = talking_skull_actor_new_ops(GUITAR_OPS, servo_update, &guitar_state);
    keyboard = talking_skull_actor_new_ops(KEYBOARD_OPS, keyboard_update, &keyboard_state);

    if (! vocals || ! drum || ! guitar || ! keyboard) {
	exit(1);
    }

    maestro_set_servo_physical_range(m, VOCALS_SERVO, 976, 1296);
    maestro_set_servo_is_inverted(m, VOCALS_SERVO, 1);
    maestro_set_servo_physical_range(m, DRUM_SERVO0, 1696, 2000);
    maestro_set_servo_physical_range(m, DRUM_SERVO0+1, 1696, 2000); // TBD
    maestro_set_servo_physical_range(m, GUITAR_SERVO, 1700, 2000);
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
    pthread_t thread;

    server_args.port = 5555;
    server_args.command = remote_event;
    server_args.state = NULL;

    fprintf(stderr, "starting server on port %d\n", server_args.port);

    pthread_create(&thread, NULL, server_thread_main, &server_args);
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

    song = track_new(SONG_WAV);
    if (! song) {
	perror(SONG_WAV);
	exit(1);
    }

    init_servos();

    wb_set(LIGHTS, 0);
    rest_servos();

    start_server();

    while (! ween_hours_is_valid() && ! force) {
	ms_sleep(1000);
    }

    wb_set(LIGHTS, 1);

    talking_skull_actor_play(vocals);
    //talking_skull_actor_play(drum);
    talking_skull_actor_play(guitar);
    talking_skull_actor_play(keyboard);
    track_play(song);

    wb_set(LIGHTS, 0);

    ms_sleep(BETWEEN_SONG_MS);

    return 0;
}
