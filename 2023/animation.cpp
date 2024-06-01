#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "pi.h"
#include "animation-station.h"
#include "audio.h"
#include "audio-player.h"
#include "file.h"
#include "i2c.h"
#include "mcp23017.h"
#include "net-listener.h"
#include "stdin-reader.h"
#include "stdout-writer.h"
#include "random-utils.h"
#include "threads-console.h"
#include "wav.h"
#include "wb.h"
#include "wifi.h"

static Audio *audio;
static AudioPlayer *audio_player;

class Bunny : public AnimationStationPopper {
public:
    Bunny(AnimationStation *station, Lights *lights, Input *button, Output *light, Output *output) : AnimationStationPopper(station, lights, button, light, "bunny"), output(output) { start(); }

    void act_stage2() override {
	fprintf(stderr, "%s\n", get_cmd());
	if (! file_exists("disable-bunny")) attack(output, 1.0, 2.5);
    }

private:
    Output *output;
};

class Gater : public AnimationStationPopper {
public:
    Gater(AnimationStation *station, Lights *lights, Input *button, Output *light, Output *output) : AnimationStationPopper(station, lights, button, light, "gater"), output(output) { start(); }

    void act_stage2() override {
	fprintf(stderr, "%s\n", get_cmd());
	if (! file_exists("disable-gater")) attack(output, 2, 2);
    }

private:
    Output *output;
};

class Question : public AnimationStationPopper {
public:
    Question(AnimationStation *station, Lights *lights, Input *button, Output *light, Output *output) : AnimationStationPopper(station, lights, button, light, "question"), output(output) {
	add_wav("laugh.wav");
	start();
    }

    void act_stage2() override {
	fprintf(stderr, "%s\n", get_cmd());
	if (! file_exists("disable-question")) {
	    output->set(true);
	    lights->blink_all();
	    attack_with_audio(output, audio_player, 1.0, 2.5);
	    output->set(false);
	}
    }

    void attack_one(Output *output, double up, double down) override {}

private:
    Output *output;
};

class Pillar : public AnimationStationPopper {
public:
    Pillar(AnimationStation *station, Lights *lights, Input *button, Output *light, Output *output) : AnimationStationPopper(station, lights, button, light, "pillar"), output(output) { start(); }

    void act_stage2() override {
	fprintf(stderr, "%s\n", get_cmd());
	if (! file_exists("disable-pillar")) attack(output, 1, 0.75);
    }

private:
    Output *output;
};

class Snake : public AnimationStationPopper {
public:
    Snake(AnimationStation *station, Lights *lights, Input *button, Output *light, Output *output) : AnimationStationPopper(station, lights, button, light, "snake"), output(output) { start(); }

    void act_stage2() override {
	fprintf(stderr, "%s\n", get_cmd());
	if (! file_exists("disable-snake")) attack(output, 1.0, 3.0);
    }

private:
    Output *output;
};

class AnimationServerListener : public NetListener {
public:
    AnimationServerListener(AnimationStation *station) : NetListener(5555, "server"), station(station) { start(); }

    void accepted(int fd) override {
	PiThread *thread = new AnimationStationServerConnection(station, new NetReader(fd), new NetWriter(fd), "server-con");
	thread->start();
    }

private:
    AnimationStation *station;
};

    Lights *lights;
class AnimationConsole : public ThreadsConsole, public PiThread {
public:
    AnimationConsole(AnimationStation *station, Reader *r, Writer *w) : ThreadsConsole(r, w), PiThread("console"), station(station) { start(); }

    void process_cmd(const char *cmd) override {
	if (station->is_cmd(cmd)) {
	    if (! station->process_cmd(cmd)) printf("busy\n");
	    else printf("ok\n");
} else if (is_command(cmd, "mem")) {
for (int i = 0; i < 10000; i++) lights->chase();
	} else {
	    ThreadsConsole::process_cmd(cmd);
	}
    }

    void main() override { ThreadsConsole::main(); }

    void usage() override {
	ThreadsConsole::usage();
	station->usage(this);
    }

private:
    AnimationStation *station;
};
    

class AnimationConsoleListener : public NetListener {
public:
    AnimationConsoleListener(AnimationStation *station) : NetListener(4567, "net-consoles"), station(station) { start(); }
    void accepted(int fd) override {
	new AnimationConsole(station, new NetReader(fd), new NetWriter(fd));
    }

private:
    AnimationStation *station;
};

void threads_main(int argc, char **argv) {
    gpioInitialise();
    seed_random();
    wifi_init("animation");

#ifdef PLATFORM_pi
    audio = new AudioPi();
#else
    i2c_init_bus(1, 400*1000);
    i2c_config_gpios(2, 3);
    audio = new AudioPico();
    MCP23017 *mcp = new MCP23017();
#endif
    audio_player = new AudioPlayer(audio);

    printf("Initializing station\n");
    AnimationStation *station = new AnimationStation();

    printf("Starting console\n");
    new AnimationConsole(station, new StdinReader(), new StdoutWriter());

    lights = new Lights;
    lights->set_blink_ms(500);

#ifdef PLATFORM_pi
#if WEEN_BOARD
    WeenBoard *wb = new WeenBoard(1);
    station->add(new Bunny(station, lights, wb->get_input(1), wb->get_output(1, 1), wb->get_output(2, 1)));
    station->add(new Gater(station, lights, wb->get_input(2), wb->get_output(1, 2), wb->get_output(2, 2)));
    station->add(new Question(station, lights, wb->get_input(3), wb->get_output(1, 3), wb->get_output(2, 3)));
    station->add(new Pillar(station, lights, wb->get_input(4), wb->get_output(1, 4), wb->get_output(2, 4)));
    station->add(new Snake(station, lights, wb->get_input(5), wb->get_output(1, 5), wb->get_output(2, 5)));
#else
    /* This is just for testing on pi5, just make everything use a single output for lights & actions */
    GPOutput *output = new GPOutput(9);
    station->add(new Bunny(station, lights, new GPInput(4), output, output));
    station->add(new Gater(station, lights, new GPInput(5), output, output));
    station->add(new Question(station, lights, new GPInput(6), output, output));
    station->add(new Pillar(station, lights, new GPInput(7), output, output));
    station->add(new Snake(station, lights, new GPInput(8), output, output));
#endif
#else
    station->add(new Bunny(station, lights, new GPInput(4), mcp->get_output(0, 0), mcp->get_output(1, 0)));
    station->add(new Gater(station, lights, new GPInput(5), mcp->get_output(0, 1), mcp->get_output(1, 1)));
    station->add(new Question(station, lights, new GPInput(6), mcp->get_output(0, 2), mcp->get_output(1, 2)));
    station->add(new Pillar(station, lights, new GPInput(7), mcp->get_output(0, 3), mcp->get_output(1, 3)));
    station->add(new Snake(station, lights, new GPInput(8), mcp->get_output(0, 4), mcp->get_output(1, 4)));
#if 0
#endif
#endif
    printf("Starting servers\n");

    new AnimationConsoleListener(station);
    new AnimationServerListener(station);
}

int
main(int argc, char **argv)
{
    pi_init_with_threads(threads_main, argc, argv);
}
