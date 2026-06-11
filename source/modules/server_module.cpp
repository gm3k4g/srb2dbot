#include "srb2dbot/module.hpp"
#include "srb2dbot/server.hpp"
#include "version.h"
#include <dpp/dpp.h>
#include <sstream>
#include <cstdlib>

class ServerControlModule : public Module {
public:
    auto name() const -> std::string_view override { return "server_control"; }
    auto description() const -> std::string_view override { return "Restart and stop the SRB2 server, send console commands"; }

    explicit ServerControlModule(bool fifo_enabled) : fifo_available_(fifo_enabled) {}

    auto commands(dpp::snowflake bot_id, dpp::permission perms) -> std::vector<dpp::slashcommand> override {
        dpp::slashcommand restart(CMD_RESTART_SERVER, "Restarts the SRB2 server.", bot_id);
        restart.set_default_permissions(perms);

        dpp::slashcommand stop(CMD_STOP_SERVER, "Stops the SRB2 server.", bot_id);
        stop.set_default_permissions(perms);

        dpp::slashcommand server_do(CMD_SERVER_DO, "Do something as the server.", bot_id);
        server_do.add_option(dpp::command_option(dpp::co_string, "server_command", "Server command to execute", true));
        server_do.set_default_permissions(perms);

        return {restart, stop, server_do};
    }

    auto handle_slashcommand(const dpp::slashcommand_t& event) -> bool override {
        auto cmd = event.command.get_command_name();
        std::string result;

        if (cmd == CMD_SERVER_DO) {
            std::string serv_cmd = std::get<std::string>(event.get_parameter("server_command"));
            if (!fifo_available_) {
                result = "```FIFO not available — use srb2-fifo binary for pipe commands.```";
            } else {
                bool success = pipe_srb2_server_do(serv_cmd);
                result = success ? "```Success```" : "```Failed to execute command```";
            }
            event.reply(dpp::message(event.command.channel_id, result).set_flags(dpp::m_ephemeral));
            return true;
        }

        if (cmd == CMD_RESTART_SERVER) {
            std::string sys_cmd = "systemctl restart " + service_name_ + " --user";
            int ret = std::system(sys_cmd.c_str());
            result = (ret == 0) ? "```Server was succesfully restarted!```\n" : "```Failed to restart server```\n";
            event.reply(dpp::message(event.command.channel_id, result).set_flags(dpp::m_ephemeral));
            return true;
        }

        if (cmd == CMD_STOP_SERVER) {
            std::string sys_cmd = "systemctl stop " + service_name_ + " --user";
            int ret = std::system(sys_cmd.c_str());
            result = (ret == 0) ? "```Server was stopped.```\n" : "```Failed to stop server```\n";
            event.reply(dpp::message(event.command.channel_id, result).set_flags(dpp::m_ephemeral));
            return true;
        }

        return false;
    }

private:
    bool fifo_available_;
    std::string service_name_ = "srb2@srb2b";
};

auto create_server_module(bool fifo_available) -> std::unique_ptr<Module> {
    return std::make_unique<ServerControlModule>(fifo_available);
}
