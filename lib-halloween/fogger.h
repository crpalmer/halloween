#ifndef __FOGGER_H__
#define __FOGGER_H__

#include "io.h"
#include "pi-threads.h"
#include "threads-console.h"

class Fogger : public PiThread {
public:
    Fogger(Output *fogger, double default_duty = 0.025, double delta_duty = 0.025) : PiThread("fogger"), fogger(fogger), default_duty(default_duty), delta_duty(delta_duty) {
	lock = new PiMutex();
	load_duty();
	start();
    }

    virtual bool is_active() { return true; }

    bool fog(unsigned ms, bool non_blocking = false);

    void main(void) override;

    friend class FoggerConsole;

    double get_duty() { return duty; }

    double inc_duty(double delta = 0) {
	if (delta == 0) delta = delta_duty;
	duty += delta;
	if (duty < 0) duty = 0;
	return duty;
    }

    double dec_duty() {
	duty -= delta_duty;
	if (duty < 0) duty = 0;
	return duty;
    }

protected:
    Output *fogger;

    double default_duty;
    double delta_duty;
    double duty;

    void save_duty();

private:
    PiMutex *lock;

    void load_duty();
};

class FoggerConsole : public ThreadsConsole {
public:
    FoggerConsole(Fogger *fogger, Reader *r, Writer *w) : ThreadsConsole(r, w), fogger(fogger) { }
    void process_cmd(const char *cmd) override;
    void usage(void) override;

private:
    Fogger *fogger;
};

#endif
