#include "srb2dbot/bridge.hpp"
#include "srb2dbot/module.hpp"
#include "srb2dbot/server.hpp"
#include "version.h"
#include <dpp/dpp.h>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>

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
            std::string serv_cmd = sanitize_message_for_srb2(std::get<std::string>(event.get_parameter("server_command")));
            if (!fifo_available_) {
                result = "```FIFO not available - use srb2-fifo binary for pipe commands.```";
            } else {
                bool success = pipe_srb2_server_do(serv_cmd);
                result = success ? "```Success```" : "```Failed to execute command```";
            }
            event.reply(dpp::message(event.command.channel_id, result).set_flags(dpp::m_ephemeral));
            return true;
        }

        if (cmd == CMD_RESTART_SERVER) {
            int ret = run_systemctl("restart");
            result = (ret == 0) ? "```Server was succesfully restarted!```\n" : "```Failed to restart server```\n";
            event.reply(dpp::message(event.command.channel_id, result).set_flags(dpp::m_ephemeral));
            return true;
        }

        if (cmd == CMD_STOP_SERVER) {
            int ret = run_systemctl("stop");
            result = (ret == 0) ? "```Server was stopped.```\n" : "```Failed to stop server```\n";
            event.reply(dpp::message(event.command.channel_id, result).set_flags(dpp::m_ephemeral));
            return true;
        }

        return false;
    }

private:
    bool fifo_available_;
    std::string service_name_ = "srb2@srb2b";

    auto run_systemctl(const std::string& verb) -> int {
        pid_t pid = fork();
        if (pid == -1) return -1;
        if (pid == 0) {
            execlp("systemctl", "systemctl", verb.c_str(), service_name_.c_str(), "--user", (char*)nullptr);
            _exit(EXIT_FAILURE);
        }
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    }
};

auto create_server_module(bool fifo_available) -> std::unique_ptr<Module> {
    return std::make_unique<ServerControlModule>(fifo_available);
}
