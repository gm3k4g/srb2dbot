#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include <dpp/dpp.h>
#include <optional>

class ServerStartCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "server_start_card"; }
    auto description() const -> std::string_view override { return "Discord embed card when the bot starts or SRB2 server starts"; }

    explicit ServerStartCardModule(dpp::snowflake channel) : channel_(channel) {}

    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override {
        return {};
    }

    auto on_ready(dpp::cluster& bot, dpp::snowflake bridge_channel) -> void override {
        dpp::snowflake ch = channel_ != 0 ? channel_ : bridge_channel;
        if (ch == 0) return;
        dpp::embed embed;
        embed.set_title(":green_circle: The server has started");
        embed.set_color(0x57F287);
        embed.set_timestamp(std::time(nullptr));
        bot.message_create(dpp::message(ch, "").add_embed(embed));
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

private:
    dpp::snowflake channel_;
};

auto create_server_start_card_module(dpp::snowflake channel) -> std::unique_ptr<Module> {
    return std::make_unique<ServerStartCardModule>(channel);
}
