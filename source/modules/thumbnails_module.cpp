#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include "srb2dbot/server.hpp"
#include <dpp/dpp.h>

class MapThumbnailsModule : public Module {
public:
    auto name() const -> std::string_view override { return "map_thumbnails"; }
    auto description() const -> std::string_view override { return "Auto-extract and attach map thumbnails from PK3 files"; }

    explicit MapThumbnailsModule(std::string srb2_dir) : srb2_dir_(std::move(srb2_dir)) {}

    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override {
        return {};
    }

    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        // Thumbnails don't produce their own embeds — they modify existing ones.
        // Instead they work via on_timer_tick or attach_thumb callbacks.
        return std::nullopt;
    }

    // Check if a thumbnail exists; if not, try to extract it
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

private:
    std::string srb2_dir_;
};

auto create_thumbnails_module(const std::string& srb2_dir) -> std::unique_ptr<Module> {
    return std::make_unique<MapThumbnailsModule>(srb2_dir);
}
