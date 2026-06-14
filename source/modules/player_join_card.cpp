#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include "srb2dbot/utils.hpp"
#include <dpp/dpp.h>
#include <optional>

class PlayerJoinCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "player_join_card"; }
    auto description() const -> std::string_view override { return "Discord embed when a player joins the game"; }
    explicit PlayerJoinCardModule(std::string msg) : msg_(std::move(msg)) {}
    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override { return {}; }
    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "PLAYER_JOIN") return std::nullopt;
        std::string player = event.fields.size() >= 1 ? event.fields[0] : "Someone";
        std::string node = event.fields.size() >= 2 ? event.fields[1] : "";
        dpp::embed embed;
        embed.set_title(msg_.empty() ? "Player Joined" : substitute_placeholders(msg_, {{"player", player}, {"node", node}}));
        embed.set_description(":bust_in_silhouette: **" + player + "** has joined the game.");
        embed.set_color(0x57F287);
        return embed;
    }
private:
    std::string msg_;
};

auto create_player_join_card_module(const std::string& msg) -> std::unique_ptr<Module> {
    return std::make_unique<PlayerJoinCardModule>(msg);
}
