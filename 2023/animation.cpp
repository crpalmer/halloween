#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "pi.h"
#include "animation-station.h"
#include "animation-ui.h"
#include "audio.h"
#include "audio-player.h"
#include "file.h"
#include "httpd-server.h"
#include "i2c.h"
#include "lights.h"
#include "mcp23017.h"
#include "random-utils.h"
#include "wav.h"
#include "wb.h"
#include "wifi.h"

static Audio *audio;
static AudioPlayer *audio_player;
static Lights *lights;

class Button : public AnimationStationButton, public AnimationStationPopper {
public:
    Button(std::string name, Input *button, Output *output, Light *light) :
	AnimationStationButton(name, button),
	AnimationStationPopper(output, audio_player),
	name(name), light(light) {
	lights->add(light);
	AnimationStation::get()->add(name, this);
    }

    bool act() override {
	lights->off();
	light->on();
	bool ret = AnimationStationPopper::act();
	lights->off();
	lights->chase();
	return ret;
    }

protected:
    std::string name;
    Light *light = light;
};

class Bunny : public Button {
public:
    Bunny(Input *button, Output *output, Light *light) : Button("bunny", button, output, light) {
	start();
    }
};

class Gater : public Button {
public:
    Gater(Input *button, Output *output, Light *light) : Button("gater", button, output, light) {
	start();
    }

    double up_ms_targer() { return 2; }
    double down_ms_target() { return 2; }
};

class Question : public Button {
public:
    Question(Input *button, Output *output, Light *light) : Button("question", button, output, light) {
	add_wav("laugh.wav");
	start();
    }

    bool act() override {
	lights->blink_all();
	bool ret = AnimationStationPopper::act();
	lights->off();
	lights->chase();
	return ret;
    }
};

class Pillar : public Button {
public:
    Pillar(Input *button, Output *output, Light *light) : Button("pillar", button, output, light) {
	start();
    }

    double up_ms_targer() { return 1; }
    double down_ms_target() { return 0.75; }
};

class Snake : public Button {
public:
    Snake(Input *button, Output *output, Light *light) : Button("snake", button, output, light) {
	start();
    }

    double up_ms_targer() { return 1; }
    double down_ms_target() { return 3; }
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

void threads_main(int argc, char **argv) {
    gpioInitialise();
    seed_random();
    wifi_init(NULL);

#ifdef PLATFORM_pi
    audio = new AudioPi();
#else
    i2c_init_bus(1, 400*1000);
    i2c_config_gpios(2, 3);
    audio = new AudioPico();
#endif
    audio_player = new AudioPlayer(audio);

#if defined(PLATFORM_pi) && WEEN_BOARD
    WeenBoard *wb = new WeenBoard(1);
    Input *bunny_input = wb->get_input(1);
    Output *bunny_light = wb->get_output(1, 1);
    Output *bunny_output = wb->get_output(2, 1);
    Input *gater_input = wb->get_input(2);
    Output *gater_light = wb->get_output(1, 2);
    Output *gater_output = wb->get_output(2, 2);
    Input *question_input = wb->get_input(3);
    Output *question_light = wb->get_output(1, 3);
    Output *question_output = wb->get_output(2, 3);
    Input *pillar_input = wb->get_input(4);
    Output *pillar_light = wb->get_output(1, 4);
    Output *pillar_output = wb->get_output(2, 4);
    Input *snake_input = wb->get_input(5);
    Output *snake_light = wb->get_output(1, 5);
    Output *snake_output = wb->get_output(2, 5);
#else
    Input *bunny_input = new GPInput(4);
    Input *gater_input = new GPInput(5);
    Input *question_input = new GPInput(6);
    Input *pillar_input = new GPInput(7);
    Input *snake_input = new GPInput(8);
#ifdef PLATFORM_pi
    MCP23017 *mcp = new MCP23017();
    Output *bunny_light = mcp->get_output(1, 1);
    Output *bunny_output = mcp->get_output(2, 1);
    Output *gater_light = mcp->get_output(1, 2);
    Output *gater_output = mcp->get_output(2, 2);
    Output *question_light = mcp->get_output(1, 3);
    Output *question_output = mcp->get_output(2, 3);
    Output *pillar_light = mcp->get_output(1, 4);
    Output *pillar_output = mcp->get_output(2, 4);
    Output *snake_light = mcp->get_output(1, 5);
    Output *snake_output = mcp->get_output(2, 5);
#else
    Output *output = new GPOutput(9);
    Output *bunny_light = output;
    Output *bunny_output = output;
    Output *gater_light = output;
    Output *gater_output = output;
    Output *question_light = output;
    Output *question_output = output;
    Output *pillar_light = output;
    Output *pillar_output = output;
    Output *snake_light = output;
    Output *snake_output = output;
#endif
#endif

    lights = new Lights();
    lights->set_blink_ms(500);

    new Bunny(bunny_input, bunny_output, new Light(bunny_light));
    new Gater(gater_input, gater_output, new Light(gater_light));
    new Question(question_input, question_output, new Light(question_light));
    new Pillar(pillar_input, pillar_output, new Light(pillar_light));
    new Snake(snake_input, snake_output, new Light(snake_light));

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
    httpd->start(5555);
}

int
main(int argc, char **argv)
{
    pi_init_with_threads(threads_main, argc, argv);
}
