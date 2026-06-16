#include "srb2dbot/module.hpp"
#include "srb2dbot/utils.hpp"
#include "version.h"
#include <dpp/dpp.h>
#include <fstream>
#include <sstream>
#include <string>

class ShutdownCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "shutdown_card"; }
    auto description() const -> std::string_view override { return "Discord embed when the bot shuts down"; }

    explicit ShutdownCardModule(std::string msg, std::string srb2_dir)
        : msg_(std::move(msg)), srb2_dir_(std::move(srb2_dir)) {}

    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override {
        return {};
    }

    auto on_shutdown(dpp::cluster& bot, dpp::snowflake channel) -> void override {
        if (channel == 0) return;

        auto [server_name, map_name, uptime_str, players_str] = read_stats();

        std::string title = msg_.empty()
            ? ":red_circle: Bot is going down"
            : substitute_placeholders(msg_, {
                {"server_name", server_name},
                {"map", map_name},
                {"uptime", uptime_str},
                {"players", players_str}
            });

        dpp::embed embed;
        embed.set_title(title);
        embed.set_color(0xE74C3C);
        embed.set_timestamp(std::time(nullptr));

        if (!map_name.empty())
            embed.add_field("Map", map_name, true);
        if (!players_str.empty())
            embed.add_field("Players", players_str, true);
        if (!uptime_str.empty())
            embed.add_field("Server uptime", uptime_str, true);
        embed.add_field("Bot version", version_str(), true);

        bot.message_create(dpp::message(channel, "").add_embed(embed),
            [&bot](const dpp::confirmation_callback_t& cb) {
                if (cb.is_error()) std::cout << "[shutdown_card] message_create error: " << cb.get_error().human_readable << std::endl;
                std::cout << "[shutdown_card] shutdown message sent" << std::endl;
                bot.shutdown();
            });
    }

private:
    std::string msg_;
    std::string srb2_dir_;

    struct Stats {
        std::string server_name;
        std::string map_name;
        std::string uptime;
        std::string players;
    };

    auto read_stats() -> Stats {
        std::string path = srb2_dir_ + "/luafiles/client/DiscordBot/Stats.txt";
        std::ifstream file(path);
        if (!file.is_open()) return {};

        Stats stats;
        std::string line;
        int idx = 0;
        while (std::getline(file, line) && idx <= 8) {
            switch (idx) {
                case 0: stats.server_name = line; break;
                case 1: strip_map_name(line, stats.map_name); break;
                case 7: stats.uptime = line; break;
                case 8: stats.players = line; break;
            }
            ++idx;
        }
        return stats;
    }

    static void strip_map_name(const std::string& raw, std::string& out) {
        // Input:  "Lime Forest (MAPF0)"  or  "Greenflower Act 1 (1)"
        // Output: "Lime Forest"           or  "Greenflower Act 1"
        auto pos = raw.rfind(" (");
        if (pos != std::string::npos)
            out = raw.substr(0, pos);
        else
            out = raw;
    }

    static auto version_str() -> std::string {
        std::string v(PROJECT_VERSION_MAJOR "." PROJECT_VERSION_MINOR "." PROJECT_VERSION_PATCH);
        if (v == ".." || v.empty()) {
            std::string c(PROJECT_COMMIT);
            if (!c.empty()) return "git-" + c;
            return "unknown";
        }
        return v;
    }
};

auto create_shutdown_card_module(const std::string& msg, const std::string& srb2_dir) -> std::unique_ptr<Module> {
    return std::make_unique<ShutdownCardModule>(msg, srb2_dir);
}
