#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pi.h"
#include "mem.h"
#include "pi-threads.h"
#include "server.h"
#include "time-utils.h"

#include "animation-station.h"

#define DEBOUNCE_MS	100

class DummyWaiting : public AnimationStationWaiting {
public:
    void fn(unsigned total_waiting_ms) {}
    unsigned get_period_ms() { return 100*1000*1000; }
};

void
AnimationStation::main(void *as_as_vp)
{
    AnimationStation *as = (AnimationStation *) as_as_vp;
    struct timespec last_notify;

    nano_gettime(&last_notify);
    as->lights->chase();
    pi_mutex_lock(as->lock);

    while (true) {
	while (as->active_action == NULL) {
	    struct timespec now, wait_until;

	    wait_until = last_notify;
	    nano_add_ms(&wait_until, as->waiting->get_period_ms());
	    
	    pi_cond_timedwait(as->cond, as->lock, &wait_until);

	    nano_gettime(&now);

	    if (as->active_action == NULL && nano_later_than(&now, &wait_until)) {
		as->waiting->fn(nano_elapsed_ms(&now, &as->start_waiting));
		nano_gettime(&last_notify);
	    }
	}

	as->lights->off();
	Output *light = as->active_action->get_light();
	if (light) light->on();

	as->active_action->act(as->lights);
	as->active_action = NULL;

	nano_gettime(&as->start_waiting);
	last_notify = as->start_waiting;
	as->lights->chase();
    }
}

AnimationStation::AnimationStation(AnimationStationWaiting *waiting) : active_action(NULL)
{
    this->lights = new Lights();
    this->waiting = waiting != NULL ? waiting : new DummyWaiting();
    nano_gettime(&start_waiting);

    lock = pi_mutex_new();
    cond = pi_cond_new();
}

void
AnimationStation::start()
{
    pi_thread_create("station", main, this);
}

void
AnimationStation::add_action(AnimationStationAction *action)
{
    actions.push_back(action);
    Output *light = action->get_light();
    if (light) lights->add(light);
}

void
AnimationStation::check_inputs()
{
    for (std::list<AnimationStationAction *>::iterator it = actions.begin(); ! active_action && it != actions.end(); it++) {
	if ((*it)->is_triggered()) {
	    active_action = *it;
	    pi_cond_signal(cond);
	    pi_mutex_unlock(lock);
	}
    }
}

char *
AnimationStation::handle_remote_cmd(const char *cmd, bool &busy)
{
    for (std::list<AnimationStationAction *>::iterator it = actions.begin(); it != actions.end(); it++) {
	bool needs_exclusivity = (*it)->needs_exclusivity();
	if (needs_exclusivity) {
	    if (pi_mutex_trylock(lock) != 0) goto was_busy;
	    if (active_action) {
		pi_mutex_unlock(lock);
was_busy:
		busy = true;
		return NULL;
	    }
	}
	char *this_response = (*it)->handle_remote_cmd(cmd, lights);
	if (needs_exclusivity) pi_mutex_unlock(lock);
	if (this_response != NULL) {
	    return this_response;
	}
    }
    return NULL;
}

AnimationStationController::AnimationStationController(unsigned port) : port(port)
{
}

void
AnimationStationController::add_station(AnimationStation *station)
{
    stations.push_back(station);
}

char *
AnimationStationController::remote_event(void *this_as_vp, const char *cmd, struct sockaddr_in *addr, size_t size)
{
    AnimationStationController *c = (AnimationStationController *) this_as_vp;
    bool busy = false;

    for (std::list<AnimationStation *>::iterator it = c->stations.begin(); it != c->stations.end(); it++) {
	char *response = (*it)->handle_remote_cmd(cmd, busy);
	if (response != NULL) return response;
    }
    if (busy) return fatal_strdup("prop is currently busy");
    else return fatal_strdup("Invalid command");
}
    
void
AnimationStationController::main()
{
    server_args_t server_args;

    server_args.port = port;
    server_args.command = remote_event;
    server_args.state = this;

    pi_thread_create("server", server_thread_main, &server_args);

    for (std::list<AnimationStation *>::iterator it = stations.begin(); it != stations.end(); it++) {
	(*it)->start();
    }

    while (true) {
	for (std::list<AnimationStation *>::iterator it = stations.begin(); it != stations.end(); it++) {
	    (*it)->check_inputs();
	}
    }
}
