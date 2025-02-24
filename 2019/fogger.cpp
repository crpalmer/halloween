#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "net-listener.h"
#include "net-reader.h"
#include "net-writer.h"
#include "pi-threads.h"
#include "wb.h"
#include "ween2019.h"
#include "fogger.h"

class FoggerNet : public NetListener {
public:
    FoggerNet(Fogger *fogger) : NetListener(5678), fogger(fogger) { start(); }
    void accepted(int fd) {
	new FoggerConsole(fogger, new NetReader(fd), new NetWriter(fd));
    }

private:
    Fogger *fogger;
};

static void threads_main(int argc, char **argv) {
#if WEEN_BOARD
    WeenBoard *wb = new WeenBoard();
    Output *output = wb->get_output(2, 1);
#else
    Output *output = new GPOutput(2);
#endif

    Fogger *fogger = new Fogger(output, 0.01, 0.01);
    new FoggerNet(fogger);
}

int
main(int argc, char **argv)
{
    pi_init_with_threads(threads_main, argc, argv);
}
