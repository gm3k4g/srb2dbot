#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include <dpp/dpp.h>
#include <optional>

class KickPlayerCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "kick_player_card"; }
    auto description() const -> std::string_view override { return "Discord embed when a player is kicked"; }
    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override { return {}; }
    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "KICK_PLAYER") return std::nullopt;
        std::string player = event.fields.size() >= 1 ? event.fields[0] : "Someone";
        dpp::embed embed;
        embed.set_title("Player Kicked");
        embed.set_description(":boot: **" + player + "** was kicked.");
        embed.set_color(0xED4245);
        return embed;
    }
};

auto create_kick_player_card_module() -> std::unique_ptr<Module> {
    return std::make_unique<KickPlayerCardModule>();
}
