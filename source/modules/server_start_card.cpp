#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include <dpp/dpp.h>
#include <optional>

class ServerStartCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "server_start_card"; }
    auto description() const -> std::string_view override { return "Discord embed card when the SRB2 server starts"; }

    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override {
        return {};
    }

    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type == "SERVER_START") {
            dpp::embed embed;
            embed.set_title(":green_circle: The server has started");
            embed.set_color(0x57F287);
            return embed;
        }
        return std::nullopt;
    }
};

auto create_server_start_card_module() -> std::unique_ptr<Module> {
    return std::make_unique<ServerStartCardModule>();
}
