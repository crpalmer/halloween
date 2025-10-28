#ifndef __ANIMATION_UI_H__
#define __ANIMATION_UI_H__

#include <string>
#include "httpd-server.h"

class AnimationStationUI : public HttpdPrefixHandler {
public:
    AnimationStationUI(HttpdServer *httpd, std::string root = "/station") : root(root) {
	httpd->add_prefix_handler(root, this);
    }

    HttpdResponse *open(std::string path) override;

private:
    std::string root;

    HttpdResponse *trigger(std::string prop);
    std::string start_html();
    std::string try_to_trigger(std::string path);
    void add_props(std::string &html);
    void finish(std::string html);
    HttpdResponse *create_response(int status, std::string html);
    void header(std::string name, std::string &html);
};

#endif
