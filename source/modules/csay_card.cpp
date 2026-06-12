#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include <dpp/dpp.h>
#include <optional>

class CsayCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "csay_card"; }
    auto description() const -> std::string_view override { return "Discord embed for server announcements (/say)"; }
    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override { return {}; }

    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "CSAY") return std::nullopt;
        if (event.fields.empty()) return std::nullopt;

        dpp::embed embed;
        embed.set_title(":loudspeaker: Server Announcement");
        embed.set_description(event.fields[0]);
        embed.set_color(0xFEE75C);
        return embed;
    }
};

auto create_csay_card_module() -> std::unique_ptr<Module> {
    return std::make_unique<CsayCardModule>();
}
