#ifndef __ANIMATION_UI_H__
#define __ANIMATION_UI_H__

#include <string>
#include "httpd-server.h"

class AnimationStationUI : public HttpdPrefixHandler {
public:
    AnimationStationUI(std::string root = "/station") : root(root) {
	HttpdServer::get()->add_prefix_handler(root, this);
    }

    HttpdResponse *open(std::string path) override;

private:
    std::string root;

    std::string start_html();
    bool try_to_trigger(std::string path, std::string &html);
    void add_props(std::string &html);
    void finish(std::string html);
};

#endif