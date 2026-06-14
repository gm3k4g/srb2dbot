#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include "srb2dbot/utils.hpp"
#include <dpp/dpp.h>
#include <ctime>
#include <optional>

class PlayerQuitCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "player_quit_card"; }
    auto description() const -> std::string_view override { return "Discord embed when a player leaves the game"; }
    explicit PlayerQuitCardModule(std::string msg) : msg_(std::move(msg)) {}
    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override { return {}; }
    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "PLAYER_QUIT") return std::nullopt;
        std::string player = event.fields.size() >= 1 ? event.fields[0] : "Someone";
        std::string node = event.fields.size() >= 2 ? event.fields[1] : "";
        std::string reason = event.fields.size() >= 3 ? event.fields[2] : "";
        dpp::embed embed;
        embed.set_title(msg_.empty() ? ":door: **" + player + "** has left the game." : substitute_placeholders(msg_, {{"player", player}, {"node", node}, {"reason", reason}}));
        embed.set_color(0xED4245);
        embed.set_timestamp(std::time(nullptr));
        return embed;
    }
private:
    std::string msg_;
};

auto create_player_quit_card_module(const std::string& msg) -> std::unique_ptr<Module> {
    return std::make_unique<PlayerQuitCardModule>(msg);
}
