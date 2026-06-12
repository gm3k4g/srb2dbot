#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include <dpp/dpp.h>
#include <optional>

class ChatCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "chat_card"; }
    auto description() const -> std::string_view override { return "Relay SRB2 chat messages as Discord embeds"; }
    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override { return {}; }

    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "CHAT") return std::nullopt;
        if (event.fields.size() < 3) return std::nullopt;

        std::string player = event.fields[1];
        std::string message = event.fields[2];
        if (message.empty()) return std::nullopt;

        dpp::embed embed;
        embed.set_author(player, "", "");
        embed.set_description(message);
        embed.set_color(name_color(player));
        return embed;
    }

private:
    static auto name_color(const std::string& name) -> uint32_t {
        if (name.find("~Server") != std::string::npos) return 0xFEE75C;
        uint32_t h = 0x2F3136;
        for (char c : name) h = h * 33 + static_cast<uint32_t>(c);
        return 0x2F3136 + (h % 0xCD5C5C);
    }
};

auto create_chat_card_module() -> std::unique_ptr<Module> {
    return std::make_unique<ChatCardModule>();
}
