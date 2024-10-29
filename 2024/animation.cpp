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
#include "wav.h"
#include "wifi.h"
#include "animation-setup.h"

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

class Kyle : public Button {
public:
    Kyle(Input *button, Output *output, Light *light) : Button("Kyle", button, output, light) {
	start();
    }

    double up_ms_targer() { return 2; }
    double down_ms_target() { return 2; }
};

class Pollito : public Button {
public:
    Pollito(Input *button, Output *output, Light *light) : Button("Pollito", button, output, light) {
	start();
    }
};

class Question : public Button {
public:
    Question(Input *button, Output *output, Light *light) : Button("Question", button, output, light) {
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

class Banana : public Button {
public:
    Banana(Input *button, Output *output, Light *light) : Button("Banana-Pop", button, output, light) {
	start();
    }
};

class FartBlaster : public Button {
public:
    FartBlaster(Input *button, Output *output, Light *light, Input *ready) : Button("Fart-Blaster", button, output, light), ready(ready) {
	ready->set_pullup_down();
	ready->set_is_inverted();
	start();
    }

    bool act() override {
	lights->blink_all();
	output->on();
	ms_sleep(100);
	printf("ready: %d\n", ready->get());
	while (ready->get()) ms_sleep(1);
	output->off();
	lights->off();
	lights->chase();
	return true;
    }
	
private:
    Input *ready;
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
    Input *input[6];
    Output *output[6];
    Light *light[6];

    lights = new Lights();
    lights->set_blink_ms(500);

    for (int i = 1; i <= 5; i++) {
	input[i] = prop_get_input(i);
	output[i] = prop_get_output(i);
	light[i] = new Light(prop_get_light(i));
    }

    new Kyle(input[1], output[1], light[1]);
    new Pollito(input[2], output[2], light[2]);
    new Question(input[3], output[3], light[3]);
    new Banana(input[4], output[4], light[4]);
    new FartBlaster(input[5], output[5], light[5], prop_get_extra_input(5));

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

int
main(int argc, char **argv)
{
    pi_init_with_threads(threads_main, argc, argv);
}
