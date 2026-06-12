#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include <dpp/dpp.h>
#include <ctime>
#include <optional>

class RoundEndCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "round_end_card"; }
    auto description() const -> std::string_view override { return "Discord embed with round results and player scores"; }
    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override { return {}; }
    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "ROUND_END") return std::nullopt;
        dpp::embed embed;
        std::string gt = event.fields.size() >= 1 ? event.fields[0] : "Round";
        std::string map_num = event.fields.size() >= 2 ? event.fields[1] : "?";
        std::string map_title;

        std::string red_team, blue_team, unaffiliated, spectators;
        std::string red_score, blue_score;
        bool has_teams = false;

        for (size_t i = 1; i < event.fields.size(); i++) {
            size_t colon = event.fields[i].find(':');
            if (colon == std::string::npos) continue;
            std::string key = event.fields[i].substr(0, colon);
            std::string val = event.fields[i].substr(colon + 1);

            if (key == "MAPNAME") { map_title = val;
            } else if (key == "TEAM" && val == "Red") { if (++i < event.fields.size()) red_score = event.fields[i]; has_teams = true;
            } else if (key == "TEAM" && val == "Blue") { if (++i < event.fields.size()) blue_score = event.fields[i]; has_teams = true;
            } else if (key == "RED") { size_t c2 = val.find(':'); if (c2 != std::string::npos) red_team += val.substr(0, c2) + "  " + val.substr(c2 + 1) + "\n";
            } else if (key == "BLUE") { size_t c2 = val.find(':'); if (c2 != std::string::npos) blue_team += val.substr(0, c2) + "  " + val.substr(c2 + 1) + "\n";
            } else if (key == "PLAYER") { size_t c2 = val.find(':'); if (c2 != std::string::npos) unaffiliated += val.substr(0, c2) + "  " + val.substr(c2 + 1) + "\n";
            } else if (key == "SPEC") { spectators += val + "\n"; }
        }

        embed.set_title(map_num + ": " + map_title + " - " + gt);
        if (has_teams) {
            embed.add_field("Red Team  " + red_score, red_team.empty() ? "_no players_" : red_team, true);
            embed.add_field("Blue Team  " + blue_score, blue_team.empty() ? "_no players_" : blue_team, true);
        }
        if (!unaffiliated.empty()) embed.add_field("Players", unaffiliated, false);
        if (!spectators.empty()) embed.set_footer(dpp::embed_footer().set_text("Spectators:\n" + spectators));
        embed.set_timestamp(std::time(nullptr));
        return embed;
    }
};

auto create_round_end_card_module() -> std::unique_ptr<Module> {
    return std::make_unique<RoundEndCardModule>();
}
