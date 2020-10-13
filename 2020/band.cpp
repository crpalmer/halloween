#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "maestro.h"
#include "pi-usb.h"
#include "talking-skull.h"
#include "time-utils.h"
#include "track.h"
#include "util.h"
#include "wb.h"
//#include "ween2019.h"

#define EYE_BANK	1
#define VOCALS_EYES	5

#define LIGHTS		1,1

#define VOCALS_SERVO	0
#define DRUM_SERVO0	1
#define GUITAR_SERVO	3
#define KEYBOARD_SERVO0	4

#define BETWEEN_SONG_MS	5000

#define SONG_WAV	"i-want-candy.wav"
#define VOCALS_OPS	"vocals.ops"
#define DRUM_OPS	"vocals.ops"
#define GUITAR_OPS	"vocals.ops"
#define KEYBOARD_OPS	"vocals.ops"

static maestro_t *m;

static track_t *song;
static talking_skull_actor_t *vocals, *drum, *guitar, *keyboard;

static struct timespec start;

typedef struct {
    int		servo;
    int		eyes;
    int		eyes_high;
    int		eye_pin;
} servo_state_t;

static servo_state_t vocals_state = { VOCALS_SERVO, 0, 0, VOCALS_EYES };
static servo_state_t drum_state[2] = { { DRUM_SERVO0, 0, 0, -1 }, { DRUM_SERVO0+1, 0, 0, -1 } };
static servo_state_t guitar_state = { GUITAR_SERVO, 0, 0, -1 };
static servo_state_t keyboard_state[2] = { { KEYBOARD_SERVO0, 0, 0, -1 }, { KEYBOARD_SERVO0+1, 0, 0, -1 } };

static void
servo_update(void *state_as_vp, double new_pos)
{
    servo_state_t *s = (servo_state_t *) state_as_vp;
    int new_eyes;
    double pos;

    pos = new_pos;
    if (pos > 100) pos = 100;
    
    maestro_set_servo_pos(m, s->servo, pos);

    new_eyes = pos >= 50;
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
    servo_state_t *s = (servo_state_t *) state_as_vp;
    servo_update(&s[0], new_pos);
    servo_update(&s[1], 100-new_pos);
}

static void
rest_servos(void)
{
    servo_update(&vocals_state, 100);
    servo_update(&drum_state[0], 100);
    servo_update(&drum_state[1], 100);
    servo_update(&guitar_state, 0);
    servo_update(&keyboard_state[0], 100);
    servo_update(&keyboard_state[1], 100);
}

static void
init_servos(void)
{
    vocals = talking_skull_actor_new_ops(VOCALS_OPS, servo_update, &vocals_state);
    drum = talking_skull_actor_new_vsa(DRUM_OPS, drum_update, &drum_state);
    guitar = talking_skull_actor_new_vsa(GUITAR_OPS, servo_update, &guitar_state);
    keyboard = talking_skull_actor_new_vsa(KEYBOARD_OPS, keyboard_update, &keyboard_state);

    if (! vocals || ! drum || ! guitar || ! keyboard) {
	exit(1);
    }

    maestro_set_servo_physical_range(m, VOCALS_SERVO, 976, 1296);
    maestro_set_servo_is_inverted(m, VOCALS_SERVO, 1);
    maestro_set_servo_physical_range(m, DRUM_SERVO0, 1696, 2000);
    maestro_set_servo_physical_range(m, DRUM_SERVO0+1, 1696, 2000); // TBD
    maestro_set_servo_physical_range(m, GUITAR_SERVO, 1408, 2208);
    maestro_set_servo_physical_range(m, KEYBOARD_SERVO0, 850, 1500);
    maestro_set_servo_physical_range(m, KEYBOARD_SERVO0+1, 1200, 1700);

    rest_servos();
}

int
main(int argc, char **argv)
{
    printf("\n\n\n**** STARTING ****\n\n\n");

    pi_usb_init();
    wb_init();

    wb_set(LIGHTS, 0);

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

    //while (1) {
	wb_set(LIGHTS, 0);
	rest_servos();

//        ween2019_wait_until_valid();

	ms_sleep(BETWEEN_SONG_MS);

	nano_gettime(&start);
	wb_set(LIGHTS, 1);
	talking_skull_actor_play(vocals);
	//talking_skull_actor_play(drum);
	//talking_skull_actor_play(guitar);
	//talking_skull_actor_play(keyboard);
	track_play(song);
    //}

    return 0;
}
