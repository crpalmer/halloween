#include <stdio.h>
#include <pthread.h>
#include "digital-counter.h"
#include "earth-invaders-io.h"
#include "l298n.h"
#include "pca9685.h"
#include "mcp23017.h"
#include "time-utils.h"
#include "track.h"
#include "util.h"

#define SPEED	1

static double speed = 0;

static earth_invaders_io_t *io;

static track_t *hit_track;

static void
change_motor(bool direction)
{
printf("motor going %s\n", direction ? "forward" : "backward");
    io->motor->speed(0);
    ms_sleep(50);
    io->motor->direction(direction);
    io->motor->speed(speed);
}

static void *
motor_main(void *unused)
{
    bool direction = true;

    while (1) {
	if (direction == TO_IDLER && io->endstop_idler->get() == ENDSTOP_TRIGGERED) {
	    direction = TO_MOTOR;
	    change_motor(direction);
	} else if (direction == TO_MOTOR && io->endstop_motor->get() == ENDSTOP_TRIGGERED) {
	    direction = TO_IDLER;
	    change_motor(direction);
	}
    }
}

static void
motor_start(void)
{
    speed = SPEED;
    io->motor->speed(speed);
}

static void
motor_stop(void)
{
    speed = 0;
    io->motor->speed(speed);
}

static void
game(void)
{
    struct timespec start;
    int hits = 0;

    nano_gettime(&start);
    while (nano_elapsed_ms_now(&start) < 30000) {
	if (io->targets[0]->get() == TARGET_HIT && (io->triggers[0]->get() == BUTTON_PUSHED || io->triggers[1]->get() == BUTTON_PUSHED)) {
	    printf("hit\n");
	    io->score[0]->set(++hits);
	    track_play_asynchronously(hit_track, NULL);
	    while (io->triggers[0]->get() == BUTTON_PUSHED) {}
	    while (io->triggers[1]->get() == BUTTON_PUSHED) {}
	}
    }
}

int main(int argc, char **argv)
{
    gpioInitialise();
    seed_random();

printf("EARTH INVADERS IS DISABLED\n");
return 0;

    io = new earth_invaders_io_t();

    io->motor->speed(0);
    io->motor->direction(true);

if (argc > 1) return(0);

    hit_track = track_new("hit.wav");

    pthread_t thread;
    pthread_create(&thread, NULL, motor_main, NULL);

    while (1) {
	char buf[1024];
	printf("hit enter to start: "); fflush(stdin);
	fgets(buf, sizeof(buf), stdin);
	motor_start();
	io->laser->set(1);
        io->score[0]->set(0);
	game();
	io->laser->set(0);
	motor_stop();
    }
}
