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

AnimationStation::AnimationStation() {
    lock = new PiMutex();
    nano_gettime(&start_time);
}

void AnimationStation::add(std::string name, AnimationStationAction *action) {
    lock->lock();
    actions[name] = action;
    lock->unlock();
}

bool AnimationStation::trigger(std::string prop) {
    if (actions[prop] && lock->trylock()) {
	actions[prop]->act();
	lock->unlock();
	return true;
    }
    return false;
}

std::string AnimationStation::to_string() {
    std::string s = "Actions:\n------";
    for (auto kv : actions) {
	s += "\n" + kv.first;
    }
    s += "\n\nUptime: " + std::to_string(nano_elapsed_ms_now(&start_time)/1000.0) + " secs\n";
    return s;
}
