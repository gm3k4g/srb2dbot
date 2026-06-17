#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include <dpp/dpp.h>
#include <optional>

class CtfReturnCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "flag_return_card"; }
    auto description() const -> std::string_view override { return "Discord embed when a flag is returned"; }
    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override { return {}; }
    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "CTF_RETURN") return std::nullopt;
        std::string player = event.fields.size() >= 1 ? sanitize_for_discord(event.fields[0]) : "Server";
        dpp::embed embed;
        embed.set_title("Flag Returned");
        embed.set_description("The flag was returned by **" + player + "**.");
        embed.set_color(0x747F8D);
        return embed;
    }
};

auto create_flag_return_card_module() -> std::unique_ptr<Module> {
    return std::make_unique<CtfReturnCardModule>();
}
