#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pi.h"
#include "animation-station.h"
#include "animation-ui.h"
#include "audio.h"
#include "audio-player.h"
#include "l298n.h"
#include "lights.h"
#include "mcp23017.h"
#include "random-utils.h"
#include "wav.h"

static Audio *audio;
static AudioPlayer *player;
static Lights *lights;

static L298N *motor;
static MCP23017 *mcp;

static Input *low_es, *high_es;

#define UP true
#define DOWN false

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
test_es(const char *name, Input *es)
{
    if (es->get()) printf("  *** %s ENDSTOP ALREADY TRIGGERED!\n", name);
    printf("Trigger %s end-stop: ", name); fflush(stdout);
    while (! es->get()) {}
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
     while (! high_es->get()) {}
     down();
     printf("going down\n");
     while (! low_es->get()) {}
     motor->stop();
}

class Gus : public AnimationStationButton, AnimationStationAction {
public:
    Gus() : AnimationStationButton("gus", mcp->get_input(0, 7)) {
	button->set_pullup_up();
	button->set_is_inverted();
	light = new Light(mcp->get_output(1, 0));
	lights->add(light);
	lights->chase();
	AnimationStation::get()->add("gus", this);
	start();
    }

    bool supports_on_change() override { return false; }

    bool act() override {
	struct timespec start;

	light->on();

	nano_gettime(&start);
	//player->play(wav);

	up();

	while (nano_elapsed_ms_now(&start) < 20*1000 && ! high_es->get()) {}

	down();

	player->stop();
	nano_gettime(&start);
	while (nano_elapsed_ms_now(&start) < 20*1000 && ! low_es->get()) {}
	motor->stop();

	lights->chase();

	return true;
    }

private:
    Light *light;
    AudioBuffer *wav;
};

class DebugHandler : public HttpdDebugHandler {
public:
    virtual HttpdResponse *open(std::string path) {
        return HttpdDebugHandler::open(path);
    }
};

class TriggerHandler : public HttpdPrefixHandler {
public:
    TriggerHandler() : HttpdPrefixHandler() { }
    HttpdResponse *open(std::string path) {
        AnimationStation *station = AnimationStation::get();
        bool res = station->trigger(path);
        return new HttpdResponse(std::to_string(res) + " " + path);
    }
};

int
main(int argc, char **argv)
{
    pi_init();
    seed_random();
    
    audio = Audio::create_instance();
    player = new AudioPlayer(audio);
    mcp = new MCP23017();
    lights = new Lights();

    low_es = mcp->get_input(1, 6);
    low_es->set_pullup_up();
    low_es->set_is_inverted();
    high_es = mcp->get_input(1, 7);
    high_es->set_pullup_up();
    high_es->set_is_inverted();

    motor = new L298N(mcp->get_output(0, 0), mcp->get_output(0, 1), mcp->get_output(0, 2));
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
	new Gus();
	auto station = AnimationStation::get();
	station->load_state();

        lights->chase();

        auto httpd = HttpdServer::get();
        const std::string station_base = "/station";

        new AnimationStationUI(station_base);
        httpd->add_file_handler("/", new HttpdRedirectHandler(station_base + "/"));
        httpd->add_file_handler("/index.html", new HttpdRedirectHandler(station_base + "/"));
        httpd->add_file_handler(station_base + "/ui.css", new HttpdFileHandler(ANIMATION_STATION_CSS_FILENAME));
        httpd->add_prefix_handler("/debug", new DebugHandler());
        httpd->add_prefix_handler("/trigger", new TriggerHandler());
        httpd->start(80);
    }
}

