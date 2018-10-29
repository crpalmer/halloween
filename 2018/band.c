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
#include "ween2018.h"

#define EYE_BANK	1
#define LEAD_EYES	5
#define BACKUP_EYES	6
#define LIGHTS		1,1

#define LEAD_SERVO	0
#define BACKUP_SERVO	1
#define BASS_SERVO	2
#define TRIANGLE_SERVO	3

#define BETWEEN_SONG_MS	5000
#define TRIANGLE_SPEED	1000
#define TRIANGLE_DELAY	250

#define SONG_WAV	"brush.wav"
#define LEAD_VSA	"brush-lead.csv"
#define BACKUP_VSA	"brush-backup.csv"
#define BASS_VSA	"brush-bass.csv"
#define TRIANGLE_VSA	"brush-triangle.csv"

static maestro_t *m;

static track_t *song;
static talking_skull_actor_t *lead, *backup, *bass, *triangle;

static struct timespec start;

typedef struct {
    int		servo;
    int		eyes;
    int		eyes_high;
    int		eye_pin;
} servo_state_t;

typedef struct {
    int		active;
    struct timespec start;
    int		is_high;
} triangle_state_t;

static servo_state_t lead_state = { LEAD_SERVO, 0, 0, LEAD_EYES };
static servo_state_t backup_state = { BACKUP_SERVO, 0, 1, BACKUP_EYES };
static servo_state_t bass_state   = { BASS_SERVO,   0, 0, -1 };
static triangle_state_t triangle_state = { 0, };

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
triangle_servo_to_rest_pos()
{
    maestro_set_servo_speed(m, TRIANGLE_SERVO, 0);
    maestro_set_servo_pos(m, TRIANGLE_SERVO, 0);
}

static void
triangle_rest(triangle_state_t *s)
{
    triangle_servo_to_rest_pos();
    s->active = 0;
    s->is_high = 0;
}

static void *
triangle_main(void *s_as_vp)
{
    triangle_state_t *s = (triangle_state_t *) s_as_vp;
    int is_active;

    while (1) {
	int go = 0;

	if (is_active && ! s->active) {
	    triangle_servo_to_rest_pos(s);
	} else if (! is_active && s->active) {
	    go = 1;
	    maestro_set_servo_speed(m, TRIANGLE_SERVO, TRIANGLE_SPEED);
	} else if (is_active && nano_elapsed_ms_now(&s->start) > TRIANGLE_DELAY) {
	    go = 1;
	}

	is_active = s->active;

	if (go) {
	    s->is_high = ! s->is_high;
	    maestro_set_servo_pos(m, TRIANGLE_SERVO, s->is_high ? 100 : 0);
	    nano_gettime(&s->start);
	}
    }
    return NULL;
}

static void
triangle_update(void *s_as_vp, double new_pos)
{
    triangle_state_t *s = (triangle_state_t *) s_as_vp;

    s->active = new_pos > 0;
}


static void
rest_servos(void)
{
    servo_update(&lead_state, 100);
    servo_update(&backup_state, 0);
    servo_update(&bass_state, 49);
    triangle_rest(&triangle_state);
}

static void
init_servos(void)
{
    pthread_t triangle_thread;

    lead = talking_skull_actor_new_vsa(LEAD_VSA, servo_update, &lead_state);
    backup = talking_skull_actor_new_vsa(BACKUP_VSA, servo_update, &backup_state);
    bass = talking_skull_actor_new_vsa(BASS_VSA, servo_update, &bass_state);
    triangle = talking_skull_actor_new_vsa(TRIANGLE_VSA, triangle_update, &triangle_state);

    if (! lead || ! backup || ! bass || ! triangle) {
	exit(1);
    }

    maestro_set_servo_physical_range(m, LEAD_SERVO, 896, 1248);
    maestro_set_servo_physical_range(m, BACKUP_SERVO, 560, 800);
    maestro_set_servo_physical_range(m, BASS_SERVO, 1408, 1904);
    maestro_set_servo_physical_range(m, TRIANGLE_SERVO, 1024, 1296);

    pthread_create(&triangle_thread, NULL, triangle_main, &triangle_state);

    rest_servos();
}

int
main(int argc, char **argv)
{
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

    while (1) {
	wb_set(LIGHTS, 0);
	rest_servos();

        ween2018_wait_until_valid();

	ms_sleep(BETWEEN_SONG_MS);

	nano_gettime(&start);
	wb_set(LIGHTS, 1);
	talking_skull_actor_play(lead);
	talking_skull_actor_play(backup);
	talking_skull_actor_play(bass);
	talking_skull_actor_play(triangle);
	track_play(song);
    }

    return 0;
}
