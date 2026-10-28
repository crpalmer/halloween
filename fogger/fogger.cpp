#include "pi.h"
#include <string.h>
#include "net-listener.h"
#include "net-reader.h"
#include "net-writer.h"
#include "gp-output.h"
#include "wifi.h"

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
    wifi_init("fogger");

    Output *output = new GPOutput(0);

    ms_sleep(5000);
    printf("starting\n");
    Fogger *fogger = new Fogger(output);
    new FoggerNet(fogger);
    printf("finishing\n");
}

int
main(int argc, char **argv)
{
    pi_init_with_threads(threads_main, argc, argv);
}
