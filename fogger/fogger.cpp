#include "pi.h"
#include <string.h>
#include "httpd-server.h"
#include "gp-output.h"
#include "wifi.h"

#include "fogger.h"

class WWWHandler : public HttpdPrefixHandler {
public:
    WWWHandler(Fogger *fogger) {
	this->fogger = fogger;
    }

    HttpdResponse *open(std::string path, HttpdRequest *request) override {
	if (path == "more" || path == "less") {
	    if (path == "more") fogger->inc_duty();
	    else fogger->dec_duty();
	    HttpdResponse *response = new HttpdResponse("success");
	    response->set_status(302);
	    response->add_header("Location: /");
	    return response;
	}

	HttpdResponse *response = new HttpdResponse(
"<html>"
"<head><title>Fogger</title></head>"
"<body>"
  "<h2>Fogger</h2>"
  "<h3>Current duty is <b>" + std::to_string(fogger->get_duty()) + "</b> - <a href='/more'>[more fog]</a> || <a href='/less'>[less fog]</a></h3>"
"</body>"
	);
	response->set_content_type("text/html");
	return response;
    }

private:
    Fogger *fogger;
};

static void create_httpd_server(Fogger *fogger) {
    HttpdServer *www = new HttpdServer();
    www->add_prefix_handler("", new WWWHandler(fogger));
    www->start();
}

static void threads_main(int argc, char **argv) {
    wifi_init("fogger");

    Output *output = new GPOutput(0);
    create_httpd_server(new Fogger(output));
}

int
main(int argc, char **argv)
{
    pi_init_with_threads(threads_main, argc, argv);
}
