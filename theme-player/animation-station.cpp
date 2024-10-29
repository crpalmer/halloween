#include <string.h>
#include "pi.h"
#include "time-utils.h"
#include "random-utils.h"
#include "animation-station.h"

std::string AnimationStationAction::serialize_state() {
    return std::to_string(n_acts) + " " + std::to_string(n_acts_async) + " " + std::to_string(disabled);
}

void AnimationStationAction::deserialize_state(const char *buf) {
    n_acts = atoi(buf);
    while (*buf && *buf != ' ') buf++;
    if (*buf) buf++;
    n_acts_async = atoi(buf);
    while (*buf && *buf != ' ') buf++;
    if (*buf) buf++;
    disabled = atoi(buf);
}

bool AnimationStationPopper::add_wav(std::string wav) {
    return random_audio.add(wav.c_str());
}

bool AnimationStationPopper::act() {
    if (! player || random_audio.is_empty()) {
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

AnimationStationButton::AnimationStationButton(std::string action, Input *button) : PiThread(action.c_str()), action(action), button(button) {
    button->set_pullup_down();
    button->set_debounce(1);
    button->set_notifier(this);
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

bool AnimationStation::load_state() {
    file_t *f;
    bool ret = false;
    int version;

    if ((f = file_open(save_filename, "r")) == NULL) return false;

    const int n_buf = 1024;
    char *buf = (char *) fatal_malloc(1024);

    if (! file_gets(f, buf, n_buf)) goto done;
    version = atoi(buf);
    if (version != save_version) {
	consoles_printf("Saved version is %d but current version is %d\n", version, save_version);
	goto done;
    }
 
    while (file_gets(f, buf, n_buf)) {
	char *serialized = buf;
	int len = atoi(buf);

	while (*serialized && *serialized != ' ') serialized++;
	if (*serialized) serialized++;
	char *name = serialized;
	for (int i = 0; i < len && *serialized; i++, serialized++) {}
	if (*serialized) *serialized++ = '\0';

	if (actions.count(name)) actions[name]->deserialize_state(serialized);
    }

    ret = true;
    save_dirty = false;
    nano_gettime(&last_save);

done:
    fatal_free(buf);
    file_close(f);
    return ret;
}

bool AnimationStation::save_state() {
    bool ret = false;
    file_t *f;
    if ((f = file_open(save_filename, "w")) == NULL) {
	consoles_printf("Failed to open %s\n", save_filename);
    } else {
	file_printf(f, "%d (version)\n", save_version);
	for (auto a : actions) {
	    file_printf(f, "%d %s %s\n", (int) a.first.length(), a.first.c_str(), a.second->serialize_state().c_str());
	}
	file_close(f);
	ret = true;
    }
    nano_gettime(&last_save);
    save_dirty = false;
    return ret;
}

bool AnimationStation::trigger_common() {
    assert(actions[active_prop]);

    bool ret = actions[active_prop]->act();

    lock->lock();

    save_dirty = true;
    active_prop = "";

    if (save_dirty && nano_elapsed_ms_now(&last_save) >= save_every_ms) {
	save_state();
    }

    lock->unlock();

    return ret;
}
    
void AnimationStation::main() {
    while (true) {
	lock->lock();
	while (! (active_prop == "" && triggered_prop != "")) cond->wait(lock);
	active_prop = triggered_prop;
	triggered_prop = "";
	lock->unlock();

	trigger_common();

    }
}

bool AnimationStation::trigger_async(std::string prop) {
    bool ret = false;

    if (actions.count(prop) && ! actions[prop]->is_disabled() && lock->trylock()) {
	if (active_prop == "" && triggered_prop == "") {
	    triggered_prop = prop;
	    actions[prop]->n_acts_async++;
	    cond->signal();
	    ret = true;
	}
        lock->unlock();
    }
    return ret;
}

bool AnimationStation::trigger(std::string prop) {
    bool ret = false;
    if (actions.count(prop) && ! actions[prop]->is_disabled() && lock->trylock()) {
	if (active_prop == "" && triggered_prop == "") {
	    active_prop = prop;
	    actions[prop]->n_acts++;
	    lock->unlock();
 	    ret = trigger_common();
	} else {
	    lock->unlock();
	    ret = false;
	}
    }
    return ret;
}

void AnimationStation::add(std::string name, AnimationStationAction *action) {
    lock->lock();
    actions[name] = action;
    lock->unlock();
}
