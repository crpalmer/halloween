#include "pi.h"
#include "gp-input.h"
#include "gp-output.h"
#include "pi-threads.h"
#include "stdin-reader.h"
#include "stdout-writer.h"
#include "stepper.h"
#include "threads-console.h"

static const int DIR_PIN = 0;
static const int STEP_PIN = 1;
static const int ES_PIN = 2;
static const int FAST_PIN = 14;
static const int REVERSE_PIN = 15;

static const int MICROSTEPPING = 32;
static const double STEPS_FOR_FULL_ROTATION = 200*MICROSTEPPING;
static const int PULLEY_TEETH = 60;
static const double MM_PER_FULL_ROTATION = 2 * PULLEY_TEETH;
static const double STEPS_PER_MM = STEPS_FOR_FULL_ROTATION / MM_PER_FULL_ROTATION;

static const double MM_PER_SEC = 200;
static const int LOW_MM = 50;
static const int HIGH_MM = 1840;
static const int HIGH_HOME = 1890;

static const int PAUSE_MS = 100;
static const bool TEST_MODE = false;

class Mushroom : public PiThread {
public:
    Mushroom() : PiThread("mushroom") {
	//Input *reverse = new GPInput(REVERSE_PIN);
	//reverse->set_pullup_up();
	dir = new GPOutput(DIR_PIN);
	//if (reverse->get()) 
	dir->set_is_inverted();

	Input *fast = new GPInput(FAST_PIN);
	fast->set_pullup_up();
	step = new GPOutput(STEP_PIN);
	stepper = new Stepper(dir, step, STEPS_PER_MM / (fast->get() ? 2 : 1));

	end_stop = new GPInput(ES_PIN);
	end_stop->set_pullup_up();
	start();
    }

    void main(void) {
	ms_sleep(1000);
	printf("Homing\n");
	home();
	printf("pos %g speed %g end_stop %s\n", stepper->get_pos(), MM_PER_SEC, end_stop->get() ? "triggered" : "not");
	fflush(stdout);
	ms_sleep(PAUSE_MS);
	while (1) {
	    printf("Move to high end\n");
	    stepper->go(HIGH_MM, MM_PER_SEC);
	    ms_sleep(PAUSE_MS);
	    printf("Move to low  end\n");
	    stepper->go(LOW_MM, MM_PER_SEC);
	    ms_sleep(PAUSE_MS);
	}
    }

protected:
    void home() {
	stepper->home(end_stop, true, HIGH_HOME, 100);
    }

protected:
    Output *dir;
    Output *step;
    Stepper *stepper;
    Input *end_stop = NULL;
};

class ConsoleMushroom : public ThreadsConsole, public Mushroom {
public:
    ConsoleMushroom() : ThreadsConsole(new StdinReader(), new StdoutWriter()) {
    }

    void main() override {
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
	} else if (have_arg && is_command(cmd, "set-accel")) {
	    stepper->set_acceleration(arg);
	} else if (have_arg && is_command(cmd, "set-jerk")) {
	    stepper->set_jerk(arg);
	} else if (is_command(cmd, "status")) {
	    printf("pos %g speed %g end_stop %s\n", pos, mm_sec, end_stop->get() ? "triggered" : "not");
	} else if (is_command(cmd, "home")) {
	    home();
        } else {
            ThreadsConsole::process_cmd(cmd);
        }
    }

    void usage() override {
        ThreadsConsole::usage();
	printf("home\nmove-to <mm>\nset-speed <mm/sec>\nset-accel <mm_per_s2>\nset-jerk <mm_per_s>\nstatus\n");
    }

private:
    double mm_sec = 200;
    double pos = 0;
};

void thread_main(int argc, char **argv) {
    ms_sleep(1000);		// Make sure that the stepper is really running
    printf("Starting\n");
    if (TEST_MODE) new ConsoleMushroom();
    else new Mushroom();
}

int main(int argc, char **argv) {
    pi_init_with_threads(thread_main, argc, argv);
}
