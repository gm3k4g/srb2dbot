#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include <dpp/dpp.h>
#include <optional>
#include <cstdlib>

class ChatCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "chat_card"; }
    auto description() const -> std::string_view override { return "Relay SRB2 chat messages to Discord"; }

    explicit ChatCardModule(std::string mode) : mode_(std::move(mode)) {}

    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override { return {}; }

    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "CHAT") return std::nullopt;
        if (mode_ != "card") return std::nullopt;
        if (event.fields.size() < 3) return std::nullopt;

        std::string player = event.fields[1];
        std::string message = event.fields[2];
        if (message.empty()) return std::nullopt;

        dpp::embed embed;
        embed.set_title(player);
        embed.set_description(message);
        embed.set_color(parse_color(event));
        return embed;
    }

    auto handle_bridge_plain_message(const BridgeEvent& event) -> std::optional<std::string> override {
        if (event.type != "CHAT") return std::nullopt;
        if (mode_ != "message") return std::nullopt;
        if (event.fields.size() < 3) return std::nullopt;

        std::string player = event.fields[1];
        std::string message = event.fields[2];
        if (message.empty()) return std::nullopt;

        return "**" + player + "** " + message;
    }

private:
    std::string mode_;

    static auto parse_color(const BridgeEvent& event) -> uint32_t {
        if (event.fields.size() >= 4) {
            std::string hex = event.fields[3];
            if (!hex.empty()) {
                char* end = nullptr;
                uint32_t c = std::strtoul(hex.c_str(), &end, 16);
                if (end && *end == '\0' && c > 0) return c;
            }
        }
        return 0x57F287;
    }
};

auto create_chat_card_module(const std::string& mode) -> std::unique_ptr<Module> {
    return std::make_unique<ChatCardModule>(mode);
}
