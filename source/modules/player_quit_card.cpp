#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include <dpp/dpp.h>
#include <optional>

class PlayerQuitCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "player_quit_card"; }
    auto description() const -> std::string_view override { return "Discord embed when a player leaves the game"; }
    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override { return {}; }
    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "PLAYER_QUIT") return std::nullopt;
        std::string player = event.fields.size() >= 1 ? event.fields[0] : "Someone";
        dpp::embed embed;
        embed.set_title("Player Left");
        embed.set_description(":door: **" + player + "** has left the game.");
        embed.set_color(0xED4245);
        return embed;
    }
};

auto create_player_quit_card_module() -> std::unique_ptr<Module> {
    return std::make_unique<PlayerQuitCardModule>();
}
