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

    dpp::cluster bot(bot_token, dpp::i_default_intents | dpp::i_message_content);
    bot.on_log(dpp::utility::cout_logger());

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

    // ── Guild emoji loader ──
    std::unordered_map<std::string, std::string> guild_emojis;
    bot.on_ready([&bot, guild_id, &guild_emojis](const dpp::ready_t&) {
        auto guild = dpp::find_guild(guild_id);
        if (guild) {
            for (const auto& emoji_id : guild->emojis) {
                auto emoji = dpp::find_emoji(emoji_id);
                if (emoji) {
                    guild_emojis[emoji->name] = std::to_string(emoji_id);
                }
            }
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
                    std::string chat_lines;
                    std::vector<dpp::embed> pending_embeds;

                    while (std::getline(lines, line)) {
                        if (line.empty()) continue;
                        auto event = bridge_parse_event(line);
                        if (event) {
                            if (!chat_lines.empty()) {
                                dpp::message chat_msg(bridge_channel_sf, chat_lines);
                                chat_msg.set_allowed_mentions(false, false, false, false, {});
                                bot.message_create(chat_msg);
                                chat_lines.clear();
                            }
                            auto embed_opt = registry.handle_bridge_event(*event);
                            if (embed_opt.has_value()) {
                                pending_embeds.push_back(*embed_opt);
                            } else if (event->type == "MAP_CHANGE") {
                                // Old-format event; skip or handle minimally
                            } else {
                                dpp::embed fallback;
                                fallback.set_title(event->type);
                                fallback.set_description(line);
                                fallback.set_color(0x2F3136);
                                pending_embeds.push_back(fallback);
                            }
                            if (pending_embeds.size() >= 10) {
                                dpp::message evt_batch(bridge_channel_sf, "");
                                for (auto& e : pending_embeds) evt_batch.add_embed(e);
                                bot.message_create(evt_batch);
                                pending_embeds.clear();
                            }
                        } else {
                            chat_lines += line;
                            chat_lines += '\n';
                        }
                    }
                    if (!chat_lines.empty()) {
                        dpp::message chat_msg(bridge_channel_sf, chat_lines);
                        chat_msg.set_allowed_mentions(false, false, false, false, {});
                        bot.message_create(chat_msg);
                    }
                    if (!pending_embeds.empty()) {
                        dpp::message evt_batch(bridge_channel_sf, "");
                        for (auto& e : pending_embeds) evt_batch.add_embed(e);
                        bot.message_create(evt_batch);
                    }
                }
                seek_start = seek_end;
                {
                    std::ofstream trunc(messages_path, std::ios::trunc);
                    trunc.close();
                }
                seek_start = 0;
            }
        }, 2);
    }

    bot.start(dpp::st_wait);
}
