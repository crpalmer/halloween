#include <string.h>
#include "pi.h"
#include "time-utils.h"
#include "random-utils.h"
#include "animation-station.h"

AnimationStation::AnimationStation() {
    lock = new PiMutex();
}

void AnimationStation::add(AnimationStationAction *action) {
    actions.push_back(action);
}

bool AnimationStation::try_to_acquire() {
    return lock->trylock();
}

void AnimationStation::release() {
    lock->unlock();
}

char *AnimationStation::process_cmd(const char *cmd) { 
    for (AnimationStationAction *action : actions) {
	if (action->is_cmd(cmd)) {
	    return fatal_strdup(action->triggered() ? "ok" : "busy");
	}
    }
    return fatal_strdup("error invalid command");
}

bool AnimationStation::is_cmd(const char *cmd) {
    for (AnimationStationAction *action : actions) {
	if (action->is_cmd(cmd)) return true;
    }
    return false;
}

void AnimationStation::usage(Writer *w) {
    w->printf("Props to trigger:\n");
    for (AnimationStationAction *action : actions) {
	w->printf("   %s\n", action->get_cmd());
    }
}
 
AnimationStationAction::AnimationStationAction(AnimationStation *station, Input *button, const char *name) : PiThread(name), station(station), button(button) {
    button->set_pullup_down();
    button->set_debounce(1);
    button->set_notifier(this);
}

bool AnimationStationAction::triggered() {
    if  (! station->try_to_acquire()) return false;
    act();
    station->release();
    return true;
}

char *AnimationStationAction::run_cmd(int argc, char **argv, void *blob, size_t n_block) {
    if (! triggered()) return fatal_strdup("busy");
    else return fatal_strdup("ok");
}

void AnimationStationAction::main() {
    bool old_state = button->get();
    while (1) {
	pause();

	bool new_state = button->get();
	if (old_state != new_state && new_state) {
	    triggered();
	}
	old_state = button->get();	// It may have changed while we were running
    }
}

void AnimationStationAction::on_change() {
     resume_from_isr();
}

void AnimationStationPopper::act() {
    lights->off();
    light->set(true);
    act_stage2();
    light->set(false);
    lights->chase();
}

unsigned AnimationStationPopper::up_ms(double up) {
    return (500 + random_number_in_range(0, 250) - 125)*up;
}

unsigned AnimationStationPopper::down_ms(double down) {
    return (200 + random_number_in_range(0, 100) - 50)*down;
}

void AnimationStationPopper::attack_one(Output *output, double up, double down) {
    output->set(true);
    ms_sleep(up_ms(up));
    output->set(false);
}

void AnimationStationPopper::attack_without_audio(Output *output, double up, double down) {
    unsigned i;

    for (i = 0; i < 3; i++) {
	attack_one(output, up, down);
	ms_sleep(down_ms(down));
    }
}

void AnimationStationPopper::attack_with_audio(Output *output, AudioPlayer *audio_player, double up, double down) {
    random_audio->play_random(audio_player);
    while (audio_player->is_active()) attack_one(output, up, down);
}
    
void AnimationStationPopper::attack(Output *output, double up, double down, AudioPlayer *audio_player) {
    if (! audio_player || random_audio->is_empty()) attack_without_audio(output, up, down);
    else attack_with_audio(output, audio_player, up, down);
}
