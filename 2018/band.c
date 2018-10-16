#include <stdio.h>
#include <stdlib.h>
#include "maestro.h"
#include "pi-usb.h"
#include "talking-skull.h"
#include "time-utils.h"
#include "track.h"
#include "util.h"
#include "wb.h"

#define EYE_BANK	1
#define LEAD_LEFT_EYE	5
#define LEAD_RIGHT_EYE	6
#define BACKUP_EYES	7

#define LEAD_SERVO	0
#define BACKUP_SERVO	1
#define BASS_SERVO	2
#define TRIANGLE_SERVO	3

#define BETWEEN_SONG_MS	1000

#define SONG_WAV	"brush.wav"
#define LEAD_VSA	"brush-lead.csv"
#define BACKUP_VSA	"brush-backup.csv"
#define BASS_VSA	"brush-bass.csv"
#define TRIANGLE_VSA	"brush-triangle.csv"

static maestro_t *m;

static track_t *song;
static talking_skull_actor_t *lead, *backup, *bass;

static struct timespec start;

typedef struct {
    int		servo;
    int		eyes;
    int		eye_pins[2];
} servo_state_t;

static servo_state_t lead_state = { LEAD_SERVO, 0, { LEAD_LEFT_EYE, LEAD_RIGHT_EYE } };
static servo_state_t backup_state = { BACKUP_SERVO, 0, { BACKUP_EYES, -1 } };
static servo_state_t bass_state   = { BASS_SERVO,   0, { -1, -1 } };

static void
servo_update(void *state_as_vp, double new_pos)
{
    servo_state_t *s = (servo_state_t *) state_as_vp;
    int new_eyes;
    double pos;

    pos = new_pos;
    if (pos > 100) pos = 100;
    
    maestro_set_servo_pos(m, s->servo, pos);

    new_eyes = pos < 50;
    if (s->eyes != new_eyes) {
	if (s->eye_pins[0] >= 0) wb_set(EYE_BANK, s->eye_pins[0], new_eyes);
	if (s->eye_pins[1] >= 0) wb_set(EYE_BANK, s->eye_pins[1], new_eyes);
	s->eyes = new_eyes;
    }
}

static void
rest_servos(void)
{
    servo_update(&lead_state, 100);
    servo_update(&backup_state, 0);
    servo_update(&bass_state, 50);
}

static void
init_servos(void)
{
    lead = talking_skull_actor_new_vsa(LEAD_VSA, servo_update, &lead_state);
    backup = talking_skull_actor_new_vsa(BACKUP_VSA, servo_update, &backup_state);
    bass = talking_skull_actor_new_vsa(BASS_VSA, servo_update, &bass_state);

    if (! lead || ! backup || ! bass) {
	exit(1);
    }

    maestro_set_servo_physical_range(m, LEAD_SERVO, 896, 1248);
    maestro_set_servo_physical_range(m, BACKUP_SERVO, 560, 800);
    maestro_set_servo_physical_range(m, BASS_SERVO, 1408, 1904);
    /* TODO: add other ranges here */

    rest_servos();
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

    while (1) {
	rest_servos();

	ms_sleep(BETWEEN_SONG_MS);

	nano_gettime(&start);
	talking_skull_actor_play(lead);
	talking_skull_actor_play(backup);
	talking_skull_actor_play(bass);
	track_play(song);
    }

    return 0;
}
