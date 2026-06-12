#include "srb2dbot/module.hpp"
#include "srb2dbot/utils.hpp"
#include "version.h"
#include <dpp/dpp.h>

class ServerStartCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "server_start_card"; }
    auto description() const -> std::string_view override { return "Discord embed card when the bot starts"; }

    explicit ServerStartCardModule(dpp::snowflake channel, std::string msg)
        : channel_(channel), msg_(std::move(msg)) {}

    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override {
        return {};
    }

    auto on_ready(dpp::cluster& bot, dpp::snowflake bridge_channel) -> void override {
        dpp::snowflake ch = channel_ != 0 ? channel_ : bridge_channel;
        if (ch == 0) return;
        dpp::embed embed;
        embed.set_title(msg_.empty() ? ":green_circle: The server has started" : msg_);
        embed.set_description(PROJECT_DESCRIPTION);
        embed.add_field("Version", PROJECT_VERSION_MAJOR "." PROJECT_VERSION_MINOR, true);
        embed.add_field("Author", PROJECT_AUTHOR, true);
        embed.add_field("Repository", PROJECT_URL, false);
        embed.set_color(0x57F287);
        embed.set_timestamp(std::time(nullptr));
        bot.message_create(dpp::message(ch, "").add_embed(embed));
    }

private:
    dpp::snowflake channel_;
    std::string msg_;
};

auto create_server_start_card_module(dpp::snowflake channel, const std::string& msg) -> std::unique_ptr<Module> {
    return std::make_unique<ServerStartCardModule>(channel, msg);
}
