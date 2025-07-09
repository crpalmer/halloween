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
#include "lights.h"
#include "random-utils.h"
#include "setup.h"
#include "wav.h"
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

    bool act(bool play_all_audio) override {
	lights->off();
	light->on();
	bool ret = AnimationStationPopper::act(play_all_audio);
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

    bool act(bool play_all_audio) override {
	lights->blink_all();
	bool ret = AnimationStationPopper::act(play_all_audio);
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
    seed_random();
    wifi_init(NULL);

    audio = Audio::create_instance();
    audio_player = new AudioPlayer(audio);

    lights = new Lights();
    lights->set_blink_ms(500);

    Input *bunny_input = prop_get_input(1);
    Output *bunny_light = prop_get_light(1);
    Output *bunny_output = prop_get_output(1);
    new Bunny(bunny_input, bunny_output, new Light(bunny_light));

    Input *gater_input = prop_get_input(2);
    Output *gater_light = prop_get_light(2);
    Output *gater_output = prop_get_output(2);
    new Gater(gater_input, gater_output, new Light(gater_light));

    Input *question_input = prop_get_input(3);
    Output *question_light = prop_get_light(3);
    Output *question_output = prop_get_output(3);
    new Question(question_input, question_output, new Light(question_light));

    Input *pillar_input = prop_get_input(4);
    Output *pillar_light = prop_get_light(4);
    Output *pillar_output = prop_get_output(4);
    new Pillar(pillar_input, pillar_output, new Light(pillar_light));

    Input *snake_input = prop_get_input(5);
    Output *snake_light = prop_get_light(5);
    Output *snake_output = prop_get_output(5);
    new Snake(snake_input, snake_output, new Light(snake_light));

    auto station = AnimationStation::get();
    station->load_state();

    lights->chase();

    HttpdServer *httpd = new HttpdServer();
    const std::string station_base = "/station";

    new AnimationStationUI(httpd, station_base);
    httpd->add_file_handler("/", new HttpdRedirectHandler(station_base + "/"));
    httpd->add_file_handler("/index.html", new HttpdRedirectHandler(station_base + "/"));
    httpd->add_file_handler(station_base + "/ui.css", new HttpdFileHandler(ANIMATION_STATION_CSS_FILENAME));
    httpd->add_prefix_handler("/debug", new DebugHandler());
    httpd->add_prefix_handler("/trigger", new TriggerHandler());
    httpd->start();
}

int
main(int argc, char **argv)
{
    pi_init_with_threads(threads_main, argc, argv);
}
