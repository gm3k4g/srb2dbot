#include <dpp/dpp.h>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <ctime>
#include <unistd.h>
#include <algorithm>
#include <csignal>
#include <streambuf>

#include "version.h"
#include "srb2dbot/utils.hpp"
#include "srb2dbot/script.hpp"
#include "srb2dbot/bridge.hpp"
#include "srb2dbot/module.hpp"
#include "srb2dbot/server.hpp"
using json = nlohmann::json;

const auto PERMS = dpp::p_ban_members;

// TeeBuf: writes to both stdout and a log file simultaneously
static std::ofstream g_log_file;
static volatile sig_atomic_t g_shutdown_requested = 0;
class TeeBuf : public std::streambuf {
    std::streambuf* cout_buf_;
public:
    explicit TeeBuf(std::streambuf* cout_buf) : cout_buf_(cout_buf) {}
protected:
    int overflow(int c) override {
        if (c == EOF) return EOF;
        if (cout_buf_->sputc(c) == EOF) return EOF;
        if (g_log_file.is_open()) g_log_file.put(c);
        return c;
    }
    int sync() override {
        cout_buf_->pubsync();
        if (g_log_file.is_open()) g_log_file.flush();
        return 0;
    }
};

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

    // Override SRB2 home directory from modules.json if configured
    {
        std::ifstream mod_file("modules.json");
        if (!mod_file.is_open()) {
            std::cerr << "ERROR: modules.json not found\n";
            return EXIT_FAILURE;
        }
        try {
            auto mod_cfg = json::parse(mod_file);
            if (mod_cfg.contains("srb2_home")) {
                auto& sh = mod_cfg["srb2_home"];
                if (sh.is_object() && sh.value("enabled", false) && sh.contains("path") && sh["path"].is_string()) {
                    std::string home = sh["path"].get<std::string>();
                    // Expand leading ~ to $HOME
                    if (!home.empty() && home[0] == '~') {
                        const char* home_env = std::getenv("HOME");
                        home = (home_env ? home_env : "/tmp") + home.substr(1);
                    }
                    setenv("SRB2DBOT_SRB2_HOME", home.c_str(), 1);
                }
            }
        } catch (...) {}
    }

    std::string srb2_dir = dir_srb2_str();
    std::filesystem::create_directories(srb2_dir + "/srb2_servers.d/srb2b.d");
    std::filesystem::create_directories(srb2_dir + "/addons");

    // Detect bot's own directory via /proc/self/exe (Linux)
    std::string bot_dir = std::filesystem::canonical("/proc/self/exe").parent_path().string();
    std::filesystem::create_directories(bot_dir + "/logs");
    std::filesystem::create_directories(bot_dir + "/thumbnails");

    std::cout << "[init] SRB2 data dir: " << srb2_dir << std::endl;
    std::cout << "[init] Bot data dir:  " << bot_dir << std::endl;

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
    std::string log_dir = bot_dir + "/logs";
    std::filesystem::create_directories(log_dir);
    std::time_t now = std::time(nullptr);
    char ts[64];
    std::strftime(ts, sizeof(ts), "%Y-%m-%d_%H-%M-%S", std::localtime(&now));
    std::string log_name = log_dir + "/" + ts + ".txt";
    g_log_file.open(log_name, std::ios::app);
    if (g_log_file.is_open()) {
        std::string symlink_name = bot_dir + "/latest-logs-srb2dbot.txt";
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
    static TeeBuf teebuf(std::cout.rdbuf());
    std::cout.rdbuf(&teebuf);
    // ── Single-instance lock ──
    std::string pid_path = bot_dir + "/srb2dbot.pid";
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
        std::string path = std::filesystem::canonical("/proc/self/exe").parent_path().string() + "/srb2dbot.pid";
        std::filesystem::remove(path);
    });
    signal(SIGINT,  [](int) { g_shutdown_requested = 1; });
    signal(SIGTERM, [](int) { g_shutdown_requested = 1; });
    signal(SIGQUIT, [](int) { g_shutdown_requested = 1; });

    dpp::cluster bot(bot_token, dpp::i_default_intents | dpp::i_message_content | dpp::i_guild_emojis);
    bot.on_log([](const dpp::log_t& event) {
#ifndef NDEBUG
        if (event.message.find("\"op\":1") != std::string::npos) return;
#else
        if (event.severity < dpp::ll_info) return;
#endif
        std::cout << event.message << std::endl;
    });

    const bool fifo_available = detect_fifo_support();

    // Initialize Module Registry with all context
    ModuleRegistry registry;
    if (!registry.load_from_config("modules.json", {
        bot, fifo_available, bridge_channel_id, bot_id, srb2_dir, bot_dir
    })) {
        return EXIT_FAILURE;
    }

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
        // Fetch guild via REST API (cache may not be populated at on_ready time)
        bot.guild_get(guild_id, [&guild_emojis](const dpp::confirmation_callback_t& cb) {
            if (cb.is_error()) {
                std::cout << "[ready] Failed to fetch guild: " << cb.get_error().human_readable << std::endl;
                return;
            }
            auto guild = std::get<dpp::guild>(cb.value);
            std::cout << "[ready] Guild fetched: " << guild.name << " (" << guild.id << "), "
                      << guild.emojis.size() << " emojis" << std::endl;
            for (const auto& emoji_id : guild.emojis) {
                auto emoji = dpp::find_emoji(emoji_id);
                if (emoji) {
                    guild_emojis[emoji->name] = std::to_string(emoji_id);
                }
            }
            std::cout << "[ready] Guild emojis loaded: " << guild_emojis.size();
            if (!guild_emojis.empty()) {
                std::cout << " (";
                bool first = true;
                for (const auto& [name, id] : guild_emojis) {
                    if (!first) std::cout << ", ";
                    std::cout << name << "=" << id;
                    first = false;
                }
                std::cout << ")";
            }
            std::cout << std::endl;
        });
        if (dpp::run_once<struct notify_modules_ready>()) {
            dpp::snowflake ch = bridge_channel_id != "0" ? std::stoull(bridge_channel_id) : 0;
            if (ch != 0) registry.on_ready(bot, ch);
            std::cout << "[ready] Bot online" << std::endl;
        }
    });

    // ── Bridge setup ──
    std::string bridge_dir = srb2_dir + "/luafiles/client/DiscordBot";
    std::filesystem::create_directories(bridge_dir);
    std::string messages_path = bridge_dir + "/Messages.txt";
    {
        std::ofstream msg_file(messages_path, std::ios::trunc);
    }
    {
        std::ofstream disc_file(bridge_dir + "/discordmessage.txt", std::ios::trunc);
    }
    {
        std::ofstream con_file(bridge_dir + "/console.txt", std::ios::trunc);
    }

    // Read auto_pause from modules.json and write to console.txt for the Lua script
    {
        std::ifstream mod_config("modules.json");
        if (mod_config.is_open()) {
            try {
                auto mod_data = json::parse(mod_config);
                bool auto_pause = true;
                auto& mods = mod_data["modules"]["auto"];
                if (mods.contains("auto_pause")) {
                    auto& entry = mods["auto_pause"];
                    if (entry.is_object())
                        auto_pause = entry.value("enabled", true);
                    else
                        auto_pause = entry.get<bool>();
                }
                std::ofstream con_file(bridge_dir + "/console.txt", std::ios::app);
                if (con_file.is_open())
                    con_file << "dbot_autopause " << (auto_pause ? "1" : "0") << "\n";
            } catch (const json::parse_error&) {}
        }
    }

#ifndef NDEBUG
    std::cout << "[bridge] FIFO pipe support: " << (fifo_available ? "yes" : "no (vanilla SRB2)")
              << std::endl;
#endif

    if (bridge_channel_id != "0") {
        std::cout << "[bridge] channel " << bridge_channel_id << " — polling every 2s" << std::endl;
        bool dbot_synced = false;
        int dbot_sync_retries = 0;
        constexpr int DBOT_SYNC_MAX_RETRIES = 15;
        dpp::snowflake bridge_channel_sf = std::stoull(bridge_channel_id);

        bot.start_timer([&bot, &registry, messages_path, &dbot_synced,
                         &dbot_sync_retries, bridge_channel_sf, &guild_emojis,

                         fifo_available](dpp::timer) {
            if (g_shutdown_requested) {
                static bool shutdown_sent = false;
                if (!shutdown_sent) {
                    shutdown_sent = true;
                    registry.on_shutdown(bot, bridge_channel_sf);
                }
                return;
            }
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

            // Atomically rename Messages.txt to .tmp so Lua's write/close
            // completes before we read.  rename(2) is POSIX-atomic on the
            // same filesystem; Lua's io.openlocal("a+") creates a fresh
            // Messages.txt for the next write.
            // After reading, move .tmp to an archive dir for inspection.
            std::string tmp_path = messages_path + ".tmp";
            if (std::rename(messages_path.c_str(), tmp_path.c_str()) == 0) {
                std::ifstream tmp_file(tmp_path);
                if (tmp_file.is_open()) {
                    std::string content((std::istreambuf_iterator<char>(tmp_file)),
                                         std::istreambuf_iterator<char>());
                    tmp_file.close();
                    if (!content.empty()) {
#ifndef NDEBUG
                        std::cout << "[bridge] raw content from .tmp (" << content.size() << " bytes):" << std::endl;
                        std::istringstream raw_lines(content);
                        std::string raw_line;
                        while (std::getline(raw_lines, raw_line)) {
                            if (!raw_line.empty()) std::cout << "[bridge]   line: " << raw_line << std::endl;
                        }
#endif
                    }
                    // Archive for inspection instead of deleting
                    {
                        std::string archive_dir = (std::filesystem::path(messages_path).parent_path() / "archive").string();
                        std::filesystem::create_directories(archive_dir);
                        auto now = std::time(nullptr);
                        char ts[32];
                        std::strftime(ts, sizeof(ts), "%H%M%S", std::localtime(&now));
                        std::rename(tmp_path.c_str(), (archive_dir + "/" + ts + ".tmp").c_str());
                    }
                if (content.size() > 1) {
                    content = bridge_replace_emojis(content, guild_emojis);
                    std::istringstream lines(content);
                    std::string line;
                    std::vector<dpp::embed> pending_embeds;
                    std::string chat_text;

                    while (std::getline(lines, line)) {
                        if (line.empty()) continue;
                        if (auto event = bridge_parse_event(line)) {
                            auto embed_opt = registry.handle_bridge_event(*event);
                            if (embed_opt.has_value()) {
                                {
                                    std::string title = bridge_replace_emojis(embed_opt->title, guild_emojis);
                                    embed_opt->set_title(title);
                                }
#ifndef NDEBUG
                                std::cout << "[bridge] SRB2→Discord: " << event->type << " raw=" << line << std::endl;
#endif
                                auto attach = registry.get_bridge_attachment(*event);
                                if (attach.has_value()) {
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
                            } else if (event->type == "CHAT" && event->fields.size() >= 4) {
                                // chat_card disabled — format as plain text
                                auto& f = event->fields;
                                std::string node = f[0];
                                std::string name = f[1];
                                std::string msg  = f[2];
                                for (size_t i = 3; i + 4 < f.size(); i++) msg += "|" + f[i];
                                std::string skin = f.size() >= 6 ? f[f.size() - 4] : "";
                                skin = sanitize_for_discord(skin);
                                name = sanitize_for_discord(name);
                                msg  = sanitize_for_discord(msg);
                                if (!chat_text.empty()) chat_text += "\n";
                                chat_text += node + " :" + skin + ": <" + name + "> " + msg;
                            }
                            if (pending_embeds.size() >= 10) {
                                dpp::message evt_batch(bridge_channel_sf, "");
                                for (auto& e : pending_embeds) evt_batch.add_embed(e);
                                bot.message_create(evt_batch, [](const dpp::confirmation_callback_t& cb) {
                                    if (cb.is_error()) std::cout << "[bridge] message_create error: " << cb.get_error().human_readable << std::endl;
                                });
                                pending_embeds.clear();
                            }
}
                    }
                    if (!chat_text.empty()) {
                        dpp::message txt(bridge_channel_sf, chat_text);
                        bot.message_create(txt, [](const dpp::confirmation_callback_t& cb) {
                            if (cb.is_error()) std::cout << "[bridge] message_create error: " << cb.get_error().human_readable << std::endl;
                        });
                    }
                    if (!pending_embeds.empty()) {
                        dpp::message evt_batch(bridge_channel_sf, "");
                        for (auto& e : pending_embeds) evt_batch.add_embed(e);
                        bot.message_create(evt_batch, [](const dpp::confirmation_callback_t& cb) {
                            if (cb.is_error()) std::cout << "[bridge] message_create error: " << cb.get_error().human_readable << std::endl;
                        });
                    }
                }
                } else {
                    std::remove(tmp_path.c_str());
                }
            }
        }, 2);
    } else {
        std::cout << "[bridge] disabled (no channel_id in secret.json)" << std::endl;
    }

    bot.start(dpp::st_wait);
}
