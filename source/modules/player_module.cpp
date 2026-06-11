#include "srb2dbot/module.hpp"
#include "srb2dbot/server.hpp"
#include "version.h"
#include <dpp/dpp.h>
#include <fstream>
#include <sstream>

class PlayerManagementModule : public Module {
public:
    auto name() const -> std::string_view override { return "player_management"; }
    auto description() const -> std::string_view override { return "Kick, ban, and message players in-game"; }

    explicit PlayerManagementModule(bool fifo_enabled) : fifo_available_(fifo_enabled) {}

    auto commands(dpp::snowflake bot_id, dpp::permission perms) -> std::vector<dpp::slashcommand> override {
        dpp::slashcommand server_say(CMD_SERVER_SAY, "Say something as the server", bot_id);
        server_say.add_option(dpp::command_option(dpp::co_string, "server_message", "Message that the server will say", true));
        server_say.set_default_permissions(perms);

        dpp::slashcommand kick(CMD_KICK_PLAYER, "Kicks a player off the server", bot_id);
        kick.add_option(dpp::command_option(dpp::co_string, "player", "Player to kick from the server.", true));
        kick.set_default_permissions(perms);

        dpp::slashcommand ban(CMD_BAN_PLAYER, "Bans a player off the server", bot_id);
        ban.add_option(dpp::command_option(dpp::co_string, "player", "Player to ban from the server.", true));
        ban.set_default_permissions(perms);

        return {server_say, kick, ban};
    }

    auto handle_slashcommand(const dpp::slashcommand_t& event) -> bool override {
        auto cmd = event.command.get_command_name();

        if (cmd == CMD_SERVER_SAY) {
            std::string msg = std::get<std::string>(event.get_parameter("server_message"));
            std::string result;
            if (!fifo_available_) {
                result = "```FIFO not available — use srb2-fifo binary for pipe commands.```";
            } else {
                bool success = pipe_srb2_server_say(msg);
                result = success ? "```Success```" : "```Failed to say message.```";
            }
            event.reply(dpp::message(event.command.channel_id, result).set_flags(dpp::m_ephemeral));
            return true;
        }

        if (cmd == CMD_KICK_PLAYER) {
            std::string player = std::get<std::string>(event.get_parameter("player"));
            bool kicked = false;
            std::stringstream result;
            if (!fifo_available_) {
                result << "```FIFO not available — use srb2-fifo binary for pipe commands.```\n";
            } else {
                kicked = pipe_srb2_kick_player(player);
                result << (kicked ? "```Attempted to kick player " : "```Failed to kick player ")
                       << player << " .```\n";
            }
            event.reply(dpp::message(event.command.channel_id, result.str()).set_flags(dpp::m_ephemeral));

            if (kicked) {
                std::string home = dir_srb2_str();
                std::ofstream msgs(home + "/luafiles/client/DiscordBot/Messages.txt", std::ios::app);
                if (msgs.is_open())
                    msgs << "[EVENT:KICK_PLAYER]|" << player << "\n";
            }
            return true;
        }

        if (cmd == CMD_BAN_PLAYER) {
            std::string player = std::get<std::string>(event.get_parameter("player"));
            bool banned = false;
            std::stringstream result;
            if (!fifo_available_) {
                result << "```FIFO not available — use srb2-fifo binary for pipe commands.```\n";
            } else {
                banned = pipe_srb2_ban_player(player);
                result << (banned ? "```Attempted to ban player " : "```Failed to ban player ")
                       << player << " .```\n";
            }
            event.reply(dpp::message(event.command.channel_id, result.str()).set_flags(dpp::m_ephemeral));

            if (banned) {
                std::string home = dir_srb2_str();
                std::ofstream msgs(home + "/luafiles/client/DiscordBot/Messages.txt", std::ios::app);
                if (msgs.is_open())
                    msgs << "[EVENT:BAN_PLAYER]|" << player << "\n";
            }
            return true;
        }

        return false;
    }

private:
    bool fifo_available_;
};

auto create_player_module(bool fifo_available) -> std::unique_ptr<Module> {
    return std::make_unique<PlayerManagementModule>(fifo_available);
}
