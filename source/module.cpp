#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include "srb2dbot/server.hpp"
#include "srb2dbot/script.hpp"
#include "srb2dbot/utils.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>

// Module factory functions
auto create_script_module() -> std::unique_ptr<Module>;
auto create_wad_module(dpp::cluster& bot) -> std::unique_ptr<Module>;
auto create_server_module(bool fifo) -> std::unique_ptr<Module>;
auto create_player_module(bool fifo) -> std::unique_ptr<Module>;
auto create_relay_module(const std::string& bridge_channel, const std::string& bot_id, bool fifo) -> std::unique_ptr<Module>;
auto create_round_start_card_module() -> std::unique_ptr<Module>;
auto create_round_end_card_module() -> std::unique_ptr<Module>;
auto create_ctf_capture_card_module() -> std::unique_ptr<Module>;
auto create_ctf_drop_card_module() -> std::unique_ptr<Module>;
auto create_ctf_pickup_card_module() -> std::unique_ptr<Module>;
auto create_ctf_return_card_module() -> std::unique_ptr<Module>;
auto create_player_join_card_module() -> std::unique_ptr<Module>;
auto create_player_quit_card_module() -> std::unique_ptr<Module>;
auto create_kick_player_card_module() -> std::unique_ptr<Module>;
auto create_ban_player_card_module() -> std::unique_ptr<Module>;
auto create_server_start_card_module(dpp::snowflake channel) -> std::unique_ptr<Module>;
auto create_thumbnails_module(const std::string& srb2_dir) -> std::unique_ptr<Module>;

auto ModuleRegistry::load_from_config(const std::string& config_path, const RegistryContext& ctx) -> bool {
    std::ifstream file(config_path);
    if (!file.is_open()) {
        std::cerr << "[module] WARNING: Could not open " << config_path
                  << " — all modules enabled by default" << std::endl;
        config_ = json::object();
        modules_.push_back(create_script_module());
        modules_.push_back(create_wad_module(ctx.bot));
        modules_.push_back(create_server_module(ctx.fifo_available));
        modules_.push_back(create_player_module(ctx.fifo_available));
        modules_.push_back(create_relay_module(ctx.bridge_channel_id, ctx.bot_id, ctx.fifo_available));
        modules_.push_back(create_round_start_card_module());
        modules_.push_back(create_round_end_card_module());
        modules_.push_back(create_ctf_capture_card_module());
        modules_.push_back(create_ctf_drop_card_module());
        modules_.push_back(create_ctf_pickup_card_module());
        modules_.push_back(create_ctf_return_card_module());
        modules_.push_back(create_player_join_card_module());
        modules_.push_back(create_player_quit_card_module());
        modules_.push_back(create_kick_player_card_module());
        modules_.push_back(create_ban_player_card_module());
        dpp::snowflake ch = ctx.bridge_channel_id != "0" ? std::stoull(ctx.bridge_channel_id) : 0;
        modules_.push_back(create_server_start_card_module(ch));
        return true;
    }

    try {
        config_ = json::parse(file);
    } catch (const json::parse_error& e) {
        std::cerr << "[module] ERROR: Failed to parse " << config_path << ": " << e.what() << std::endl;
        return false;
    }

    auto& mods = config_["modules"];

    auto try_add = [&](std::string_view name, auto&& factory) {
        bool enabled = true;
        // Search across all categories (slash_commands, chat, auto)
        for (auto& [key, val] : mods.items()) {
            if (val.contains(name)) {
                enabled = val[name].get<bool>();
                break;
            }
        }
        if (enabled) {
            modules_.push_back(factory());
            std::cout << "[module] " << name << " enabled" << std::endl;
        } else {
            std::cout << "[module] " << name << " disabled" << std::endl;
        }
    };

    try_add("script_editor",    [&]{ return create_script_module(); });
    try_add("wad_manager",      [&]{ return create_wad_module(ctx.bot); });
    try_add("server_control",   [&]{ return create_server_module(ctx.fifo_available); });
    try_add("player_management",[&]{ return create_player_module(ctx.fifo_available); });
    try_add("chat_relay",       [&]{ return create_relay_module(ctx.bridge_channel_id, ctx.bot_id, ctx.fifo_available); });
    dpp::snowflake bridge_sf = ctx.bridge_channel_id != "0" ? std::stoull(ctx.bridge_channel_id) : 0;
    try_add("server_start_card",    [&]{ return create_server_start_card_module(bridge_sf); });
    try_add("round_start_card",     [&]{ return create_round_start_card_module(); });
    try_add("round_end_card",       [&]{ return create_round_end_card_module(); });
    try_add("ctf_capture_card",     [&]{ return create_ctf_capture_card_module(); });
    try_add("ctf_drop_card",        [&]{ return create_ctf_drop_card_module(); });
    try_add("ctf_pickup_card",      [&]{ return create_ctf_pickup_card_module(); });
    try_add("ctf_return_card",      [&]{ return create_ctf_return_card_module(); });
    try_add("player_join_card",     [&]{ return create_player_join_card_module(); });
    try_add("player_quit_card",     [&]{ return create_player_quit_card_module(); });
    try_add("kick_player_card",     [&]{ return create_kick_player_card_module(); });
    try_add("ban_player_card",      [&]{ return create_ban_player_card_module(); });
    try_add("map_thumbnails_card",  [&]{ return create_thumbnails_module(ctx.srb2_dir); });

    return true;
}

auto ModuleRegistry::all_commands(dpp::snowflake bot_id, dpp::permission perms) -> std::vector<dpp::slashcommand> {
    std::vector<dpp::slashcommand> cmds;
    for (auto& mod : modules_) {
        auto mod_cmds = mod->commands(bot_id, perms);
        cmds.insert(cmds.end(), mod_cmds.begin(), mod_cmds.end());
    }
    return cmds;
}

auto ModuleRegistry::handle_slashcommand(const dpp::slashcommand_t& event) -> bool {
    for (auto& mod : modules_) {
        if (mod->handle_slashcommand(event)) return true;
    }
    return false;
}

auto ModuleRegistry::handle_message(const dpp::message_create_t& event) -> bool {
    for (auto& mod : modules_) {
        if (mod->handle_message(event)) return true;
    }
    return false;
}

auto ModuleRegistry::handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> {
    for (auto& mod : modules_) {
        auto result = mod->handle_bridge_event(event);
        if (result.has_value()) return result;
    }
    return std::nullopt;
}

auto ModuleRegistry::get_bridge_attachment(const BridgeEvent& event) -> std::optional<std::pair<std::string, std::string>> {
    for (auto& mod : modules_) {
        auto result = mod->get_bridge_attachment(event);
        if (result.has_value()) return result;
    }
    return std::nullopt;
}

auto ModuleRegistry::on_ready(dpp::cluster& bot, dpp::snowflake bridge_channel) -> void {
    for (auto& mod : modules_) {
        mod->on_ready(bot, bridge_channel);
    }
}

auto ModuleRegistry::on_timer_tick(dpp::cluster& bot) -> void {
    for (auto& mod : modules_) {
        mod->on_timer_tick(bot);
    }
}

auto ModuleRegistry::is_module_enabled(std::string_view name) const -> bool {
    if (!config_.contains("modules")) return true;
    auto& mods = config_["modules"];
    for (auto& [key, val] : mods.items()) {
        if (val.is_object() && val.contains(name)) {
            return val[name].get<bool>();
        }
    }
    return true;
}
