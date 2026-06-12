#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include <dpp/dpp.h>
#include <optional>

class CtfPickupCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "flag_pickup_card"; }
    auto description() const -> std::string_view override { return "Discord embed when a flag is picked up"; }
    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override { return {}; }
    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "CTF_PICKUP") return std::nullopt;
        std::string player = event.fields.size() >= 1 ? event.fields[0] : "Someone";
        dpp::embed embed;
        embed.set_title("Flag Taken");
        embed.set_description("**" + player + "** picked up the flag.");
        embed.set_color(0xFEE75C);
        return embed;
    }
};

auto create_flag_pickup_card_module() -> std::unique_ptr<Module> {
    return std::make_unique<CtfPickupCardModule>();
}
