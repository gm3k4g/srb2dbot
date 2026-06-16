#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include "srb2dbot/utils.hpp"
#include <dpp/dpp.h>
#include <ctime>
#include <fstream>
#include <optional>
#include <filesystem>
#include <cstdio>
#include <nlohmann/json.hpp>

class RoundEndCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "round_end_card"; }
    auto description() const -> std::string_view override { return "Discord embed when a round ends"; }

    explicit RoundEndCardModule(std::string msg, bool thumbs, std::string srb2_dir)
        : msg_(std::move(msg)), thumbs_enabled_(thumbs), srb2_dir_(std::move(srb2_dir)) {}

    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override {
        return {};
    }

    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "ROUND_END") return std::nullopt;
        if (event.fields.size() < 4) return std::nullopt;

        std::string gt        = event.fields[0];
        std::string map_num   = event.fields[1];
        std::string map_time  = event.fields[2];
        std::string game_time = event.fields.size() >= 4 ? event.fields[3] : "0";
        std::string total_pl  = event.fields.size() >= 5 ? event.fields[4] : "0";
        std::string red_pl    = event.fields.size() >= 6 ? event.fields[5] : "0";
        std::string blue_pl   = event.fields.size() >= 7 ? event.fields[6] : "0";
        std::string spec_pl   = event.fields.size() >= 8 ? event.fields[7] : "0";

        dpp::embed embed;
        embed.set_title("The round has ended.");
        embed.set_color(0xE74C3C);
        embed.set_timestamp(std::time(nullptr));

        embed.add_field("Map", map_num, true);
        embed.add_field("Map time", format_game_time(map_time), true);
        embed.add_field("Game time", format_game_time(game_time), true);

        std::string player_info = total_pl + " total";
        if (std::stoi(red_pl) > 0 || std::stoi(blue_pl) > 0) {
            player_info += " | :red_square: " + red_pl + " | :blue_square: " + blue_pl;
        }
        if (std::stoi(spec_pl) > 0) {
            player_info += " | Spectators: " + spec_pl;
        }
        embed.add_field("Players", player_info, true);

        return embed;
    }

    auto get_bridge_attachment(const BridgeEvent& event) -> std::optional<std::pair<std::string, std::string>> override {
        if (event.type != "ROUND_END") return std::nullopt;
        if (event.fields.size() < 2) return std::nullopt;

        std::string results_path = srb2_dir_ + "/luafiles/client/DiscordBot/RoundResults.json";
        std::ifstream rf(results_path);
        if (!rf.is_open()) return std::nullopt;

        nlohmann::json data;
        try { rf >> data; } catch (...) { return std::nullopt; }
        rf.close();

        std::string gametype = data.value("gametype", "FFA");
        std::string map_name = data.value("map", "");
        std::string title    = data.value("title", "");
        std::string round_time = data.value("round_time", "");
        std::string mode     = data.value("mode", "ffa");
        int blue_score = data.value("blue_score", 0);
        int red_score  = data.value("red_score", 0);

        if (map_name.empty()) return std::nullopt;

        std::string thumb_dir = srb2_dir_ + "/luafiles/client/DiscordBot/thumbnails";
        std::filesystem::create_directories(thumb_dir);
        std::string thumb_path = thumb_dir + "/" + map_name + ".png";
        bridge_extract_thumbnail(map_name, thumb_dir);
        bool has_thumb = (access(thumb_path.c_str(), F_OK) == 0);

        std::string players_json = data["players"].dump();
        std::string spec_json = data["spectators"].dump();

        // Find the intermission script — search common locations
        std::string script_path;
        std::vector<std::string> search_paths = {
            srb2_dir_ + "/generate_intermission.sh",
            srb2_dir_ + "/luafiles/client/DiscordBot/generate_intermission.sh",
            "/run/media/einfoed/EXTERNAL1/games/free/srb2/SRB2_TOOLS/srb2dbot/generate_intermission.sh",
        };
        for (auto& p : search_paths) {
            if (access(p.c_str(), X_OK) == 0) { script_path = p; break; }
        }
        if (script_path.empty()) {
            std::cout << "[round_end_card] generate_intermission.sh not found" << std::endl;
            return std::nullopt;
        }

        std::string out_path = thumb_dir + "/intermission_" + map_name + ".png";

        std::string cmd = "'" + script_path + "'"
            + " --gametype '" + mode + "'"
            + " --map '" + map_name + "'"
            + " --title '" + title + "'";
        if (!round_time.empty())
            cmd += " --round-time '" + round_time + "'";
        if (mode == "team") {
            cmd += " --blue-score " + std::to_string(blue_score);
            cmd += " --red-score " + std::to_string(red_score);
        }
        cmd += " --players '" + players_json + "'";
        if (spec_json != "[]")
            cmd += " --spectators '" + spec_json + "'";
        if (has_thumb)
            cmd += " --thumb '" + thumb_path + "'";
        cmd += " --out '" + out_path + "'";

        int ret = std::system(cmd.c_str());
        if (ret != 0) {
            std::cout << "[round_end_card] generate_intermission.sh failed (code " << ret << ")" << std::endl;
            return std::nullopt;
        }

        if (access(out_path.c_str(), F_OK) != 0) return std::nullopt;

        std::ifstream file(out_path, std::ios::binary);
        if (!file.is_open()) return std::nullopt;

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return std::make_pair("intermission_" + map_name + ".png", content);
    }

private:
    std::string msg_;
    bool thumbs_enabled_;
    std::string srb2_dir_;

    static auto format_game_time(const std::string& tics_str) -> std::string {
        std::int64_t tics = 0;
        try { tics = std::stoll(tics_str); } catch (...) { return "0s"; }
        if (tics < 0) return "0s";
        auto seconds = tics / 35;
        if (seconds < 60) return std::to_string(seconds) + "s";
        auto mins = seconds / 60;
        auto secs = seconds % 60;
        if (mins < 60) return std::to_string(mins) + "m " + std::to_string(secs) + "s";
        auto hrs = mins / 60;
        mins = mins % 60;
        return std::to_string(hrs) + "h " + std::to_string(mins) + "m";
    }
};

auto create_round_end_card_module(const std::string& msg, bool thumbs, const std::string& srb2_dir) -> std::unique_ptr<Module> {
    return std::make_unique<RoundEndCardModule>(msg, thumbs, srb2_dir);
}
