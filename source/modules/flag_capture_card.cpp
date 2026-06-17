#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include <dpp/dpp.h>
#include <optional>

class CtfCaptureCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "flag_capture_card"; }
    auto description() const -> std::string_view override { return "Discord embed when a flag is captured"; }
    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override { return {}; }
    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "CTF_CAPTURE") return std::nullopt;
        std::string player = event.fields.size() >= 1 ? sanitize_for_discord(event.fields[0]) : "Someone";
        std::string team = event.fields.size() >= 2 ? event.fields[1] : "";
        dpp::embed embed;
        embed.set_title("Flag Captured!");
        embed.set_description("**" + player + "** has captured the **" + team + "** flag!");
        embed.set_color(team == "Blue" ? 0x5865F2 : 0xED4245);
        return embed;
    }
};

auto create_flag_capture_card_module() -> std::unique_ptr<Module> {
    return std::make_unique<CtfCaptureCardModule>();
}
