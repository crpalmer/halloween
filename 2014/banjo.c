#include <stdio.h>
#include <stdlib.h>
#include "call-every.h"
#include "maestro.h"
#include "pi-usb.h"
#include "track.h"
#include "util.h"

#define SERVO_ID	0
#define UPDATE_MS	40

#define INTER_TRACK_LOW 3000
#define INTER_TRACK_HIGH 5000

#define MIN_TICKS	3
#define MAX_TICKS	9

#define POS_LOW		32
#define POS_HIGH	58
#define SPEED_MS	(100 * (POS_HIGH - POS_LOW + 1) / 100)

typedef struct {
    bool is_playing;
    maestro_t *m;
    bool is_low;
    unsigned n_ticks_left;
} servo_state_t;

static void
update_servos(void *s_as_vp)
{
    servo_state_t *s = (servo_state_t *) s_as_vp;

    if (! s->is_playing) {
	return;
    }

    if (--s->n_ticks_left) {
	return;
    }

    s->n_ticks_left = random_number_in_range(MIN_TICKS, MAX_TICKS);
    s->is_low = !s->is_low;

    if (! maestro_set_servo_pos(s->m, SERVO_ID, s->is_low ? POS_LOW : POS_HIGH)) printf("set_target failed.\n");
}

int
main(int argc, char **argv)
{
    maestro_t *m;
    call_every_t *e;
    servo_state_t s;
    track_t *track;

    pi_usb_init();
    if ((m = maestro_new()) == NULL) {
	fprintf(stderr, "couldn't find a recognized device.\n");
	exit(1);
    }

    track = track_new("banjo.wav");
   
    printf("n-servos = %d\n", maestro_n_servos(m));

    e = call_every_new(UPDATE_MS, update_servos, (void *) &s);

    maestro_set_servo_speed(m, SERVO_ID, SPEED_MS);

    s.m = m;
    s.is_low = true;
    s.n_ticks_left = 1;
    s.is_playing = false;

    call_every_start(e);

    while (true) {
	s.is_playing = true;
	track_play(track);
	s.is_playing = false;
	ms_sleep(random_number_in_range(INTER_TRACK_LOW, INTER_TRACK_HIGH));
    }

    return 0;
}
