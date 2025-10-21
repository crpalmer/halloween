#include "pi.h"
#include "gp-input.h"
#include "gp-output.h"
#include "pi-threads.h"
#include "stdin-reader.h"
#include "stdout-writer.h"
#include "stepper.h"
#include "threads-console.h"

static const int STEPPER_PIN0 = 0;
static const int ES_PIN = 2;

static const double STEPS_FOR_FULL_ROTATION = 360/1.8;
static const double MM_PER_FULL_ROTATION = 120;
static const double STEPS_PER_MM = STEPS_FOR_FULL_ROTATION / MM_PER_FULL_ROTATION;
static const double DISTANCE_MM = 1900;
static const double TIME_SEC = 3;

static const bool TEST_MODE = true;

class Mushroom : public PiThread {
public:
    Mushroom(int base_pin) : PiThread("mushroom") {
	stepper = new Stepper(base_pin, STEPS_PER_MM);
	start();
    }

    void main(void) {
	while (1) {
	    stepper->go(DISTANCE_MM-1, DISTANCE_MM/TIME_SEC);
	    stepper->go(1, DISTANCE_MM/TIME_SEC);
	}
    }

private:
    Stepper *stepper;
};

class ConsoleThread : public ThreadsConsole, public PiThread {
public:
    ConsoleThread() : ThreadsConsole(new StdinReader(), new StdoutWriter()) {
	stepper = new Stepper(STEPPER_PIN0, STEPS_PER_MM);
	endstop = new GPInput(ES_PIN);
	endstop->set_pullup_up();
	start();
    }

    void main() {
	Console::main();
    }

    void process_cmd(const char *cmd) override {
	const char *p = strchr(cmd, ' ');
	double arg;
	bool have_arg = (p && sscanf(p, "%lg", &arg) == 1);

        if (have_arg && is_command(cmd, "move-to")) {
	    stepper->go(arg, mm_sec);
	} else if (have_arg && is_command(cmd, "set-speed")) {
	    mm_sec = arg;
	} else if (is_command(cmd, "status")) {
	    printf("pos %g speed %g endstop %s\n", pos, mm_sec, endstop->get() ? "triggered" : "not");
        } else {
            ThreadsConsole::process_cmd(cmd);
        }
    }

    void usage() override {
        ThreadsConsole::usage();
	printf("move-to <mm>\nset-speed <mm/sec>\nstatus\n");
    }

private:
    Stepper *stepper = NULL;
    Input *endstop = NULL;

    double mm_sec = 200;
    double pos = 0;
};

void thread_main(int argc, char **argv) {
    ms_sleep(1000);
    printf("Starting\n");
    if (TEST_MODE) new ConsoleThread();
    else new Mushroom(STEPPER_PIN0);
}

int main(int argc, char **argv) {
    pi_init_with_threads(thread_main, argc, argv);
}
