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

    int get_n_acts() { return n_acts; }
    bool is_disabled() { return disabled; }

protected:
    int n_acts = 0;
    bool disabled = false;

    void deserialize_state(const char *serialized);
    std::string serialize_state();
    void disable() { disabled = true; }
    void enable() { disabled = false; }

    friend class AnimationStation;
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

    bool load_state();

    void add(std::string name, AnimationStationAction *action);

    bool trigger_async(std::string name);
    bool trigger(std::string name);

    bool disable(std::string name) {
	bool ret = (actions.count(name));
	if (ret) {
	    actions[name]->disable();
	    save_state();
	}
	return ret;
    }

    bool enable(std::string name) {
	bool ret = (actions.count(name));
	if (ret) {
	    actions[name]->enable();
	    save_state();
	}
	return ret;
    }

    std::string to_string();

    auto get_active_prop() { return active_prop != "" ? active_prop : triggered_prop; }

    // TODO:: Figure out an iterator
    auto get_actions() { return actions; }

    void main() override;

private:
    AnimationStation();
    bool trigger_common();
    bool save_state();

private:
    struct timespec start_time;
    struct timespec last_save;

    bool save_dirty = false;
    const int save_version = 0;
#ifdef PLATFORM_pico
    const int save_every_ms = 60*1000;
#else
    const int save_every_ms = 0;
#endif
    const char *save_filename = "animation.sav";

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
