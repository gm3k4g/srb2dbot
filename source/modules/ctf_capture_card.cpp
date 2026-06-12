#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include "srb2dbot/utils.hpp"
#include <dpp/dpp.h>
#include <optional>

class CtfCaptureCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "ctf_capture_card"; }
    auto description() const -> std::string_view override { return "Discord embed when a flag is captured"; }
    explicit CtfCaptureCardModule(std::string msg) : msg_(std::move(msg)) {}
    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override { return {}; }
    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "CTF_CAPTURE") return std::nullopt;
        std::string player = event.fields.size() >= 1 ? event.fields[0] : "Someone";
        std::string team = event.fields.size() >= 2 ? event.fields[1] : "";
        dpp::embed embed;
        embed.set_title(msg_.empty()
            ? "Flag Captured!"
            : substitute_placeholders(msg_, {{"player", player}, {"team", team}}));
        embed.set_description("**" + player + "** has captured the **" + team + "** flag!");
        embed.set_color(team == "Blue" ? 0x5865F2 : 0xED4245);
        return embed;
    }
private:
    std::string msg_;
};

auto create_ctf_capture_card_module(const std::string& msg) -> std::unique_ptr<Module> {
    return std::make_unique<CtfCaptureCardModule>(msg);
}
