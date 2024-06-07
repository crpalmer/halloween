#include "pi.h"
#include "animation-station.h"
#include "animation-ui.h"

HttpdResponse *AnimationStationUI::open(std::string path) {
    int status = 200;

    std::string html = start_html();

    if (path != "") {
	if (try_to_trigger(path, html)) status = 302;
    }

    add_props(html);
    finish(html);

    auto response = new HttpdResponse(html);
    response->set_status(status);
    response->add_header("Location: " + root + "/\r\n");
    response->set_content_type("text/html");

    return response;
}

std::string AnimationStationUI::start_html() {
    auto station = AnimationStation::get();
    auto active_prop = station->get_active_prop();

    return "<html><header><title>Animation Station</title><meta http-equiv=\"refresh\" content=\"" + std::string(active_prop != "" ? "5" : "30") + "\"></header><body>";
}

bool AnimationStationUI::try_to_trigger(std::string prop, std::string &html) {
    AnimationStation *station = AnimationStation::get();

    bool ret = station->trigger_async(prop);
    if (ret) html += "<h2>Triggered: " + prop + " </h2>";
    else html += "<h2>Failed: " + prop + "</h2>";

    return ret;
}

void AnimationStationUI::add_props(std::string &html) {
    auto station = AnimationStation::get();
    auto active_prop = station->get_active_prop();

    html += "<table><tr><th>Prop</th><th>Status</th></tr>\n";
    for (auto action : station->get_actions()) {
	auto status = action.first == active_prop ? "active" : "ready";
	html += "<tr><td><a href=\"" + action.first + "\">" + action.first + "</a></td><td>" + status + "</td></tr>";
    }
    html += "</table>";
}

void AnimationStationUI::finish(std::string html) {
    html += "</body>";
}
