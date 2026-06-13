#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include <dpp/dpp.h>
#include <optional>

class ServerChatCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "server_chat_card"; }
    auto description() const -> std::string_view override { return "Relay SRB2 server messages as Discord embeds"; }
    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override { return {}; }

    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "SERVER_CHAT") return std::nullopt;
        if (event.fields.size() < 2) return std::nullopt;

        std::string message = event.fields[1];
        if (message.empty()) return std::nullopt;

        dpp::embed embed;
        embed.set_title(":loudspeaker: Server");
        embed.set_description(message);
        embed.set_color(0xFEE75C);
        return embed;
    }
};

auto create_server_chat_card_module() -> std::unique_ptr<Module> {
    return std::make_unique<ServerChatCardModule>();
}
