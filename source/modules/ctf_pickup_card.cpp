#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include "srb2dbot/utils.hpp"
#include <dpp/dpp.h>
#include <optional>

class CtfPickupCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "ctf_pickup_card"; }
    auto description() const -> std::string_view override { return "Discord embed when a flag is picked up"; }
    explicit CtfPickupCardModule(std::string msg) : msg_(std::move(msg)) {}
    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override { return {}; }
    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "CTF_PICKUP") return std::nullopt;
        std::string player = event.fields.size() >= 1 ? event.fields[0] : "Someone";
        dpp::embed embed;
        embed.set_title(msg_.empty() ? "Flag Taken" : substitute_placeholders(msg_, {{"player", player}}));
        embed.set_description("**" + player + "** picked up the flag.");
        embed.set_color(0xFEE75C);
        return embed;
    }
private:
    std::string msg_;
};

auto create_ctf_pickup_card_module(const std::string& msg) -> std::unique_ptr<Module> {
    return std::make_unique<CtfPickupCardModule>(msg);
}
