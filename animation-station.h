#ifndef __ANIMATION_STATION_H__
#define __ANIMATION_STATION_H__

#include <list>
#include "audio-player.h"
#include "lights.h"
#include "io.h"
#include "random-audio.h"
#include "server.h"
#include "writer.h"

class AnimationStationAction;

class AnimationStation {
public:
    AnimationStation();

    void add(AnimationStationAction *action);

    bool try_to_acquire();
    void release();

    char *process_cmd(const char *cmd);
    bool is_cmd(const char *cmd);
    void usage(Writer *w);

private:
    PiMutex *lock;
    std::list<AnimationStationAction *> actions;
};

class AnimationStationServerConnection : public ServerConnection {
public:
    AnimationStationServerConnection(AnimationStation *station, NetReader *r, NetWriter *w, const char *name = "as-client") : ServerConnection(r, w, name), station(station) { }
    char *process_cmd(const char *cmd) override { return station->process_cmd(cmd); }
	
private:
    AnimationStation *station;
};

class AnimationStationAction : public PiThread{
public:
    /* Note: You must call start() when you are ready for it to start */
    AnimationStationAction(AnimationStation *station, Input *button, const char *name = "as-action");

    char *run_cmd(int argc, char **argv, void *blob, size_t n_block);
    virtual bool triggered();

    virtual void act() = 0;
    virtual bool is_cmd(const char *cmd) = 0;
    virtual const char *get_cmd() { return NULL; }

    void main() override;

private:
    AnimationStation *station;
    Input *button;
};

class AnimationStationPopper : public AnimationStationAction {
public:
    AnimationStationPopper(AnimationStation *station, Lights *lights, Input *button, Output *light, const char *remote_cmd = NULL) : AnimationStationAction(station, button, remote_cmd ? remote_cmd : "as-popper"), lights(lights), light(light), remote_cmd(remote_cmd) { 
	random_audio = new RandomAudio();
    }

    ~AnimationStationPopper() {
	delete random_audio;
    }

    void act() override;
    virtual void act_stage2() = 0;

    void add_wav(const char *wav) {
	random_audio->add(wav);
    }

    bool is_cmd(const char *cmd) override { return strcmp(cmd, remote_cmd) == 0; }
    const char *get_cmd() override { return remote_cmd; }

protected:
    void attack(Output *output, double up, double down, AudioPlayer *audio_player = NULL);
    virtual void attack_one(Output *output, double up, double down);

    Lights *lights;
    Output *light;

    unsigned up_ms(double up);
    unsigned down_ms(double down);
    void attack_without_audio(Output *output, double up, double down);
    void attack_with_audio(Output *output, AudioPlayer *audio_player, double up, double down);

private:
    const char *remote_cmd;
    RandomAudio *random_audio;
};

#endif
