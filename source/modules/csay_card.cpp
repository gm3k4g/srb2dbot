#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include <dpp/dpp.h>
#include <optional>

class CsayCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "csay_card"; }
    auto description() const -> std::string_view override { return "Discord embed for server announcements (/say)"; }
    explicit CsayCardModule(std::string msg) : msg_(std::move(msg)) {}
    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override { return {}; }

    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "CSAY") return std::nullopt;
        if (event.fields.empty()) return std::nullopt;

        std::string message = event.fields[0];
        for (size_t i = 1; i < event.fields.size(); i++) {
            message += "|" + event.fields[i];
        }

        dpp::embed embed;
        embed.set_title(msg_.empty() ? ":loudspeaker: Server Announcement" : msg_);
        embed.set_description(message);
        embed.set_color(0xFEE75C);
        return embed;
    }
private:
    std::string msg_;
};

auto create_csay_card_module(const std::string& msg) -> std::unique_ptr<Module> {
    return std::make_unique<CsayCardModule>(msg);
}
