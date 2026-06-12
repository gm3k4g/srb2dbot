#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include <dpp/dpp.h>
#include <optional>

class PlayerJoinCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "player_join_card"; }
    auto description() const -> std::string_view override { return "Discord embed when a player joins the game"; }
    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override { return {}; }
    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "PLAYER_JOIN") return std::nullopt;
        std::string player = event.fields.size() >= 1 ? event.fields[0] : "Someone";
        dpp::embed embed;
        embed.set_title("Player Joined");
        embed.set_description(":bust_in_silhouette: **" + player + "** has joined the game.");
        embed.set_color(0x57F287);
        return embed;
    }
};

auto create_player_join_card_module() -> std::unique_ptr<Module> {
    return std::make_unique<PlayerJoinCardModule>();
}
