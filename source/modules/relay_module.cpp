#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include "srb2dbot/server.hpp"
#include <dpp/dpp.h>
#include <fstream>
#include <filesystem>

class ChatRelayModule : public Module {
public:
    auto name() const -> std::string_view override { return "chat_relay"; }
    auto description() const -> std::string_view override { return "Relay chat messages between Discord and SRB2"; }

    explicit ChatRelayModule(const std::string& bridge_id = "", const std::string& bot_id_val = "") {
        if (!bridge_id.empty() && bridge_id != "0") {
            bridge_channel_sf_ = std::stoull(bridge_id);
        }
        bot_id_ = bot_id_val;
    }

    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override {
        return {};
    }

    auto handle_message(const dpp::message_create_t& event) -> bool override {
        if (!bridge_channel_sf_.has_value()) return false;

        dpp::snowflake channel_id = bridge_channel_sf_.value();
        if (event.msg.channel_id != channel_id) return false;

        std::string author_str = std::to_string(event.msg.author.id);
        if (author_str == bot_id_) return false;
        if (event.msg.author.is_bot()) return false;

        std::string content = event.msg.content;
        if (content.empty()) return false;

        std::string sanitized = sanitize_message_for_srb2(content);
        if (sanitized.size() <= 1) return false;

        std::string display_name = event.msg.author.global_name;
        if (display_name.empty()) {
            display_name = event.msg.author.username;
        }
        for (auto& c : display_name) {
            if (c == '<' || c == '>' || c == '|' || c == '\\' || c == '"' || c == '\'' || c == ';' || c == '^' || static_cast<unsigned char>(c) > 0x7E) c = '_';
        }

        // Message written raw — semicolons and other chars are preserved.
        // Lua reads this file and calls chatprint() directly, bypassing the
        // SRB2 console command parser entirely, so no command injection is possible.
        std::string home = dir_srb2_str();
        std::string bridge_path = home + "/client/DiscordBot";
        std::filesystem::create_directories(bridge_path);
        std::string disc_path = bridge_path + "/discordmessage.txt";
        std::cout << "[relay] Writing to " << disc_path << std::endl;
        std::ofstream disc_file(disc_path, std::ios::app);
        if (disc_file.is_open()) {
            disc_file << display_name << "|" << sanitized << "\n";
        }
#ifndef NDEBUG
        std::cout << "[bridge] Discord→SRB2: <" << display_name << "> " << sanitized << std::endl;
#endif
        return true;
    }

private:
    std::optional<dpp::snowflake> bridge_channel_sf_;
    std::string bot_id_;
};

auto create_relay_module(const std::string& bridge_channel, const std::string& bot_id, bool /*fifo_available*/) -> std::unique_ptr<Module> {
    return std::make_unique<ChatRelayModule>(bridge_channel, bot_id);
}
