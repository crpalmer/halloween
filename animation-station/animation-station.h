#ifndef __ANIMATION_STATION_H__
#define __ANIMATION_STATION_H__

#include <list>
#include <map>
#include <algorithm>
#include "audio-player.h"
#include "io.h"
#include "random-audio.h"
#include "time-utils.h"

class AnimationStationAction;

class AnimationStationAction {
public:
    virtual bool act() = 0;
};

class AnimationStationPopper : public AnimationStationAction {
public:
    AnimationStationPopper(Output *output, AudioPlayer *player = NULL) : output(output), player(player) {
    }

    bool add_wav(std::string wav);

    bool act() override;

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

class AnimationStation : PiThread {
public:
    static AnimationStation *get() {
	static AnimationStation instance;
	return &instance;
    }
    virtual ~AnimationStation() { }

    void add(std::string name, AnimationStationAction *action);

    bool trigger_async(std::string name);
    bool trigger(std::string name);

    std::string to_string();

    auto get_active_prop() { return active_prop; }

    // TODO:: Figure out an iterator
    auto get_actions() { return actions; }

    void main() override;

private:
    AnimationStation();

private:
    struct timespec start_time;

    PiMutex *lock;
    PiCond *cond;
    std::string active_prop;
    std::string triggered_prop;

    std::map<std::string, AnimationStationAction *> actions;
};

class AnimationStationButton : public PiThread, public InputNotifier {
public:
    AnimationStationButton(std::string action, Input *button);

    virtual bool pressed() { return AnimationStation::get()->trigger(action); }

    void main() override;
    void on_change() override;

private:
    std::string action;
    Input *button;
};

#endif
