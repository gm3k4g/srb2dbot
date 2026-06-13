#pragma once

#include <dpp/dpp.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <memory>

struct BridgeEvent;

class Module {
public:
    virtual ~Module() = default;

    virtual auto name() const -> std::string_view = 0;
    virtual auto description() const -> std::string_view = 0;

    virtual auto commands(dpp::snowflake bot_id, dpp::permission perms) -> std::vector<dpp::slashcommand> = 0;
    virtual auto handle_slashcommand(const dpp::slashcommand_t& event) -> bool { return false; }
    virtual auto handle_message(const dpp::message_create_t& event) -> bool { return false; }
    virtual auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> { return std::nullopt; }
    virtual auto handle_bridge_plain_message(const BridgeEvent& event) -> std::optional<std::string> { return std::nullopt; }
    virtual auto get_bridge_attachment(const BridgeEvent& event) -> std::optional<std::pair<std::string, std::string>> { return std::nullopt; }
    virtual auto on_ready(dpp::cluster& bot, dpp::snowflake bridge_channel) -> void {}
    virtual auto on_timer_tick(dpp::cluster&) -> void {}
};

struct RegistryContext {
    dpp::cluster& bot;
    bool fifo_available;
    std::string bridge_channel_id;
    std::string bot_id;
    std::string srb2_dir;
};

class ModuleRegistry {
public:
    using json = nlohmann::json;

    auto load_from_config(const std::string& config_path, const RegistryContext& ctx) -> bool;
    auto all_commands(dpp::snowflake bot_id, dpp::permission perms) -> std::vector<dpp::slashcommand>;
    auto handle_slashcommand(const dpp::slashcommand_t& event) -> bool;
    auto handle_message(const dpp::message_create_t& event) -> bool;
    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed>;
    auto handle_bridge_plain_message(const BridgeEvent& event) -> std::optional<std::string>;
    auto get_bridge_attachment(const BridgeEvent& event) -> std::optional<std::pair<std::string, std::string>>;
    auto on_ready(dpp::cluster& bot, dpp::snowflake bridge_channel) -> void;
    auto on_timer_tick(dpp::cluster& bot) -> void;
    auto is_module_enabled(std::string_view name) const -> bool;

private:
    std::vector<std::unique_ptr<Module>> modules_;
    json config_;
};
