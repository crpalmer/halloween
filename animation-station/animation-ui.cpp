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

    return "<html><header><title>Animation Station</title><meta http-equiv=\"refresh\" content=\"" + std::string(active_prop != "" ? "5" : "30") + "; URL='" + root.c_str() + "/'\"></header><body><link rel='stylesheet' href='ui.css'>";
}

std::string AnimationStationUI::try_to_trigger(std::string prop) {
    AnimationStation *station = AnimationStation::get();

    if (station->trigger_async(prop)) {
	return "<div class='triggered'`>Triggered: " + prop + " </div>";
    } else {
	return "<div class='trigger-failed'`>Failed: " + prop + "</div>";
    }
}

void AnimationStationUI::header(std::string name, std::string &html) {
    html += "<div class='header'><div class='header-" + name + "'></div></div>";
}

void AnimationStationUI::add_props(std::string &html) {
    auto station = AnimationStation::get();
    auto active_prop = station->get_active_prop();

    html += "<div class='status-table'><div class='header-row'>";
    header("prop", html);
    header("status", html);
    header("times-async", html);
    header("times", html);
    html += "</div>\n";

    for (auto action : station->get_actions()) {
	auto name = action.first;
	auto a = action.second;
	std::string status;

	if (a->is_disabled()) status = "disabled";
	else if (name == active_prop) status = "ACTIVE";
	else status = "ready";

	if (a->is_disabled()) {
	    html += "<div class='status-row-disabled'><div class='prop'>" + name + "</div>";
	} else {
	    html += "<div class='status-row'><div class='prop'><a href=\"" + name + "\">" + name + "</a></div>";
	}
	html += "<div class='status'><div class='status-" + status + "''></div></div>";
	html += "<div class='n-acts-async'>" + std::to_string(a->get_n_acts_async()) + "</div>";
	html += "<div class='n-acts'>" + std::to_string(a->get_n_acts()) + "</div>";
	html += "</div>";
    }
    html += "</div>";
}

void AnimationStationUI::finish(std::string html) {
    html += "</body>";
}
