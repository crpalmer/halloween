#ifndef __ANIMATION_STATION_H__
#define __ANIMATION_STATION_H__

#include <pthread.h>
#include <list>
#include "io.h"
#include "server.h"

#include "lights.h"

class AnimationStationWaiting {
public:
    virtual void fn(unsigned total_wait_ms) = 0;
    virtual unsigned get_period_ms() = 0;
};

class AnimationStationAction {
public:
     virtual output_t *get_light()  { return NULL; }
     virtual bool is_triggered() { return false; }
     virtual char *handle_remote_cmd(const char *cmd) { return NULL; }
     virtual void act(Lights *lights) {};
     virtual bool needs_exclusivity() { return true; }
};

class AnimationStation {
friend class AnimationStationController;

public:
    AnimationStation(AnimationStationWaiting *waiting = NULL);
    void add_action(AnimationStationAction *action);
    void set_blink_ms(int blink_ms) { lights->set_blink_ms(blink_ms); }

protected:
    void check_inputs();
    char *handle_remote_cmd(const char *cmd, bool &busy);
    void start();

private:
    static void *main(void *this_as_vp);

    Lights *lights;

    AnimationStationWaiting *waiting;
    struct timespec start_waiting;

    std::list<AnimationStationAction *> actions;

    AnimationStationAction *active_action;

    pthread_t thread;
    pthread_mutex_t lock;
    pthread_cond_t cond;
};

class AnimationStationController {
public:
    AnimationStationController(unsigned port = 5555);
    void add_station(AnimationStation *station);
    void main();

private:
    static char *remote_event(void *this_as_vp, const char *cmd, struct sockaddr_in *addr, size_t size);
    std::list<AnimationStation *> stations;
    pthread_t thread;
    unsigned port;
};
    
#endif
