#include <string.h>
#include "pi.h"
#include "time-utils.h"
#include "random-utils.h"
#include "animation-station.h"

bool AnimationStationPopper::add_wav(std::string wav) {
    return random_audio.add(wav.c_str());
}

bool AnimationStationPopper::triggered() {
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

bool AnimationStationButton::triggered() {
    AnimationStation *station = AnimationStation::get();

    if  (! station->try_to_trigger()) return false;
    on();
    bool ret = action->triggered();
    off();
    station->trigger_done();
    return ret;
}

void AnimationStationButton::main() {
    bool old_state = button->get();
    while (1) {
	pause();

	bool new_state = button->get();
	if (old_state != new_state && new_state) {
	    action->triggered();
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
    if (! lights) lights = new Lights();
    actions[name] = action;
    lock->unlock();
}

void AnimationStation::add(std::string name, AnimationStationButton *button) {
    lock->lock();
    if (! lights) lights = new Lights();
    buttons[name] = button;
    lock->unlock();
}

bool AnimationStation::trigger(std::string prop) {
    if (actions[prop]) return actions[prop]->triggered();
    if (buttons[prop]) return buttons[prop]->triggered();
    return false;
}

bool AnimationStation::try_to_trigger() {
    if (lock->trylock()) {
        lights->off();
	return true;
    } else {
	return false;
    }
}

void AnimationStation::trigger_done() {
    lock->unlock();
    lights->chase();
}

std::string AnimationStation::to_string() {
    std::string s = "Actions:\n------";
    for (auto kv : actions) {
	s += "\n" + kv.first;
    }
    s += "\n\nButtons:\n------";
    for (auto kv : buttons) {
	s += "\n " + kv.first;
    }
    s += "\n\nUptime: " + std::to_string(nano_elapsed_ms_now(&start_time)/1000.0) + " secs\n";
    return s;
}
