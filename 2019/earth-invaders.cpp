#include <stdio.h>
#include <pthread.h>
#include "l298n.h"
#include "pca9685.h"
#include "mcp23017.h"
#include "util.h"

#define OPTO_IS_TRIGGERED true
#define SPEED	1

static input_t *forward_es, *backward_es;
static motor_t *motor;
static double speed = 0;

static void
change_motor(bool is_forward)
{
printf("motor going %s\n", is_forward ? "forward" : "backward");
    motor->speed(0);
    ms_sleep(50);
    motor->direction(is_forward);
    motor->speed(speed);
}

static void *
motor_main(void *unused)
{
    bool is_forward = true;

    while (1) {
	if (is_forward && forward_es->get() == OPTO_IS_TRIGGERED) {
	    is_forward = false;
	    change_motor(is_forward);
	} else if (! is_forward && backward_es->get() == OPTO_IS_TRIGGERED) {
	    is_forward = true;
	    change_motor(is_forward);
	}
    }
}

static void
motor_start(void)
{
    speed = SPEED;
    motor->speed(speed);
}

static void
motor_stop(void)
{
    speed = 0;
    motor->speed(speed);
}

int main(int argc, char **argv)
{
    gpioInitialise();
    seed_random();

    PCA9685 *pca = new PCA9685();
    MCP23017 *mcp = new MCP23017();

    backward_es = mcp->get_input(1, 0);
    forward_es = mcp->get_input(1, 1);
    motor = new L298N(pca->get_output(0), pca->get_output(1), pca->get_output(2));

    motor->speed(0);
    motor->direction(true);

if (argc > 1) return(0);

    pthread_t thread;
    pthread_create(&thread, NULL, motor_main, NULL);

    while (1) {
	char buf[1024];
	printf("hit enter to start: "); fflush(stdin);
	fgets(buf, sizeof(buf), stdin);
	motor_start();
	ms_sleep(10*1000);
	motor_stop();
    }
}
