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
#include "pico-slave.h"
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

    bool act(bool play_all_audio) override {
	lights->off();
	light->on();
	bool ret = AnimationStationPopper::act(false);
	lights->off();
	lights->chase();
	return ret;
    }

protected:
    std::string name;
    Light *light = light;
};

class Coin : public Button {
public:
    Coin(Input *button, Output *output, Light *light) : Button("Coin", button, output, light) {
	add_wav("coin.wav");
	start();
    }
};

class Plant : public Button {
public:
    Plant(Input *button, Output *output, Light *light) : Button("Plant", button, output, light) {
	add_wav("plant.wav");
	start();
    }

    int up_ms() override { return 5000; }
    int down_ms() override { return 0; }
};

class Question : public Button {
public:
    Question(Input *button, Output *output, Light *light) : Button("Question", button, output, light) {
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

class Bob : public Button {
public:
    Bob(Input *button, Output *output, Light *light) : Button("Bob", button, output, light) {
	add_wav("bob.wav");
	start();
    }
};

class Cheep : public Button {
public:
    Cheep(Input *button, Output *output, Light *light) : Button("Cheep", button, output, light) {
	add_wav("bubble.wav");
	start();
    }

    double up_ms_target() override { return 2; }
    double down_ms_target() override { return 2; }
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

    new Coin(input[1], output[1], light[1]);
    new Plant(input[2], output[2], light[2]);
    new Question(input[3], output[3], light[3]);
    new Bob(input[4], output[4], light[4]);
    new Cheep(input[5], output[5], light[5]);

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
