#include <dpp/dpp.h>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <unistd.h>
#include <algorithm>
#include <csignal>

#include "version.h"
#include "srb2dbot/utils.hpp"
#include "srb2dbot/script.hpp"
#include "srb2dbot/bridge.hpp"
#include "srb2dbot/module.hpp"
#include "srb2dbot/server.hpp"
using json = nlohmann::json;

const auto PERMS = dpp::p_ban_members;

int main() {
    std::ifstream secret("secret.json");
    if (!secret.is_open()) {
        std::cerr << "ERROR: Could not open secret.json\n";
        return EXIT_FAILURE;
    }
    json data;
    try {
        data = json::parse(secret);
    } catch (const json::parse_error& e) {
        std::cerr << "ERROR: Failed to parse secret.json: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    if (data.is_null()) {
        std::cerr << "ERROR: secret.json is empty or null\n";
        return EXIT_FAILURE;
    }

    std::string srb2_dir = dir_srb2_str();
    std::filesystem::create_directories(srb2_dir + "/srb2_servers.d/srb2b.d");
    std::filesystem::create_directories(srb2_dir + "/addons");

    std::string guild_id_str = data["guild_id"].get<std::string>();
    dpp::snowflake guild_id = 0;
    try {
        guild_id = std::stoll(guild_id_str);
    } catch (const std::exception&) {
        std::cerr << "ERROR: Invalid guild_id in secret.json\n";
        return EXIT_FAILURE;
    }

    std::string bot_token = data["bot_token"].get<std::string>();
    std::string bot_id = data.contains("bot_id") && data["bot_id"].is_string()
        ? data["bot_id"].get<std::string>() : "0";
    std::string bridge_channel_id = data.contains("channel_id") && data["channel_id"].is_string()
        ? data["channel_id"].get<std::string>() : "0";

    // ── Session-based logging ──
    std::string log_dir = std::string(getenv("HOME")) + "/Desktop/srb2dbot/logs";
    std::filesystem::create_directories(log_dir);
    std::time_t now = std::time(nullptr);
    char ts[64];
    std::strftime(ts, sizeof(ts), "%Y-%m-%d_%H-%M-%S", std::localtime(&now));
    std::string log_name = log_dir + "/" + ts + ".txt";
    auto log_stream = std::make_shared<std::ofstream>(log_name, std::ios::app);
    if (log_stream->is_open()) {
        std::string symlink_name = std::string(getenv("HOME")) + "/Desktop/srb2dbot/latest-log.txt";
        std::filesystem::remove(symlink_name);
        std::filesystem::create_symlink(log_name, symlink_name);
        std::vector<std::filesystem::path> old_logs;
        for (auto& entry : std::filesystem::directory_iterator(log_dir)) {
            if (entry.path().extension() == ".txt") old_logs.push_back(entry.path());
        }
        std::sort(old_logs.begin(), old_logs.end());
        while (old_logs.size() > 7) {
            std::filesystem::remove(old_logs.front());
            old_logs.erase(old_logs.begin());
        }
    }
    // ── Single-instance lock ──
    std::string pid_path = std::string(getenv("HOME")) + "/Desktop/srb2dbot/srb2dbot.pid";
    {
        std::ifstream pid_file(pid_path);
        if (pid_file.is_open()) {
            pid_t old_pid = 0;
            pid_file >> old_pid;
            pid_file.close();
            if (old_pid > 0) {
                std::string proc_path = "/proc/" + std::to_string(old_pid);
                if (std::filesystem::exists(proc_path)) {
                    std::cerr << "ERROR: srb2dbot is already running (PID " << old_pid << ")\n";
                    return EXIT_FAILURE;
                }
            }
        }
    }
    std::ofstream pid_file(pid_path, std::ios::trunc);
    if (pid_file.is_open()) {
        pid_file << getpid() << std::endl;
        pid_file.close();
    }
    std::atexit([]{
        std::string path = std::string(getenv("HOME")) + "/Desktop/srb2dbot/srb2dbot.pid";
        std::filesystem::remove(path);
    });
    signal(SIGINT, [](int) { std::exit(0); });
    signal(SIGTERM, [](int) { std::exit(0); });
    signal(SIGQUIT, [](int) { std::exit(0); });

    dpp::cluster bot(bot_token, dpp::i_default_intents | dpp::i_message_content);
    bot.on_log([log_stream](const dpp::log_t& event) {
        // Skip Discord Gateway heartbeat noise only, keep everything else (including error JSON)
        if (event.message.find("\"op\":1") != std::string::npos) return;
        std::cout << event.message << std::endl;
        if (log_stream && log_stream->is_open()) {
            *log_stream << event.message << std::endl;
        }
    });

    const bool fifo_available = detect_fifo_support();

    // Initialize Module Registry with all context
    ModuleRegistry registry;
    registry.load_from_config("modules.json", {
        bot, fifo_available, bridge_channel_id, bot_id, srb2_dir
    });

    // ── Slash command handler ──
    bot.on_slashcommand([&registry](const dpp::slashcommand_t& event) {
        dpp::permission perms = event.command.get_resolved_permission(event.command.usr.id);
        if (!perms.can(PERMS)) {
            event.reply(dpp::message(event.command.channel_id, "```Missing permissions```")
                .set_flags(dpp::m_ephemeral));
            return;
        }
        if (!registry.handle_slashcommand(event)) {
            event.reply(dpp::message(event.command.channel_id, "```Unknown command```")
                .set_flags(dpp::m_ephemeral));
        }
    });

    // ── Discord→SRB2 message handler ──
    bot.on_message_create([&registry](const dpp::message_create_t& event) {
        registry.handle_message(event);
    });

    // ── Ready handler: delete stale + register current commands ──
    bot.on_ready([&bot, guild_id, &registry](const dpp::ready_t&) {
        bot.guild_commands_get(bot.me.id, [&bot, guild_id](const dpp::confirmation_callback_t& cb){
            auto commands = std::get<dpp::slashcommand_map>(cb.value);
            for (auto& [id, cmd] : commands) {
                bot.guild_command_delete(id, guild_id);
            }
        });
        if (dpp::run_once<struct register_bot_commands>()) {
            auto cmds = registry.all_commands(bot.me.id, PERMS);
            bot.guild_bulk_command_create(cmds, guild_id);
        }
    });

    // ── Guild emoji loader + module on_ready notification ──
    std::unordered_map<std::string, std::string> guild_emojis;
    bot.on_ready([&bot, guild_id, &guild_emojis, &registry, bridge_channel_id](const dpp::ready_t&) {
        auto guild = dpp::find_guild(guild_id);
        if (guild) {
            for (const auto& emoji_id : guild->emojis) {
                auto emoji = dpp::find_emoji(emoji_id);
                if (emoji) {
                    guild_emojis[emoji->name] = std::to_string(emoji_id);
                }
            }
        }
        if (dpp::run_once<struct notify_modules_ready>()) {
            dpp::snowflake ch = bridge_channel_id != "0" ? std::stoull(bridge_channel_id) : 0;
            if (ch != 0) registry.on_ready(bot, ch);
        }
    });

    // ── Bridge setup ──
    std::string bridge_dir = srb2_dir + "/luafiles/client/DiscordBot";
    std::filesystem::create_directories(bridge_dir);
    std::string messages_path = bridge_dir + "/Messages.txt";
    {
        std::ofstream msg_file(messages_path, std::ios::trunc);
        if (msg_file.is_open()) msg_file << "\n";
    }
    {
        std::ofstream disc_file(bridge_dir + "/discordmessage.txt", std::ios::trunc);
        if (disc_file.is_open()) disc_file << "\n";
    }

    std::cout << "[bridge] FIFO pipe support: " << (fifo_available ? "yes" : "no (vanilla SRB2)")
              << std::endl;

    if (bridge_channel_id != "0") {
        size_t seek_start = 0;
        bool dbot_synced = false;
        int dbot_sync_retries = 0;
        constexpr int DBOT_SYNC_MAX_RETRIES = 15;
        dpp::snowflake bridge_channel_sf = std::stoull(bridge_channel_id);

        bot.start_timer([&bot, &registry, messages_path, &seek_start, &dbot_synced,
                         &dbot_sync_retries, bridge_channel_sf, &guild_emojis,
                         fifo_available](dpp::timer) {
            if (fifo_available && !dbot_synced && dbot_sync_retries < DBOT_SYNC_MAX_RETRIES) {
                ++dbot_sync_retries;
                dbot_synced = pipe_srb2_server_do("dbot_sync");
#ifndef NDEBUG
                if (!dbot_synced) {
                    std::cout << "[DEBUG] bridge: retrying dbot_sync ("
                              << dbot_sync_retries << "/" << DBOT_SYNC_MAX_RETRIES << ")"
                              << std::endl;
                } else {
                    pipe_srb2_server_do("dbot_debug on");
                }
#endif
            }

            size_t seek_end = bridge_get_lines(messages_path);
            if (seek_end > seek_start) {
                std::string content = bridge_read_range(messages_path, seek_start, seek_end);
                if (content.size() > 1) {
                    content = bridge_replace_emojis(content, guild_emojis);
                    std::istringstream lines(content);
                    std::string line;
                    std::vector<dpp::embed> pending_embeds;

                    while (std::getline(lines, line)) {
                        if (line.empty()) continue;
                        if (auto event = bridge_parse_event(line)) {
                            auto embed_opt = registry.handle_bridge_event(*event);
                            if (embed_opt.has_value()) {
                                auto attach = registry.get_bridge_attachment(*event);
                                if (attach.has_value()) {
                                    // Flush pending, then send thumbnailed embed individually
                                    if (!pending_embeds.empty()) {
                                        dpp::message evt_batch(bridge_channel_sf, "");
                                        for (auto& e : pending_embeds) evt_batch.add_embed(e);
                                        bot.message_create(evt_batch, [](const dpp::confirmation_callback_t& cb) {
                                            if (cb.is_error()) std::cout << "[bridge] message_create error: " << cb.get_error().human_readable << std::endl;
                                        });
                                        pending_embeds.clear();
                                    }
                                    embed_opt->set_image("attachment://" + attach->first);
                                    dpp::message thumb_msg(bridge_channel_sf, "");
                                    thumb_msg.add_embed(*embed_opt);
                                    thumb_msg.add_file(attach->first, attach->second);
                                    bot.message_create(thumb_msg, [](const dpp::confirmation_callback_t& cb) {
                                        if (cb.is_error()) std::cout << "[bridge] message_create error: " << cb.get_error().human_readable << std::endl;
                                    });
                                } else {
                                    pending_embeds.push_back(*embed_opt);
                                }
                            }
                            // No fallback — if no module produced an embed, the event is silently skipped
                            if (pending_embeds.size() >= 10) {
                                dpp::message evt_batch(bridge_channel_sf, "");
                                for (auto& e : pending_embeds) evt_batch.add_embed(e);
                                bot.message_create(evt_batch, [](const dpp::confirmation_callback_t& cb) {
                                    if (cb.is_error()) std::cout << "[bridge] message_create error: " << cb.get_error().human_readable << std::endl;
                                });
                                pending_embeds.clear();
                            }
                        }
                        // Lines not matching [EVENT:...] are silently ignored
                    }
                    if (!pending_embeds.empty()) {
                        dpp::message evt_batch(bridge_channel_sf, "");
                        for (auto& e : pending_embeds) evt_batch.add_embed(e);
                        bot.message_create(evt_batch, [](const dpp::confirmation_callback_t& cb) {
                            if (cb.is_error()) std::cout << "[bridge] message_create error: " << cb.get_error().human_readable << std::endl;
                        });
                    }
                }
                seek_start = seek_end;
            }
        }, 2);
    }

    bot.start(dpp::st_wait);
}
