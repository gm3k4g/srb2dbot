#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include "srb2dbot/server.hpp"
#include <dpp/dpp.h>
#include <fstream>

class MapThumbnailsModule : public Module {
public:
    auto name() const -> std::string_view override { return "map_thumbnails_card"; }
    auto description() const -> std::string_view override { return "Auto-extract and attach map thumbnails from PK3 files"; }

    explicit MapThumbnailsModule(std::string srb2_dir) : srb2_dir_(std::move(srb2_dir)) {}

    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override {
        return {};
    }

    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        return std::nullopt;
    }

    auto get_bridge_attachment(const BridgeEvent& event) -> std::optional<std::pair<std::string, std::string>> override {
        std::string map_name = map_name_from_event(event);
        if (map_name.empty()) return std::nullopt;

        std::string thumb_path = ensure_thumbnail(map_name);
        if (thumb_path.empty()) return std::nullopt;

        std::ifstream file(thumb_path, std::ios::binary);
        if (!file.is_open()) return std::nullopt;

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return std::make_pair(map_name + ".png", content);
    }

private:
    std::string srb2_dir_;

    static auto map_name_from_event(const BridgeEvent& event) -> std::string {
        if (event.type == "ROUND_START" && event.fields.size() >= 2) {
            return event.fields[1];
        }
        if (event.type == "ROUND_END" && event.fields.size() >= 2) {
            return event.fields[1];
        }
        return "";
    }

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

auto create_thumbnails_module(const std::string& srb2_dir) -> std::unique_ptr<Module> {
    return std::make_unique<MapThumbnailsModule>(srb2_dir);
}
