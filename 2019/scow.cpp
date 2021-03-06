#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "externals/PIGPIO/pigpio.h"
#include "animation-station.h"
#include "l298n.h"
#include "mcp23017.h"
#include "track.h"
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

class Button : public AnimationStationAction {
public:
    Button() {
	stop = stop_new();

	light = wb_get_output(2, 8);
	button = wb_get_input(5);
	button->set_pullup_down();
	button->set_debounce(10);

	if ((track = track_new("scow.wav")) == NULL) exit(1);
    }

    output_t *get_light() override { return light; }
    bool is_triggered() override { return button->get(); }
    void act(Lights *lights) override {
	scow();
    }

    void scow() {
	struct timespec start;

	nano_gettime(&start);
	stop_reset(stop);
	track_play_asynchronously(track, stop);

	up();

	while (nano_elapsed_ms_now(&start) < 20*1000 && high_es->get() != ES_HIT) {}

	down();

	stop_request_stop(stop);
	nano_gettime(&start);
	while (nano_elapsed_ms_now(&start) < 20*1000 && low_es->get() != ES_HIT) {}
	motor->stop();
    }

private:
    output_t *light;
    input_t *button;
    track_t *track;
    stop_t *stop;
};

class ScowAction : public AnimationStationAction {
public:
    ScowAction(Button *scow) { this->scow = scow; }

    char *handle_remote_cmd(const char *cmd) override {
        printf("cmd: [%s]\n", cmd);
        if (strcmp(cmd, "scow") == 0) {
	    scow->scow();
	    return strdup("ok scow done\n");
        }
        printf("cmd not recognized\n");
        return NULL;
    }

private:
    Button *scow;
};

int
main(int argc, char **argv)
{
    gpioInitialise();
    seed_random();
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
    } else if (argc > 1) {
	printf("stopping the motor\n");
	exit(0);
    } else {
	AnimationStation *as = new AnimationStation();
	Button *scow = new Button();
	as->add_action(scow);
        as->add_action(new ScowAction(scow));
	AnimationStationController *asc = new AnimationStationController();
	asc->add_station(as);
	asc->main();
    }
}

