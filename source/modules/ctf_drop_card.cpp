#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include "srb2dbot/utils.hpp"
#include <dpp/dpp.h>
#include <optional>

class CtfDropCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "ctf_drop_card"; }
    auto description() const -> std::string_view override { return "Discord embed when a flag is dropped"; }
    explicit CtfDropCardModule(std::string msg) : msg_(std::move(msg)) {}
    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override { return {}; }
    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "CTF_DROP") return std::nullopt;
        std::string player = event.fields.size() >= 1 ? event.fields[0] : "Someone";
        dpp::embed embed;
        embed.set_title(msg_.empty() ? "Flag Dropped" : substitute_placeholders(msg_, {{"player", player}}));
        embed.set_description("**" + player + "** dropped the flag.");
        embed.set_color(0xED4245);
        return embed;
    }
private:
    std::string msg_;
};

auto create_ctf_drop_card_module(const std::string& msg) -> std::unique_ptr<Module> {
    return std::make_unique<CtfDropCardModule>(msg);
}
