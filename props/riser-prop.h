#ifndef __RISER_H__

#include "io.h"
#include "motor.h"

class RiserProp {
public:
    RiserProp(Motor *motor, Input *low_es, Input *high_es) : motor(motor), low_es(low_es), high_es(high_es) {}
    void test();
    void up();
    void down();

protected:
    virtual int maximum_up_ms() { return 20*1000; }
    virtual int maximum_down_ms() { return 20*1000; }

private:
    void test_es(const char *name, Input *es);
    void wait_for_es(Input *es, int max_ms);

    Motor *motor;
    Input *low_es;
    Input *high_es;
};

#endif
