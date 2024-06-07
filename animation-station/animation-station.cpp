#include <string.h>
#include "pi.h"
#include "time-utils.h"
#include "random-utils.h"
#include "animation-station.h"

bool AnimationStationPopper::add_wav(std::string wav) {
    return random_audio.add(wav.c_str());
}

bool AnimationStationPopper::act() {
    if (random_audio.is_empty()) {
	for (int i = 0; i < 3; i++) attack_once();
    } else {
        random_audio.play_random(player);
        while (player->is_active()) attack_once();
    }
    return true;
}

void AnimationStationPopper::attack_once() {
    output->set(true);
    ms_sleep(up_ms());
    output->set(false);
    ms_sleep(down_ms());
}

void AnimationStationButton::main() {
    bool old_state = button->get();
    while (1) {
	pause();

	bool new_state = button->get();
	if (old_state != new_state && new_state) {
	    pressed();
	}
	old_state = button->get();	// It may have changed while we were running
    }
}

void AnimationStationButton::on_change() {
    resume_from_isr();
}

AnimationStation::AnimationStation() : PiThread("animation-station") {
    lock = new PiMutex();
    cond = new PiCond();
    nano_gettime(&start_time);
    start();
}

void AnimationStation::main() {
    while (true) {
	lock->lock();
	while (! (active_prop == "" && triggered_prop != "")) cond->wait(lock);
	active_prop = triggered_prop;
	triggered_prop = "";
	lock->unlock();

	assert(actions[active_prop]);
	actions[active_prop]->act();

	lock->lock();
	active_prop = "";
	lock->unlock();
    }
}

void AnimationStation::add(std::string name, AnimationStationAction *action) {
    lock->lock();
    actions[name] = action;
    lock->unlock();
}

bool AnimationStation::trigger_async(std::string prop) {
    bool ret = false;

    if (actions[prop] && lock->trylock()) {
	if (triggered_prop == "") {
	    triggered_prop = prop;
	    cond->signal();
	    ret = true;
	}
        lock->unlock();
    }
    return ret;
}

bool AnimationStation::trigger(std::string prop) {
    bool ret = false;
    if (lock->trylock()) {
	if (active_prop == "" && triggered_prop == "") {
	    active_prop = prop;
	    ret = actions[prop]->act();
	}
	lock->unlock();
    }
    return ret;
}


std::string AnimationStation::to_string() {
    std::string s = "Actions:\n------";
    for (auto kv : actions) {
	s += "\n" + kv.first;
    }
    s += "\n\nUptime: " + std::to_string(nano_elapsed_ms_now(&start_time)/1000.0) + " secs\n";
    return s;
}
