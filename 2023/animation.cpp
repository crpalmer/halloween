#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "pi.h"
#include "animation-station.h"
#include "audio.h"
#include "audio-player.h"
#include "file.h"
#include "httpd-server.h"
#include "i2c.h"
#include "mcp23017.h"
#include "random-utils.h"
#include "wav.h"
#include "wb.h"
#include "wifi.h"

static Audio *audio;
static AudioPlayer *audio_player;

class Bunny : public AnimationStationPopper {
public:
    Bunny(Output *output) : AnimationStationPopper(output) {
    }
};

class Gater : public AnimationStationPopper {
public:
    Gater(Output *output) : AnimationStationPopper(output) {
    }

    double up_ms_targer() { return 2; }
    double down_ms_target() { return 2; }
};

class Question : public AnimationStationPopper {
public:
    Question(Output *output) : AnimationStationPopper(output) {
	add_wav("laugh.wav");
    }

    bool triggered() override {
	AnimationStation::get()->blink();
	return AnimationStationPopper::triggered();
    }
};

class Pillar : public AnimationStationPopper {
public:
    Pillar(Output *output) : AnimationStationPopper(output) {
    }

    double up_ms_targer() { return 1; }
    double down_ms_target() { return 0.75; }
};

class Snake : public AnimationStationPopper {
public:
    Snake(Output *output) : AnimationStationPopper(output) {
    }

    double up_ms_targer() { return 1; }
    double down_ms_target() { return 3; }
};

class DebugHandler : public HttpdDebugHandler {
public:
    virtual HttpdResponse *open(std::string path) {
	if (path == "") return new HttpdResponse(AnimationStation::get()->to_string());
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

    AnimationStationAction *bunny = new Bunny(bunny_output);
    AnimationStationAction *gater = new Gater(gater_output);
    AnimationStationAction *question = new Question(question_output);
    AnimationStationAction *pillar = new Pillar(pillar_output);
    AnimationStationAction *snake = new Snake(snake_output);

    AnimationStation *station = AnimationStation::get();
    station->add("bunny", new AnimationStationButton(bunny, bunny_input, new Light(bunny_light)));
    station->add("gater", new AnimationStationButton(gater, gater_input, new Light(gater_light)));
    station->add("question", new AnimationStationButton(question, question_input, new Light(question_light)));
    station->add("pillar", new AnimationStationButton(pillar, pillar_input, new Light(pillar_light)));
    station->add("snake", new AnimationStationButton(snake, snake_input, new Light(snake_light)));

    HttpdServer &httpd = HttpdServer::get();
    httpd.add_prefix_handler("/debug", new DebugHandler());
    httpd.add_prefix_handler("/trigger", new TriggerHandler());
    httpd.start(5555);
}

int
main(int argc, char **argv)
{
    pi_init_with_threads(threads_main, argc, argv);
}
