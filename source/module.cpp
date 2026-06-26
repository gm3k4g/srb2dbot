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
auto create_round_start_card_module(const std::string& msg, bool thumbs, const std::string& srb2_dir) -> std::unique_ptr<Module>;
auto create_round_end_card_module(const std::string& msg, bool thumbs, const std::string& srb2_dir) -> std::unique_ptr<Module>;
auto create_flag_capture_card_module() -> std::unique_ptr<Module>;
auto create_flag_drop_card_module() -> std::unique_ptr<Module>;
auto create_flag_pickup_card_module() -> std::unique_ptr<Module>;
auto create_flag_return_card_module() -> std::unique_ptr<Module>;
auto create_player_join_card_module(const std::string& msg) -> std::unique_ptr<Module>;
auto create_player_quit_card_module(const std::string& msg) -> std::unique_ptr<Module>;
auto create_kick_player_card_module(const std::string& msg) -> std::unique_ptr<Module>;
auto create_ban_player_card_module(const std::string& msg) -> std::unique_ptr<Module>;
auto create_server_start_card_module(dpp::snowflake channel, const std::string& msg) -> std::unique_ptr<Module>;
auto create_chat_card_module(const std::string& fmt) -> std::unique_ptr<Module>;
auto create_server_chat_card_module(const std::string& msg) -> std::unique_ptr<Module>;
auto create_csay_card_module(const std::string& msg) -> std::unique_ptr<Module>;
auto create_shutdown_card_module(const std::string& msg, const std::string& srb2_dir) -> std::unique_ptr<Module>;
// create_thumbnails_module removed  -  thumbnails integrated into round_start_card

auto ModuleRegistry::load_from_config(const std::string& config_path, const RegistryContext& ctx) -> bool {
    std::ifstream file(config_path);
    if (!file.is_open()) {
        std::cerr << "[module] WARNING: Could not open " << config_path
                  << "  -  all modules enabled by default" << std::endl;
        config_ = json::object();
        modules_.push_back(create_script_module());
        modules_.push_back(create_wad_module(ctx.bot));
        modules_.push_back(create_server_module(ctx.fifo_available));
        modules_.push_back(create_player_module(ctx.fifo_available));
        modules_.push_back(create_relay_module(ctx.bridge_channel_id, ctx.bot_id, ctx.fifo_available));
        modules_.push_back(create_round_start_card_module("", false, ctx.srb2_dir));
        modules_.push_back(create_round_end_card_module("", false, ctx.srb2_dir));
        modules_.push_back(create_flag_capture_card_module());
        modules_.push_back(create_flag_drop_card_module());
        modules_.push_back(create_flag_pickup_card_module());
        modules_.push_back(create_flag_return_card_module());
        modules_.push_back(create_player_join_card_module(""));
        modules_.push_back(create_player_quit_card_module(""));
        modules_.push_back(create_kick_player_card_module(""));
        modules_.push_back(create_ban_player_card_module(""));
        dpp::snowflake ch = ctx.bridge_channel_id != "0" ? std::stoull(ctx.bridge_channel_id) : 0;
        modules_.push_back(create_server_start_card_module(ch, ""));
        modules_.push_back(create_chat_card_module(""));
        modules_.push_back(create_server_chat_card_module(""));
        modules_.push_back(create_csay_card_module(""));
        modules_.push_back(create_shutdown_card_module("", ctx.srb2_dir));
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
                auto& entry = val[name];
                if (entry.is_object()) {
                    enabled = entry.value("enabled", true);
                } else {
                    enabled = entry.template get<bool>();
                }
                break;
            }
        }
        if (enabled) {
            modules_.push_back(factory());
#ifndef NDEBUG
            std::cout << "[module] " << name << " enabled" << std::endl;
#endif
        } else {
#ifndef NDEBUG
            std::cout << "[module] " << name << " disabled" << std::endl;
#endif
        }
    };

    auto try_add_msg = [&](std::string_view name, auto&& factory) {
        bool enabled = true;
        std::string msg;
        for (auto& [key, val] : mods.items()) {
            if (val.contains(name)) {
                auto& entry = val[name];
                if (entry.is_object()) {
                    enabled = entry.value("enabled", true);
                    msg = entry.value("message", "");
                    if (msg.empty()) msg = entry.value("format", "");
                } else {
                    enabled = entry.template get<bool>();
                }
                break;
            }
        }
        if (enabled) {
            modules_.push_back(factory(msg));
#ifndef NDEBUG
            std::cout << "[module] " << name << " enabled" << std::endl;
#endif
        } else {
#ifndef NDEBUG
            std::cout << "[module] " << name << " disabled" << std::endl;
#endif
        }
    };

    auto try_add_format = [&](std::string_view name, auto&& factory) {
        bool enabled = true;
        std::string fmt;
        for (auto& [key, val] : mods.items()) {
            if (val.contains(name)) {
                auto& entry = val[name];
                if (entry.is_object()) {
                    enabled = entry.value("enabled", true);
                    fmt = entry.value("format", "");
                } else {
                    enabled = entry.template get<bool>();
                }
                break;
            }
        }
        if (enabled) {
            modules_.push_back(factory(fmt));
#ifndef NDEBUG
            std::cout << "[module] " << name << " enabled" << std::endl;
#endif
        } else {
#ifndef NDEBUG
            std::cout << "[module] " << name << " disabled" << std::endl;
#endif
        }
    };

    try_add("script_editor",    [&]{ return create_script_module(); });
    try_add("wad_manager",      [&]{ return create_wad_module(ctx.bot); });
    try_add("server_control",   [&]{ return create_server_module(ctx.fifo_available); });
    try_add("player_management",[&]{ return create_player_module(ctx.fifo_available); });
    try_add("chat_relay",       [&]{ return create_relay_module(ctx.bridge_channel_id, ctx.bot_id, ctx.fifo_available); });
    dpp::snowflake bridge_sf = ctx.bridge_channel_id != "0" ? std::stoull(ctx.bridge_channel_id) : 0;
    try_add_msg("server_start_card", [&](auto& msg){ return create_server_start_card_module(bridge_sf, msg); });
    try_add_msg("round_start_card",     [&](auto& msg){
        bool thumbs = false;
        for (auto& [key, val] : mods.items()) {
            if (val.contains("round_start_card") && val["round_start_card"].is_object()) {
                thumbs = val["round_start_card"].value("thumbnails", false);
            }
        }
        return create_round_start_card_module(msg, thumbs, ctx.srb2_dir);
    });
    try_add_msg("round_end_card",       [&](auto& msg){
        bool thumbs = false;
        for (auto& [key, val] : mods.items()) {
            if (val.contains("round_end_card") && val["round_end_card"].is_object()) {
                thumbs = val["round_end_card"].value("thumbnails", false);
            }
        }
        return create_round_end_card_module(msg, thumbs, ctx.srb2_dir);
    });
    try_add("flag_capture_card",     [&]{ return create_flag_capture_card_module(); });
    try_add("flag_drop_card",        [&]{ return create_flag_drop_card_module(); });
    try_add("flag_pickup_card",      [&]{ return create_flag_pickup_card_module(); });
    try_add("flag_return_card",      [&]{ return create_flag_return_card_module(); });
    try_add_msg("player_join_card",     [&](auto& msg){ return create_player_join_card_module(msg); });
    try_add_msg("player_quit_card",     [&](auto& msg){ return create_player_quit_card_module(msg); });
    try_add_msg("kick_player_card",     [&](auto& msg){ return create_kick_player_card_module(msg); });
    try_add_msg("ban_player_card",      [&](auto& msg){ return create_ban_player_card_module(msg); });
    try_add_format("chat_card",            [&](auto& fmt){ return create_chat_card_module(fmt); });
    try_add_format("server_chat_card",     [&](auto& fmt){ return create_server_chat_card_module(fmt); });
    try_add_format("csay_card",            [&](auto& fmt){ return create_csay_card_module(fmt); });
    try_add_msg("shutdown_card", [&](auto& msg){
        return create_shutdown_card_module(msg, ctx.srb2_dir);
    });

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

auto ModuleRegistry::on_shutdown(dpp::cluster& bot, dpp::snowflake channel) -> void {
    for (auto& mod : modules_) {
        mod->on_shutdown(bot, channel);
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
