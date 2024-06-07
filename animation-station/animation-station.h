#ifndef __ANIMATION_STATION_H__
#define __ANIMATION_STATION_H__

#include <list>
#include <map>
#include "audio-player.h"
#include "lights.h"
#include "io.h"
#include "random-audio.h"
#include "time-utils.h"

class AnimationStationAction;

class AnimationStationAction {
public:
    virtual bool triggered() = 0;
};

class AnimationStationPopper : public AnimationStationAction {
public:
    AnimationStationPopper(Output *output, AudioPlayer *player = NULL) : output(output), player(player) {
    }

    bool add_wav(std::string wav);

    bool triggered() override;

    virtual double up_ms_target() { return 1.0; }
    int up_ms() { return (500 + random_number_in_range(0, 250) - 125)*up_ms_target(); }

    virtual double down_ms_target() { return 2.5; }
    int down_ms() { return (200 + random_number_in_range(0, 100) - 50)*down_ms_target(); }

private:
    void attack_once();

private:
    Output *output;
    AudioPlayer *player;
    RandomAudio random_audio;
};

class AnimationStationButton : public PiThread, public InputNotifier, public Light {
public:
    AnimationStationButton(AnimationStationAction *action, Input *button, Light *light) : action(action), button(button), light(light) {
    }

    virtual bool triggered();

    void off() { light->off(); }
    void on() { light->on(); }
    void set(bool on) { light->set(on); }

    void main() override;
    void on_change() override;

private:
    AnimationStationAction *action;
    Input *button;
    Light *light;
};

class AnimationStation {
public:
    static AnimationStation *get() {
	static AnimationStation instance;
	return &instance;
    }

    void blink() { lights->blink_all(); }

    void add(std::string name, AnimationStationAction *action);
    void add(std::string name, AnimationStationButton *button);

    bool trigger(std::string name);
    bool try_to_trigger();
    void trigger_done();

    std::string to_string();

private:
    AnimationStation();

private:
    struct timespec start_time;
    Lights *lights;
    PiMutex *lock;
    std::map<std::string, bool> disabled;
    std::map<std::string, AnimationStationAction *> actions;
    std::map<std::string, AnimationStationButton *> buttons;
};

#endif
