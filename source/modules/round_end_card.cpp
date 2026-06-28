#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include "srb2dbot/utils.hpp"
#include <dpp/dpp.h>
#include <ctime>
#include <fstream>
#include <optional>
#include <filesystem>
#include <cstdio>

class RoundEndCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "round_end_card"; }
    auto description() const -> std::string_view override { return "Discord embed when a round ends"; }

    explicit RoundEndCardModule(std::string msg, bool thumbs, std::string srb2_dir, std::string bot_dir)
        : msg_(std::move(msg)), thumbs_enabled_(thumbs), srb2_dir_(std::move(srb2_dir)), bot_dir_(std::move(bot_dir)) {}

    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override {
        return {};
    }

    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "ROUND_END") return std::nullopt;
        if (event.fields.size() < 4) return std::nullopt;

        std::string gt        = event.fields[0];
        std::string map_num   = event.fields[1];
        std::string map_time  = event.fields[2];
        std::string server_time = event.fields.size() >= 4 ? event.fields[3] : "0";
        std::string total_pl  = event.fields.size() >= 5 ? event.fields[4] : "0";
        std::string red_pl    = event.fields.size() >= 6 ? event.fields[5] : "0";
        std::string blue_pl   = event.fields.size() >= 7 ? event.fields[6] : "0";
        std::string spec_pl   = event.fields.size() >= 8 ? event.fields[7] : "0";

        dpp::embed embed;
        embed.set_title("The round has ended.");
        embed.set_color(0xE74C3C);
        embed.set_timestamp(std::time(nullptr));

        embed.add_field("Map", map_num, true);
        embed.add_field("Map time", format_server_time(map_time), true);
        embed.add_field("Server time", format_server_time(server_time), true);

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
        if (event.fields.size() < 15) return std::nullopt;

        auto& f = event.fields;
        std::string gametype_name = f[0];
        std::string map_name      = f[1];
        std::string mode         = f[8];
        std::string red_score    = f[9];
        std::string blue_score   = f[10];
        std::string round_time   = f[11];
        std::string map_title    = unescape_pipe(f[12]);
        std::string players_json = unescape_pipe(f[13]);
        std::string spec_json    = f.size() >= 15 ? unescape_pipe(f[14]) : "[]";
        std::string point_limit  = f.size() >= 16 ? f[15] : "0";

        if (map_name.empty()) return std::nullopt;

        std::string thumb_dir = bot_dir_ + "/thumbnails";
        std::filesystem::create_directories(thumb_dir);
        std::string thumb_path = thumb_dir + "/" + map_name + ".png";
        bridge_extract_thumbnail(map_name, thumb_dir);
        if (access(thumb_path.c_str(), F_OK) != 0) {
            // Thumbnail not found — invalidate PK3 cache and retry
            bridge_invalidate_pk3_cache();
            bridge_extract_thumbnail(map_name, thumb_dir);
        }
        bool has_thumb = (access(thumb_path.c_str(), F_OK) == 0);

        // Find the intermission script
        std::string script_path;
        std::vector<std::string> search_paths = {
            bot_dir_ + "/generate_intermission.sh",
            bot_dir_ + "/../generate_intermission.sh",
            srb2_dir_ + "/luafiles/client/DiscordBot/generate_intermission.sh",
        };
        for (auto& p : search_paths) {
            if (access(p.c_str(), X_OK) == 0) { script_path = p; break; }
        }
        if (script_path.empty()) {
            std::cout << "[round_end_card] generate_intermission.sh not found" << std::endl;
            return std::nullopt;
        }

        // Write player data to temp files to avoid shell quoting issues
        std::string pf = thumb_dir + "/_players_" + map_name + ".json";
        std::string sf = thumb_dir + "/_specs_" + map_name + ".json";
        {
            std::ofstream pf_out(pf); pf_out << players_json;
            std::ofstream sf_out(sf); sf_out << spec_json;
        }

        std::string out_path = thumb_dir + "/intermission_" + map_name + ".png";

        // Build command without any single-quote wrapping (use --*-file instead)
        std::vector<std::string> args = {
            script_path,
            "--gametype", mode,
            "--gametype-name", gametype_name,
            "--map", map_name,
        };
        if (!round_time.empty()) {
            args.push_back("--round-time"); args.push_back(round_time);
        }
        if (point_limit != "0") {
            args.push_back("--point-limit"); args.push_back(point_limit);
        }
        if (mode == "team") {
            args.push_back("--blue-score"); args.push_back(blue_score);
            args.push_back("--red-score"); args.push_back(red_score);
        }
        args.push_back("--players-file"); args.push_back(pf);
        if (spec_json != "[]") {
            args.push_back("--spectators-file"); args.push_back(sf);
        }
        if (has_thumb) {
            args.push_back("--thumb"); args.push_back(thumb_path);
        }
        args.push_back("--title"); args.push_back(map_title);
        args.push_back("--out"); args.push_back(out_path);

        std::string cmd;
        for (auto& a : args) {
            cmd += "'" + a + "'";
            cmd.push_back(' ');
        }

        int ret = std::system(cmd.c_str());
        if (ret != 0) {
            std::cout << "[round_end_card] generate_intermission.sh failed (code " << ret << ")" << std::endl;
            return std::nullopt;
        }

        if (access(out_path.c_str(), F_OK) != 0) return std::nullopt;

        std::ifstream file(out_path, std::ios::binary);
        if (!file.is_open()) return std::nullopt;

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        // Clean up temp files
        std::remove(pf.c_str());
        std::remove(sf.c_str());

        return std::make_pair("intermission_" + map_name + ".png", content);
    }

private:
    std::string msg_;
    bool thumbs_enabled_;
    std::string srb2_dir_;
    std::string bot_dir_;

    static auto unescape_pipe(const std::string& s) -> std::string {
        std::string r = s;
        for (auto pos = r.find("\\x7c"); pos != std::string::npos; pos = r.find("\\x7c", pos + 1))
            r.replace(pos, 4, "|");
        for (auto pos = r.find("\\n"); pos != std::string::npos; pos = r.find("\\n", pos + 1))
            r.replace(pos, 2, "\n");
        return r;
    }

    static auto format_server_time(const std::string& tics_str) -> std::string {
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

auto create_round_end_card_module(const std::string& msg, bool thumbs, const std::string& srb2_dir, const std::string& bot_dir) -> std::unique_ptr<Module> {
    return std::make_unique<RoundEndCardModule>(msg, thumbs, srb2_dir, bot_dir);
}
