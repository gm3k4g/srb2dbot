#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include "srb2dbot/server.hpp"
#include "srb2dbot/utils.hpp"
#include <dpp/dpp.h>
#include <ctime>
#include <fstream>
#include <optional>

class RoundStartCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "round_start_card"; }
    auto description() const -> std::string_view override { return "Discord embed when a round starts on a new map"; }

    explicit RoundStartCardModule(std::string msg, bool thumbs, std::string srb2_dir)
        : msg_(std::move(msg)), thumbs_enabled_(thumbs), srb2_dir_(std::move(srb2_dir)) {}

    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override { return {}; }

    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "ROUND_START") return std::nullopt;
        dpp::embed embed;
        std::string gt = event.fields.size() >= 1 ? event.fields[0] : "Round";
        std::string map_num = event.fields.size() >= 2 ? event.fields[1] : "?";
        std::string map_title = event.fields.size() >= 3 ? event.fields[2] : "Unknown";

        embed.set_title(msg_.empty()
            ? ":map: " + map_title
            : substitute_placeholders(msg_, {{"gametype", gt}, {"map_num", map_num}, {"map_title", map_title}}));
        embed.add_field("Map", map_num, true);
        embed.add_field("Gametype", gt, true);
        embed.add_field("Source", bridge_get_map_source(map_num), false);
        embed.set_color(0x57F287);
        embed.set_timestamp(std::time(nullptr));
        return embed;
    }

    auto get_bridge_attachment(const BridgeEvent& event) -> std::optional<std::pair<std::string, std::string>> override {
        if (!thumbs_enabled_) return std::nullopt;
        if (event.type != "ROUND_START") return std::nullopt;
        std::string map_name = event.fields.size() >= 2 ? event.fields[1] : "";
        if (map_name.empty()) return std::nullopt;

        std::string thumb_path = ensure_thumbnail(map_name);
        if (thumb_path.empty()) return std::nullopt;

        std::ifstream file(thumb_path, std::ios::binary);
        if (!file.is_open()) return std::nullopt;

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return std::make_pair(map_name + ".png", content);
    }

private:
    std::string msg_;
    bool thumbs_enabled_;
    std::string srb2_dir_;

    auto ensure_thumbnail(const std::string& map_name) -> std::string {
        std::string thumb_dir = srb2_dir_ + "/luafiles/client/DiscordBot/thumbnails";
        std::filesystem::create_directories(thumb_dir);
        std::string thumb_path = thumb_dir + "/" + map_name + ".png";

        if (access(thumb_path.c_str(), F_OK) != 0) {
            bridge_extract_thumbnail(map_name, thumb_dir);
        }

        if (access(thumb_path.c_str(), F_OK) == 0) {
            return thumb_path;
        }
        return "";
    }
};

auto create_round_start_card_module(const std::string& msg, bool thumbs, const std::string& srb2_dir) -> std::unique_ptr<Module> {
    return std::make_unique<RoundStartCardModule>(msg, thumbs, srb2_dir);
}
