#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include <dpp/dpp.h>
#include <optional>

class RoundStartCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "round_start_card"; }
    auto description() const -> std::string_view override { return "Discord embed when a round starts on a new map"; }
    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override { return {}; }
    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "ROUND_START") return std::nullopt;
        dpp::embed embed;
        std::string gt = event.fields.size() >= 1 ? event.fields[0] : "Round";
        std::string map_num = event.fields.size() >= 2 ? event.fields[1] : "?";
        std::string map_title = event.fields.size() >= 3 ? event.fields[2] : "Unknown";
        embed.set_title(":map: Map is now " + map_num + ": " + map_title + " - " + gt);
        embed.set_color(0x57F287);
        return embed;
    }
};

auto create_round_start_card_module() -> std::unique_ptr<Module> {
    return std::make_unique<RoundStartCardModule>();
}
