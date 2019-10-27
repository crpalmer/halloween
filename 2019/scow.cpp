#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "externals/PIGPIO/pigpio.h"
#include "l298n.h"
#include "mcp23017.h"
#include "util.h"
#include "wb.h"

static L298N *motor;
static MCP23017 *mcp;
static input_t *low_es, *high_es;

#define ES_HIT 1
#define UP false
#define DOWN true

static void
up()
{
   motor->change_motor(UP);
}

static void
down()
{
   motor->change_motor(DOWN);
}

static void
test_es(const char *name, input_t *es)
{
    if (es->get_with_debounce() == ES_HIT) printf("  *** %s ENDSTOP ALREADY TRIGGERED!\n", name);
    printf("Trigger %s end-stop: ", name); fflush(stdout);
    while (es->get_with_debounce() != ES_HIT) {}
    printf("triggered\n");
}

static void
test_motor()
{
    char buf[1000];

    printf("center the motor: "); fflush(stdout);
    fgets(buf, sizeof(buf), stdin);
    printf("motor up\n");
    up();
    ms_sleep(2*1000);
    printf("motor down\n");
    down();
    ms_sleep(2*1000);
    motor->stop();
}

static void
test()
{
    test_es("low", low_es);
    test_es("high", high_es);
    test_motor();
}

static void
test_motor_es()
{
     printf("going up\n");
     up();
     while (high_es->get_with_debounce() != ES_HIT) {}
     down();
     printf("going down\n");
     while (low_es->get_with_debounce() != ES_HIT) {}
     motor->stop();
}

static input_t *
get_es(int i)
{
    input_t *es = wb_get_input(i);

    es->set_pullup_down();
    es->set_debounce(10);

    return es;
}

static void
scow()
{
    struct timespec start;

    nano_gettime(&start);
    up();
    while (nano_elapsed_ms_now(&start) < 20*1000 && high_es->get() != ES_HIT) {}
    down();
printf("%d up\n", nano_elapsed_ms_now(&start));
    nano_gettime(&start);
    while (nano_elapsed_ms_now(&start) < 20*1000 && low_es->get() != ES_HIT) {}
    motor->stop();
printf("%d down\n", nano_elapsed_ms_now(&start));
}

int
main(int argc, char **argv)
{
    gpioInitialise();
    wb_init_v2();

    low_es = get_es(1);
    high_es = get_es(2);
    
    mcp = new MCP23017();

    motor = new L298N(mcp->get_output(0), mcp->get_output(1), mcp->get_output(2));
    motor->stop();

    if (argc == 2 && strcmp(argv[1], "--test") == 0) {
	test();
	exit(0);
    } else if (argc == 2 && strcmp(argv[1], "--motor") == 0) {
	test_motor();
	exit(0);
    } else if (argc == 2 && strcmp(argv[1], "--motor-es") == 0) {
	test_motor_es();
	exit(0);
    } else if (argc ==2 && strcmp(argv[1], "--scow") == 0) {
	scow();
    } else if (argc > 1) {
	printf("stopping the motor\n");
	exit(0);
    }
}

