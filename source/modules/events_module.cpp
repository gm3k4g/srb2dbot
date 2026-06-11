#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include "srb2dbot/server.hpp"
#include <dpp/dpp.h>
#include <ctime>
#include <optional>

class GameEventsModule : public Module {
public:
    auto name() const -> std::string_view override { return "game_events"; }
    auto description() const -> std::string_view override { return "Display SRB2 game events as Discord embeds"; }

    GameEventsModule() = default;

    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override {
        return {};
    }

    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        dpp::embed embed;

        if (event.type == "SERVER_START") {
            embed.set_title(":green_circle: The server has started");
            embed.set_color(0x57F287);
            return embed;
        }

        if (event.type == "ROUND_START") {
            std::string gt = event.fields.size() >= 1 ? event.fields[0] : "Round";
            std::string map_num = event.fields.size() >= 2 ? event.fields[1] : "?";
            std::string map_title = event.fields.size() >= 3 ? event.fields[2] : "Unknown";
            embed.set_title(":map: Map is now " + map_num + ": " + map_title + " - " + gt);
            embed.set_color(0x57F287);
            return embed;
        }

        if (event.type == "ROUND_END") {
            std::string gt = event.fields.size() >= 1 ? event.fields[0] : "Round";
            std::string map_num = event.fields.size() >= 2 ? event.fields[1] : "?";
            std::string map_title = "";
            embed.set_title(map_num + ": " + map_title + " - " + gt);
            embed.set_color(0x57F287);

            std::string red_team, blue_team, unaffiliated, spectators;
            std::string red_score, blue_score;
            bool has_teams = false;

            for (size_t i = 1; i < event.fields.size(); i++) {
                size_t colon = event.fields[i].find(':');
                if (colon == std::string::npos) continue;
                std::string key = event.fields[i].substr(0, colon);
                std::string val = event.fields[i].substr(colon + 1);

                if (key == "MAPNAME") {
                    map_title = val;
                } else if (key == "TEAM" && val == "Red") {
                    if (++i < event.fields.size()) red_score = event.fields[i];
                    has_teams = true;
                } else if (key == "TEAM" && val == "Blue") {
                    if (++i < event.fields.size()) blue_score = event.fields[i];
                    has_teams = true;
                } else if (key == "RED") {
                    size_t c2 = val.find(':');
                    if (c2 != std::string::npos)
                        red_team += val.substr(0, c2) + "  " + val.substr(c2 + 1) + "\n";
                } else if (key == "BLUE") {
                    size_t c2 = val.find(':');
                    if (c2 != std::string::npos)
                        blue_team += val.substr(0, c2) + "  " + val.substr(c2 + 1) + "\n";
                } else if (key == "PLAYER") {
                    size_t c2 = val.find(':');
                    if (c2 != std::string::npos)
                        unaffiliated += val.substr(0, c2) + "  " + val.substr(c2 + 1) + "\n";
                } else if (key == "SPEC") {
                    spectators += val + "\n";
                }
            }

            embed.set_title(map_num + ": " + map_title + " - " + gt);
            if (has_teams) {
                embed.add_field("Red Team  " + red_score, red_team.empty() ? "_no players_" : red_team, true);
                embed.add_field("Blue Team  " + blue_score, blue_team.empty() ? "_no players_" : blue_team, true);
            }
            if (!unaffiliated.empty()) {
                embed.add_field("Players", unaffiliated, false);
            }
            if (!spectators.empty()) {
                embed.set_footer(dpp::embed_footer().set_text("Spectators:\n" + spectators));
            }
            embed.set_timestamp(std::time(nullptr));
            return embed;
        }

        if (event.type == "CTF_CAPTURE") {
            std::string player = event.fields.size() >= 1 ? event.fields[0] : "Someone";
            std::string team = event.fields.size() >= 2 ? event.fields[1] : "";
            embed.set_title("Flag Captured!");
            embed.set_description("**" + player + "** has captured the **" + team + "** flag!");
            embed.set_color(team == "Blue" ? 0x5865F2 : 0xED4245);
            return embed;
        }

        if (event.type == "CTF_DROP") {
            std::string player = event.fields.size() >= 1 ? event.fields[0] : "Someone";
            embed.set_title("Flag Dropped");
            embed.set_description("**" + player + "** dropped the flag.");
            embed.set_color(0xED4245);
            return embed;
        }

        if (event.type == "CTF_PICKUP") {
            std::string player = event.fields.size() >= 1 ? event.fields[0] : "Someone";
            embed.set_title("Flag Taken");
            embed.set_description("**" + player + "** picked up the flag.");
            embed.set_color(0xFEE75C);
            return embed;
        }

        if (event.type == "CTF_RETURN") {
            std::string player = event.fields.size() >= 1 ? event.fields[0] : "Server";
            embed.set_title("Flag Returned");
            embed.set_description("The flag was returned by **" + player + "**.");
            embed.set_color(0x747F8D);
            return embed;
        }

        if (event.type == "PLAYER_JOIN") {
            std::string player = event.fields.size() >= 1 ? event.fields[0] : "Someone";
            embed.set_title("Player Joined");
            embed.set_description(":bust_in_silhouette: **" + player + "** has joined the game.");
            embed.set_color(0x57F287);
            return embed;
        }

        if (event.type == "PLAYER_QUIT") {
            std::string player = event.fields.size() >= 1 ? event.fields[0] : "Someone";
            embed.set_title("Player Left");
            embed.set_description(":door: **" + player + "** has left the game.");
            embed.set_color(0xED4245);
            return embed;
        }

        if (event.type == "KICK_PLAYER") {
            std::string player = event.fields.size() >= 1 ? event.fields[0] : "Someone";
            embed.set_title("Player Kicked");
            embed.set_description(":boot: **" + player + "** was kicked.");
            embed.set_color(0xED4245);
            return embed;
        }

        if (event.type == "BAN_PLAYER") {
            std::string player = event.fields.size() >= 1 ? event.fields[0] : "Someone";
            embed.set_title("Player Banned");
            embed.set_description(":hammer: **" + player + "** was banned.");
            embed.set_color(0xED4245);
            return embed;
        }

        return std::nullopt;
    }
};

auto create_events_module() -> std::unique_ptr<Module> {
    return std::make_unique<GameEventsModule>();
}
