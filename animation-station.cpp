#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "server.h"
#include "time-utils.h"
#include "util.h"

#include "animation-station.h"

#define DEBOUNCE_MS	100

class DummyWaiting : public AnimationStationWaiting {
public:
    void fn(unsigned total_waiting_ms) {}
    unsigned get_period_ms() { return 100*1000*1000; }
};

void *
AnimationStation::main(void *as_as_vp)
{
    AnimationStation *as = (AnimationStation *) as_as_vp;
    struct timespec last_notify;

    nano_gettime(&last_notify);
    as->lights->chase();
    pthread_mutex_lock(&as->lock);

    while (true) {
	while (as->active_action == NULL) {
	    struct timespec now, wait_until;

	    wait_until = last_notify;
	    nano_add_ms(&wait_until, as->waiting->get_period_ms());
	    
	    pthread_cond_timedwait(&as->cond, &as->lock, &wait_until);

	    nano_gettime(&now);

	    if (as->active_action == NULL && nano_later_than(&now, &wait_until)) {
		as->waiting->fn(nano_elapsed_ms(&now, &as->start_waiting));
		nano_gettime(&last_notify);
	    }
	}

	as->lights->off();
	output_t *light = as->active_action->get_light();
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

    pthread_mutex_init(&lock, NULL);
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    pthread_cond_init(&cond, &attr);
}

void
AnimationStation::start()
{
    pthread_create(&thread, NULL, main, this);
}

void
AnimationStation::add_action(AnimationStationAction *action)
{
    actions.push_back(action);
    output_t *light = action->get_light();
    if (light) lights->add(light);
}

void
AnimationStation::check_inputs()
{
    for (std::list<AnimationStationAction *>::iterator it = actions.begin(); ! active_action && it != actions.end(); it++) {
	if ((*it)->is_triggered()) {
	    active_action = *it;
	    pthread_cond_signal(&cond);
	    pthread_mutex_unlock(&lock);
	}
    }
}

char *
AnimationStation::handle_remote_cmd(const char *cmd, bool &busy)
{
    for (std::list<AnimationStationAction *>::iterator it = actions.begin(); it != actions.end(); it++) {
	bool needs_exclusivity = (*it)->needs_exclusivity();
	if (needs_exclusivity) {
	    if (pthread_mutex_trylock(&lock) != 0) goto was_busy;
	    if (active_action) {
		pthread_mutex_unlock(&lock);
was_busy:
		busy = true;
		return NULL;
	    }
	}
	char *this_response = (*it)->handle_remote_cmd(cmd, lights);
	if (needs_exclusivity) pthread_mutex_unlock(&lock);
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
    if (busy) return strdup("prop is currently busy");
    else return strdup("Invalid command");
}
    
void
AnimationStationController::main()
{
    server_args_t server_args;

    server_args.port = port;
    server_args.command = remote_event;
    server_args.state = this;

    pthread_create(&thread, NULL, server_thread_main, &server_args);

    for (std::list<AnimationStation *>::iterator it = stations.begin(); it != stations.end(); it++) {
	(*it)->start();
    }

    while (true) {
	for (std::list<AnimationStation *>::iterator it = stations.begin(); it != stations.end(); it++) {
	    (*it)->check_inputs();
	}
    }
}
