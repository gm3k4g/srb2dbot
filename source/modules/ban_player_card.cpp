#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include <dpp/dpp.h>
#include <optional>

class BanPlayerCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "ban_player_card"; }
    auto description() const -> std::string_view override { return "Discord embed when a player is banned"; }
    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override { return {}; }
    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "BAN_PLAYER") return std::nullopt;
        std::string player = event.fields.size() >= 1 ? event.fields[0] : "Someone";
        dpp::embed embed;
        embed.set_title("Player Banned");
        embed.set_description(":hammer: **" + player + "** was banned.");
        embed.set_color(0xED4245);
        return embed;
    }
};

auto create_ban_player_card_module() -> std::unique_ptr<Module> {
    return std::make_unique<BanPlayerCardModule>();
}
