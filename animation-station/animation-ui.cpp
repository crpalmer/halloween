#include "pi.h"
#include "animation-station.h"
#include "animation-ui.h"

HttpdResponse *AnimationStationUI::open(std::string path) {
    int slash = (int) path.find_first_of("/");
    std::string prop = path;
    std::string action = "status";

    if (slash >= 0) {
	prop = path.substr(0, slash);
	action = path.substr(slash+1);
    }
    
    if (action == "status" || action == "trigger") {
	return trigger(prop);
    } else {
	std::string text = "failed to " + action + " " + prop;
	auto station = AnimationStation::get();

        if (action == "disable" && station->disable(prop)) text = "disabled " + prop;
	else if (action == "enable" && station->enable(prop)) text = "enabled " + prop;

	return new HttpdResponse(text);
    }
}

HttpdResponse *AnimationStationUI::trigger(std::string prop) {
    int status = 200;
    std::string action = "trigger";

    std::string trigger_html;
    if (prop != "") {
	// If we trigger it, give it a little time to start
	trigger_html = try_to_trigger(prop);
    }

    std::string html = start_html();
    html += trigger_html;

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

    return "<html><header><title>Animation Station</title><meta http-equiv=\"refresh\" content=\"" + std::string(active_prop != "" ? "5" : "30") + "; URL='" + root.c_str() + "/'\"></header><body>";
}

std::string AnimationStationUI::try_to_trigger(std::string prop) {
    AnimationStation *station = AnimationStation::get();

    if (station->trigger_async(prop)) {
	return "<h2>Triggered: " + prop + " </h2>";
    } else {
	return "<h2>Failed: " + prop + "</h2>";
    }
}

void AnimationStationUI::add_props(std::string &html) {
    auto station = AnimationStation::get();
    auto active_prop = station->get_active_prop();

    html += "<table><tr><th>Prop</th><th>Status</th><th># Times</th></tr>\n";
    for (auto action : station->get_actions()) {
	auto name = action.first;
	auto a = action.second;
	std::string status;

	if (a->is_disabled()) status = "disabled";
	else if (name == active_prop) status = "ACTIVE";
	else status = "ready";

	html += "<tr><td><a href=\"" + action.first + "\">" + action.first + "</a></td><td>" + status + "</td><td align='right'>" + std::to_string(a->get_n_acts()) + "</td></tr>";
    }
    html += "</table>";
}

void AnimationStationUI::finish(std::string html) {
    html += "</body>";
}
