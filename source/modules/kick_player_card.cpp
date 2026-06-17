#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include "srb2dbot/utils.hpp"
#include <dpp/dpp.h>
#include <ctime>
#include <optional>

class KickPlayerCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "kick_player_card"; }
    auto description() const -> std::string_view override { return "Discord embed when a player is kicked"; }
    explicit KickPlayerCardModule(std::string msg) : msg_(std::move(msg)) {}
    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override { return {}; }
    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "KICK_PLAYER") return std::nullopt;
        std::string player = event.fields.size() >= 1 ? sanitize_for_discord(event.fields[0]) : "Someone";
        std::string node = event.fields.size() >= 2 ? event.fields[1] : "";
        std::string reason = event.fields.size() >= 3 ? event.fields[2] : "";
        dpp::embed embed;
        embed.set_title(msg_.empty() ? ":boot: **" + player + "** was kicked." : substitute_placeholders(msg_, {{"player", player}, {"node", node}, {"reason", reason}}));
        embed.set_color(0xED4245);
        embed.set_timestamp(std::time(nullptr));
        return embed;
    }
private:
    std::string msg_;
};

auto create_kick_player_card_module(const std::string& msg) -> std::unique_ptr<Module> {
    return std::make_unique<KickPlayerCardModule>(msg);
}
