// TODO: Could be a good idea to make it so that
// instead of showing ```Succesfully blah blah```
// you use discord's built-ins such as cards or embeds
// or whatever to display the information in a more
// formatted way.
// TODO: Make admin commands invisible to anyone but you
// (ephemeral replies?)
// TODO: Reading chat logs and printing them in channel
// TODO: Game stats command

#include <dpp/appcommand.h>
#include <dpp/dpp.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <dpp/message.h>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <cerrno>
#include <filesystem>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <vector>

#include "version.h"
#include "srb2dbot/utils.hpp"
#include "srb2dbot/script.hpp"
#include "srb2dbot/bridge.hpp"
using json = nlohmann::json;

const std::array<std::string, 2>INVALID_ARRAY = {"Invalid filename", "Invalid content"};

const size_t MAX_WAD_SIZE = 100ULL * 1024 * 1024;

namespace {
    auto dir_srb2_str() -> std::string {
        struct passwd *pw = getpwuid(getuid());
        std::string homedir = pw->pw_dir;

        std::string script_path = homedir;
        std::string srb2_home = "/.srb2"; // TODO: Look at -h parameter (to look at a different srb2 directory)
        script_path.append(srb2_home);
        return script_path;
    }

    auto wads_get_str() -> std::string {
        std::string wads_dir = dir_srb2_str();
        wads_dir.append("/addons");

        std::stringstream wads_list;
        wads_list << "Showing all files in " << wads_dir << ":\n\n";
        int i = 1;
        for (const auto& entry : std::filesystem::directory_iterator(wads_dir)) {
            wads_list << i << " " << entry.path().filename() << "\n";
            i+=1;
        }

        return wads_list.str();
    }

    auto wads_search_str(std::string target_string) -> std::string {
        std::string wads_dir = dir_srb2_str();
        wads_dir.append("/addons");

        std::stringstream wads_list;
        wads_list << "Showing all matches in " << wads_dir << ":\n\n";
        int i = 1;
        for (const auto& entry : std::filesystem::directory_iterator(wads_dir)) {
            std::string line = entry.path().filename();
            if (line.find(target_string) != std::string::npos) {
                wads_list << i << " " << entry.path().filename() << "\n";
                i += 1;
            }
        }

        return wads_list.str();
    }

    // TODO: Maybe clearing the pipe is unnecessary now? (fixed garbage characters)
    // TODO: Pipe name will be taken by config name
    // e.g. "srb2b" -- "srb2b.fifo"
    auto pipe_get() -> std::string {
        std::string srb2_dir = dir_srb2_str();

        // TODO: Get srb2 config dir based on -c argument -- e.g. "srb2_custom.d"
        std::string subscript_dir   = "/srb2b.d";
        // TODO: Get srb2 pipe name based on -c argument -- e.g. "srb2_custom.fifo"
        std::string srb2_fifo       = "/srb2b.fifo";

        std::string pipe_dir = srb2_dir
            .append("/srb2_servers.d")
            .append(subscript_dir)
            .append(srb2_fifo);
        return std::string(pipe_dir);
    }

    auto pipe_write(const std::string& data) -> bool {
        std::string fifo = pipe_get();

        int fd = open(fifo.c_str(), O_WRONLY|O_NONBLOCK);
        if (fd == -1) {
            perror("open");
            return false;
        }

        const char* buf = data.c_str();
        size_t remaining = data.size();
        while (remaining > 0) {
            ssize_t bytes_written = write(fd, buf, remaining);
            if (bytes_written == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    usleep(1000);
                    continue;
                }
                perror("write");
                close(fd);
                return false;
            }
            buf += bytes_written;
            remaining -= static_cast<size_t>(bytes_written);
        }

        close(fd);
        return true;
    }

    auto pipe_srb2_server_do(const std::string &data) -> bool {
        for (char c : data) {
            if (c == '\n' || c == '\r' || (c >= 0 && c < 0x20 && c != '\t')) {
                return false;
            }
        }
        return pipe_write(data);
    }

    auto pipe_srb2_server_say(const std::string &msg) -> bool {
        for (char c : msg) {
            if (c == '\n' || c == '\r' || (c >= 0 && c < 0x20 && c != '\t')) {
                return false;
            }
        }
        std::string cmd = "say " + msg;
        bool success = pipe_write(cmd);
        return success;
    }

    auto pipe_srb2_kick_player(const std::string &player) -> bool {
        for (char c : player) {
            if (c == '\n' || c == '\r' || (c >= 0 && c < 0x20 && c != '\t')) {
                return false;
            }
        }
        std::string cmd = "kick " + player;
        bool success = pipe_write(cmd);
        return success;
    }

    auto pipe_srb2_ban_player(const std::string &player) -> bool {
        for (char c : player) {
            if (c == '\n' || c == '\r' || (c >= 0 && c < 0x20 && c != '\t')) {
                return false;
            }
        }
        std::string cmd = "ban " + player;
        bool success = pipe_write(cmd);
        return success;
    }

    // TODO: Separate into script_get_path and script_get_str
    // returns: [0]: Script name [1]: Script absolute path
    auto script_get(std::ifstream& script_file) -> std::array<std::string, 2> {
        std::string script_path = dir_srb2_str();

        // TODO: Look at -c parameter (to look at a different script file name)
        std::string subscript_dir   = "/srb2b.d";
        std::string script_name     = "/srb2b.sh";

        script_path.append("/srb2_servers.d")
            .append(subscript_dir)
            .append(script_name);
        script_file.open(script_path);
        if (!script_file.is_open()) {
            return INVALID_ARRAY;
        }

        // TODO: maybe script_name becomes a globally accessible variable (globally within int main)
        return {script_name, script_path};
    }

    // TODO: include other details to show in message, such as absolute path?
    // Afterwards append these
    auto script_get_str() -> std::array<std::string, 2> {
        std::ifstream script_file;
        auto script_content = script_get(script_file);
        if (!script_file) {
            return INVALID_ARRAY;
        }

        std::stringstream input_buffer;
        std::stringstream output_buffer;
        std::string line;
        input_buffer << script_file.rdbuf();

        while(std::getline(input_buffer, line)) {
            if (line.find("password") != std::string::npos) {
                output_buffer << "[REDACTED]\n";
            } else {
                output_buffer << line << "\n";
            }

        }

        std::array<std::string, 2> script = {script_content[0], output_buffer.str()};
        return script;
    }

    // TODO: Pass arguments for case insensitive/sensitive search
    auto script_find(const std::string &target_string) -> std::string {
        std::ifstream script_file;
        auto script_content = script_get(script_file);
        if (!script_file) {
            return std::string();
        }
        std::stringstream buf;
        buf << script_file.rdbuf();
        return script_find_str(buf.str(), target_string);
    }

    // TODO: Validate bash command as string before writing it
    auto script_validate() -> bool {
        std::ifstream script_buf;
        auto script_content = script_get(script_buf);

        std::string script_path = script_content[1];

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return false;
        }
        if (pid == 0) {
            execlp("bash", "bash", "-n", script_path.c_str(), (char*)nullptr);
            _exit(EXIT_FAILURE);
        }
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }

    auto script_write(std::string script_name, std::string script_contents) -> bool {
        std::ofstream script_file(script_name, std::ios::trunc);
        if (!script_file) {
            return false;
        }

        script_file << script_contents;

        return true;
    }

    auto script_inspect(std::string& script_content, int target_line) -> std::string {
        return script_inspect_str(script_content, target_line);
    }

    auto script_add_line(std::string& line_content, int line_number) -> bool {
        std::ifstream script_file;
        auto script_content = script_get(script_file);
        std::stringstream old_script;
        old_script << script_file.rdbuf();
        std::string result = script_add_line_str(old_script.str(), line_content, line_number);
        return script_write(script_content[1], result);
    }

    // TODO: Show old line and new lines
    auto script_change_line(std::string& line_content, int line_number) -> bool {
        std::ifstream script_file;
        auto script_content = script_get(script_file);
        std::stringstream old_script;
        old_script << script_file.rdbuf();
        std::string result = script_change_line_str(old_script.str(), line_content, line_number);
        return script_write(script_content[1], result);
    }

    auto script_move_line(int old_line, int new_line) -> bool {
        std::ifstream script_file;
        auto script_content = script_get(script_file);
        std::stringstream old_script;
        old_script << script_file.rdbuf();
        std::string result = script_move_line_str(old_script.str(), old_line, new_line);
        return script_write(script_content[1], result);
    }

    // TODO: Show line being removed
    // TODO: parameter that lets you keep an empty line
    // instead of removing the line?
    auto script_remove_line(int line_num) -> bool {
        std::ifstream script_file;
        auto script_content = script_get(script_file);
        std::stringstream old_script;
        old_script << script_file.rdbuf();
        std::string result = script_remove_line_str(old_script.str(), line_num);
        return script_write(script_content[1], result);
    }

} // namespace

// Permissions
const auto PERMS = dpp::p_ban_members; //dpp::p_administrator

int main() {
    // Read secret
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
    auto guild_id = std::stol(guild_id_str);

    std::string bot_token = data["bot_token"].get<std::string>();

    std::string bot_id = "0";
    if (data.contains("bot_id") && data["bot_id"].is_string()) {
        bot_id = data["bot_id"].get<std::string>();
    }

    std::string bridge_channel_id = "0";
    if (data.contains("channel_id") && data["channel_id"].is_string()) {
        bridge_channel_id = data["channel_id"].get<std::string>();
    }

    std::string service_name = "srb2@srb2b";
    if (data.contains("service_name") && data["service_name"].is_string()) {
        service_name = data["service_name"].get<std::string>();
    }

    dpp::cluster bot(bot_token, dpp::i_default_intents | dpp::i_message_content);

    bot.on_log(dpp::utility::cout_logger());

    bot.on_slashcommand([&bot, &service_name](const dpp::slashcommand_t& event) {
        //std::cout << "Received slash command: " << event.command.get_command_name() << "\n";

        // Gatekeep anyone who potentially tries to run commands without perms.
   	    dpp::permission perms = event.command.get_resolved_permission(event.command.usr.id);
        if (!perms.can(PERMS)) {
            dpp::message msg(event.command.channel_id, "```Missing permissions```");
            event.reply(msg.set_flags(dpp::m_ephemeral));
            return;
       }


        // Gets the script that runs the SRB2 server and shows it in Discord.
        // Optionally provide an argument to make a search for specific words.
        // Searched words will show 10 above and 10 below results.

        // Example:
        // /get_script "map.pk3"
        if (event.command.get_command_name() == CMD_GET_SCRIPT) {
            auto script_contents = script_get_str();
            dpp::message msg(event.command.channel_id, "");
            msg.add_file(script_contents[0], script_contents[1]);
            event.reply(msg.set_flags(dpp::m_ephemeral));
        }

        else if (event.command.get_command_name() == CMD_SEARCH_WADS) {
            std::string target_filename = std::get<std::string>(event.get_parameter("target_filename"));
            std::string wad_list = wads_search_str(target_filename);

            dpp::message msg(event.command.channel_id, "");
            msg.add_file("wads_search.txt", wad_list);
            event.reply(msg.set_flags(dpp::m_ephemeral));
        }

        else if (event.command.get_command_name() == CMD_LIST_WADS) {
            std::string wad_list = wads_get_str();

            dpp::message msg(event.command.channel_id, "");
            msg.add_file("wads_list.txt", wad_list);
            event.reply(msg.set_flags(dpp::m_ephemeral));
        }

        // TODO: Accept zip files. Automatically extracts them to the wad directory.
        // Files can be either URLs or uploaded files.
        else if (event.command.get_command_name() == CMD_ADDFILE_UPLOAD) {
            auto wad_file   = event.get_parameter("file");

            if (std::holds_alternative<dpp::snowflake>(wad_file)) {
                dpp::snowflake attachment_id = std::get<dpp::snowflake>(wad_file);

                // Find the attachment object
                auto attachment = event.command.resolved.attachments.find(attachment_id);
                if (attachment != event.command.resolved.attachments.end()) {
                    std::string url = attachment->second.url;
                    std::string filename = sanitize_filename(attachment->second.filename);

                    bot.request(url, dpp::m_get, [&bot, event, filename](const dpp::http_request_completion_t& resp) {
                        if (resp.status == 200) {
                            if (resp.body.size() > MAX_WAD_SIZE) {
                                std::string msg_str = "```File " + filename + " exceeds max size of 100MB.```\n";
                                bot.interaction_response_create(
                                    event.command.id,
                                    event.command.token,
                                    dpp::interaction_response(
                                        dpp::ir_channel_message_with_source,
                                        dpp::message(msg_str).set_flags(dpp::m_ephemeral)
                                    )
                                );
                                return;
                            }
                            std::string content = resp.body;
                            std::string wad_dir = dir_srb2_str();
                            wad_dir.append("/addons/").append(filename);

                            std::ofstream buffer(wad_dir);
                            buffer << content;

                            std::string msg_str = "```Successfully uploaded attached file " + filename + " to the server.```\n";
                            bot.interaction_response_create(
                                event.command.id,
                                event.command.token,
                                dpp::interaction_response(
                                    dpp::ir_channel_message_with_source,
                                    dpp::message(msg_str).set_flags(dpp::m_ephemeral)
                                )
                            );
                        } else {
                            std::string msg_str = "```Failed to upload the attached file " + filename + " .```\n";
                            bot.interaction_response_create(
                                event.command.id,
                                event.command.token,
                                dpp::interaction_response(
                                    dpp::ir_channel_message_with_source,
                                    dpp::message(msg_str).set_flags(dpp::m_ephemeral)
                                )
                            );
                        }
                    });
                }
            }
        }

        else if (event.command.get_command_name() == CMD_ADDFILE_LINK) {
            auto wad_url    = std::get<std::string>(event.get_parameter("file_url"));
            std::string filename = link_filename_str(wad_url);

            // Download the file from Discord CDN
            bot.request(wad_url, dpp::m_get, [&bot, event, filename](const dpp::http_request_completion_t& resp) {
                if (resp.status == 200) {
                    if (resp.body.size() > MAX_WAD_SIZE) {
                        std::string msg_str = "```File " + filename + " exceeds max size of 100MB.```\n";
                        bot.interaction_response_create(
                            event.command.id,
                            event.command.token,
                            dpp::interaction_response(
                                dpp::ir_channel_message_with_source,
                                dpp::message(msg_str).set_flags(dpp::m_ephemeral)
                            )
                        );
                        return;
                    }
                    std::string content = resp.body;
                    std::string wad_dir = dir_srb2_str();
                    wad_dir.append("/addons/").append(filename);

                    std::ofstream buffer(wad_dir);
                    buffer << content;

                    std::string msg_str = "```Successfully uploaded file "  + filename + " from URL to the server.```\n";
                    bot.interaction_response_create(
                        event.command.id,
                        event.command.token,
                        dpp::interaction_response(
                            dpp::ir_channel_message_with_source,
                            dpp::message(msg_str).set_flags(dpp::m_ephemeral)
                        )
                    );
                } else {
                    std::string msg_str = "```Failed to upload file " + filename + " from URL.```\n";
                    bot.interaction_response_create(
                        event.command.id,
                        event.command.token,
                        dpp::interaction_response(
                            dpp::ir_channel_message_with_source,
                            dpp::message(msg_str).set_flags(dpp::m_ephemeral)
                        )
                    );
                }
            });
        }

        else if (event.command.get_command_name() == CMD_INSERT_LINE) {

            int64_t line_num = std::get<int64_t>(event.get_parameter("line_num"));
            std::string inserted_line;
            if (event.get_parameter("line_contents").index() != 0) {
                inserted_line = std::get<std::string>(event.get_parameter("line_contents"));
            }

            bool script_added_line = script_add_line(inserted_line, line_num);
            std::stringstream result;
            if (!script_added_line) {
                result << "```Couldn't add line!```";
            } else {
                auto script_contents = script_get_str();
                auto inspect = script_inspect(script_contents[1], line_num);
                result  << "```# Successfully added to line number " << line_num << "!```\n"
                        << inspect;
            }
            dpp::message msg(event.command.channel_id, result.str());
            event.reply(msg.set_flags(dpp::m_ephemeral));
        }
        else if (event.command.get_command_name() == CMD_REMOVE_LINE) {
            int64_t line_num = std::get<int64_t>(event.get_parameter("line_num"));

            bool script_removed_line = script_remove_line(line_num);
            std::stringstream result;
            if (!script_removed_line) {
                result << "```Couldn't remove line!```";
            } else {
                auto script_contents = script_get_str();
                auto inspect = script_inspect(script_contents[1], line_num);
                result  << "```# Successfully removed line number " << line_num << "!```\n"
                        << inspect;
            }
            dpp::message msg(event.command.channel_id, result.str());
            event.reply(msg.set_flags(dpp::m_ephemeral));
        }

        else if (event.command.get_command_name() == CMD_CHANGE_LINE) {
            int64_t line_num = std::get<int64_t>(event.get_parameter("line_num"));

            std::string changed_line;
            if (event.get_parameter("line_contents").index() != 0) {
                changed_line = std::get<std::string>(event.get_parameter("line_contents"));
            }

            bool script_changed_line = script_change_line(changed_line, line_num);
            std::stringstream result;
            if (!script_changed_line) {
                result << "```Couldn't change line!```";
            } else {
                auto script_contents = script_get_str();
                auto inspect = script_inspect(script_contents[1], line_num);
                result  << "```# Successfully changed line at line number " << line_num << "!```\n"
                        << inspect;
            }
            dpp::message msg(event.command.channel_id, result.str());
            event.reply(msg.set_flags(dpp::m_ephemeral));

        }

        else if (event.command.get_command_name() == CMD_MOVE_LINE) {
            int64_t old_line = std::get<int64_t>(event.get_parameter("old_line_num"));
            int64_t new_line = std::get<int64_t>(event.get_parameter("new_line_num"));

            bool script_moved_line = script_move_line(old_line, new_line);
            std::stringstream result;
            if (!script_moved_line) {
                result << "```Couldn't move line!```";
            } else {
                auto script_contents = script_get_str();
                auto inspect = script_inspect(script_contents[1], new_line);
                result  << "```# Successfully moved line from " << old_line << " to " << new_line << "!```\n"
                        << inspect;
            }
            dpp::message msg(event.command.channel_id, result.str());
            event.reply(msg.set_flags(dpp::m_ephemeral));
        }

        else if (event.command.get_command_name() == CMD_FIND_LINE) {
            std::string target_string = std::get<std::string>(event.get_parameter("target_string"));
            std::string found_results = script_find(target_string);

            if (found_results.empty()) {
                found_results = "```No matches were found!```";
            } else {
                found_results = "```"+found_results+"```";
            }
            dpp::message msg(event.command.channel_id, found_results);
            event.reply(msg.set_flags(dpp::m_ephemeral));
        }

        // Inspects a line. This will show the targeted line along with line numbers,
        // as well as 10 lines above and beneath the targeted line.
        else if (event.command.get_command_name() == CMD_INSPECT_LINE) {
            int64_t line_num = std::get<int64_t>(event.get_parameter("line_num"));

            auto script_contents = script_get_str();
            std::string script_lines = script_inspect(script_contents[1], line_num);
            dpp::message msg(event.command.channel_id, script_lines);
            event.reply(msg.set_flags(dpp::m_ephemeral));
       }

       // Do something as server.
       else if (event.command.get_command_name() == CMD_SERVER_DO) {
           std::string serv_command = std::get<std::string>(event.get_parameter("server_command"));
           bool success = pipe_srb2_server_do(serv_command);
           std::string result;
           if (!success) {
               result = "```Failed to execute command```";
           } else {
               result = "```Success```";
           }
           dpp::message msg(event.command.channel_id, result);
           event.reply(msg.set_flags(dpp::m_ephemeral));
       }

       // Says something as the server.
        else if (event.command.get_command_name() == CMD_SERVER_SAY) {
           std::string serv_msg = std::get<std::string>(event.get_parameter("server_message"));
           bool success = pipe_srb2_server_say(serv_msg);
           std::string result;
           if (!success) {
               result = "```Failed to say message.```";
           } else {
               result = "```Success```";
           }
           dpp::message msg(event.command.channel_id, result);
           event.reply(msg.set_flags(dpp::m_ephemeral));
       }

            // TODO: find a more concrete way of knowing whether
            // the systemctl command succeeded or not
        else if (event.command.get_command_name() == CMD_KICK_PLAYER) {
           std::string player = std::get<std::string>(event.get_parameter("player"));
           bool kicked_player = pipe_srb2_kick_player(player);
           std::stringstream result;
           if (!kicked_player) {
               result << "```Failed to kick player " << player << " .```\n";
           } else {
               result << "```Attempted to kick player " << player << " .```\n";
           }
           dpp::message msg(event.command.channel_id, result.str());
           event.reply(msg.set_flags(dpp::m_ephemeral));

       }

        else if (event.command.get_command_name() == CMD_BAN_PLAYER) {
           std::string player = std::get<std::string>(event.get_parameter("player"));
           bool banned_player = pipe_srb2_ban_player(player);
           std::stringstream result;
           if (banned_player) {
               result << "```Attempted to ban player " << player << " .```\n";
           } else {
               result << "```Failed to ban player " << player << " .```\n";
           }
           dpp::message msg(event.command.channel_id, result.str());
           event.reply(msg.set_flags(dpp::m_ephemeral));

       }

        else if (event.command.get_command_name() == CMD_RESTART_SERVER) {
            std::string cmd = "systemctl restart " + service_name + " --user";
            int ret = system(cmd.c_str());
            std::string return_status;
            if (ret == 0) {
                return_status = "```Server was succesfully restarted!```\n";
            } else {
                return_status = "```Failed to restart server```\n";
            }
            dpp::message msg(event.command.channel_id, return_status);
            event.reply(msg.set_flags(dpp::m_ephemeral));
        }

        else if (event.command.get_command_name() == CMD_STOP_SERVER) {
            std::string cmd = "systemctl stop " + service_name + " --user";
            int ret = system(cmd.c_str());
            std::string return_status;
            if (ret == 0) {
                return_status = "```Server was stopped.```\n";
            } else {
                return_status = "```Failed to stop server```\n";
            }

            dpp::message msg(event.command.channel_id, return_status);
            event.reply(msg.set_flags(dpp::m_ephemeral));
        }
    });

    bot.on_message_create([&bot, bridge_channel_id, bot_id](const dpp::message_create_t& event) {
        std::string channel_str = std::to_string(event.msg.channel_id);
        std::string author_str = std::to_string(event.msg.author.id);
        if (channel_str != bridge_channel_id) return;
        if (author_str == bot_id) return;

        std::string content = event.msg.content;
        if (content.empty()) return;

        std::string sanitized = sanitize_message_for_srb2(content);
        if (sanitized.size() <= 1) return;

        std::string display_name = event.msg.author.global_name;
        if (display_name.empty()) {
            display_name = event.msg.author.username;
        }

        std::string home = dir_srb2_str();
        std::string bridge_path = home + "/luafiles/client/DiscordBot";
        std::filesystem::create_directories(bridge_path);
        std::ofstream disc_file(bridge_path + "/discordmessage.txt", std::ios::app);
        if (disc_file.is_open()) {
            disc_file << "<" << display_name << "> " << sanitized << "\n";
        }
    });

    bot.on_ready([&bot, guild_id](const dpp::ready_t& event) {

        // Iterates over all commands on the server and deletes them (guild-specific)
        bot.guild_commands_get(bot.me.id, [&bot, guild_id](const dpp::confirmation_callback_t& cb){
            auto commands = std::get<dpp::slashcommand_map>(cb.value);
            for (auto& [id, cmd] : commands) {
                bot.guild_command_delete(bot.me.id, guild_id);
            }
        });

        if (dpp::run_once<struct register_bot_commands>()) {

            // TODO: When the function implementations are created,
            // move these comments to their respective functions.
            //
            dpp::slashcommand   get_script(CMD_GET_SCRIPT, "Gets the entire srb2 script and uploads it to the user.", bot.me.id);
            get_script
                .set_default_permissions(PERMS);
            dpp::slashcommand   find_line(CMD_FIND_LINE, "Finds matches of the specified string to look for.", bot.me.id);
            find_line
                .add_option(dpp::command_option(dpp::co_string, "target_string", "String to look up in the script", true))
                .set_default_permissions(PERMS);
            dpp::slashcommand   inspect_line(CMD_INSPECT_LINE, "Inspects lines at a given line number from the server script, and shows them.", bot.me.id);
            inspect_line
                .add_option(dpp::command_option(dpp::co_integer, "line_num", "Line number to inspect.", true))
                .set_default_permissions(PERMS);
            dpp::slashcommand   insert_line(CMD_INSERT_LINE, "Adds a line to the script at a given line number, then prints the result.", bot.me.id);
            insert_line
                .add_option(dpp::command_option(dpp::co_integer, "line_num", "The line number at which to insert the content.", true))
                .add_option(dpp::command_option(dpp::co_string, "line_contents", "The line contents that will be inserted.", false))
                .set_default_permissions(PERMS);
            dpp::slashcommand   remove_line(CMD_REMOVE_LINE, "Removes a line from the script, then prints the result.", bot.me.id);
            remove_line
                .add_option(dpp::command_option(dpp::co_integer, "line_num", "The line to remove.", true))
                .set_default_permissions(PERMS);
            dpp::slashcommand   restart_server(CMD_RESTART_SERVER, "Restarts the SRB2 server.", bot.me.id);
            restart_server
                .set_default_permissions(PERMS);
            dpp::slashcommand   stop_server(CMD_STOP_SERVER, "Stops the SRB2 server.", bot.me.id);
            stop_server
                .set_default_permissions(PERMS);
            dpp::slashcommand   move_line(CMD_MOVE_LINE, "Moves a line from a given old line number to a new line number.", bot.me.id);
            move_line
                .add_option(dpp::command_option(dpp::co_integer, "old_line_num", "The line to be moved.", true))
                .add_option(dpp::command_option(dpp::co_integer, "new_line_num", "The destination of the line to be moved.", true))
                .set_default_permissions(PERMS);
            dpp::slashcommand   change_line(CMD_CHANGE_LINE, "Change the contents of the given line number.", bot.me.id);
            change_line
                .add_option(dpp::command_option(dpp::co_integer, "line_num", "The line to be moved.", true))
                .add_option(dpp::command_option(dpp::co_string, "line_contents", "The new content for the given line.", false))
                .set_default_permissions(PERMS);
            dpp::slashcommand   search_wads(CMD_SEARCH_WADS, "Look up wads that contain a target string in filename.", bot.me.id);
            search_wads
                .add_option(dpp::command_option(dpp::co_string, "target_filename", "The string to look for in filenames.", true))
                .set_default_permissions(PERMS);
            dpp::slashcommand   list_wads(CMD_LIST_WADS, "Lists wads in the wads directory.", bot.me.id);
            list_wads
                .set_default_permissions(PERMS);
            dpp::slashcommand   addfile_upload(CMD_ADDFILE_UPLOAD, "Uploads a file to the server's wad directory via an attachment.", bot.me.id);
            addfile_upload
                .add_option(dpp::command_option(dpp::co_attachment, "file", "File to upload to the server's wad directory", true))
                .set_default_permissions(PERMS);

            dpp::slashcommand   addfile_link(CMD_ADDFILE_LINK, "Uploads a file to the server's wad directory via a link.", bot.me.id);
            addfile_link
                .add_option(dpp::command_option(dpp::co_string, "file_url", "URL of file to upload to the server's wad directory", true))
                .set_default_permissions(PERMS);
            dpp::slashcommand   kick_player(CMD_KICK_PLAYER, "Kicks a player off the server", bot.me.id);
            kick_player
                .add_option(dpp::command_option(dpp::co_string, "player", "Player to kick from the server.", true))
                .set_default_permissions(PERMS);
            dpp::slashcommand   ban_player(CMD_BAN_PLAYER, "Bans a player off the server", bot.me.id);
            ban_player
                .add_option(dpp::command_option(dpp::co_string, "player", "Player to ban from the server.", true))
                .set_default_permissions(PERMS);
            dpp::slashcommand   server_do(CMD_SERVER_DO, "Do something as the server.", bot.me.id);
            server_do
                .add_option(dpp::command_option(dpp::co_string, "server_command", "Server command to execute", true))
                .set_default_permissions(PERMS);
            dpp::slashcommand   server_say(CMD_SERVER_SAY, "Say something as the server", bot.me.id);
            server_say
                .add_option(dpp::command_option(dpp::co_string, "server_message", "Message that the server will say", true))
                .set_default_permissions(PERMS);

            bot.guild_bulk_command_create({
                get_script,

                find_line,
                inspect_line,
                insert_line,
                remove_line,
                move_line,
                change_line,

                restart_server,
                stop_server,

                search_wads,
                list_wads,

                addfile_upload,
                addfile_link,

                server_do,
                server_say,
                kick_player,
                ban_player,
            }, guild_id);
        }
    });

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

    // Chat bridge status
    if (bridge_channel_id != "0") {
        std::cout << "[bridge] disc -> srb2: channel " << bridge_channel_id
                  << " (messages forwarded to " << bridge_dir << "/discordmessage.txt)\n";
        std::cout << "[bridge] srb2 -> disc: polling " << bridge_dir << "/Messages.txt"
                  << " every 2s" << std::endl;
    } else {
        std::cout << "[bridge] disabled (channel_id not set in secret.json)" << std::endl;
    }

    std::unordered_map<std::string, std::string> guild_emojis;
    bot.on_ready([&bot, guild_id, &guild_emojis](const dpp::ready_t& event) {
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

    size_t seek_start = 0;
    dpp::snowflake bridge_channel_sf = std::stoull(bridge_channel_id);
    bot.start_timer([&bot, messages_path, &seek_start, bridge_channel_sf, &guild_emojis](dpp::timer) {
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
                    auto event = bridge_parse_event(line);
                    if (event) {
                        if (!chat_lines.empty()) {
                            dpp::message chat_msg(bridge_channel_sf, chat_lines);
                            chat_msg.set_allowed_mentions(false, false, false, false, {});
                            bot.message_create(chat_msg);
                            chat_lines.clear();
                        }
                        dpp::embed embed;
                        if (event->type == "SERVER_START") {
                            embed.set_title("Server Online");
                            embed.set_description(event->fields.size() >= 2
                                ? "**" + event->fields[0] + "** — " + event->fields[1] : "SRB2 Server started");
                            embed.set_color(0x57F287);
                        } else if (event->type == "MAP_CHANGE") {
                            embed.set_title("Map Changed");
                            std::string map_name = event->fields.size() >= 2
                                ? event->fields[1] + " (" + event->fields[0] + ")" : event->fields[0];
                            embed.set_description("Now playing: **" + map_name + "**");
                            embed.set_color(0x5865F2);
                        } else if (event->type == "ROUND_START") {
                            std::string gt = event->fields.size() >= 1 ? event->fields[0] : "Round";
                            std::string map_info = event->fields.size() >= 3
                                ? event->fields[2] + " (" + event->fields[1] + ")" : "";
                            embed.set_title(gt + " — Round Started");
                            embed.set_description("Map: **" + map_info + "**");
                            embed.set_color(0x5865F2);
                        } else if (event->type == "ROUND_END") {
                            std::string gt = event->fields.size() >= 1 ? event->fields[0] : "Round";
                            embed.set_title(gt + " — Round Ended");
                            std::string scoreboard;
                            for (size_t i = 1; i < event->fields.size(); i++) {
                                size_t colon = event->fields[i].find(':');
                                if (colon != std::string::npos) {
                                    std::string val = event->fields[i].substr(colon + 1);
                                    scoreboard += val + "\n";
                                }
                            }
                            embed.set_description(scoreboard.empty() ? "Round complete." : scoreboard);
                            embed.set_color(0x747F8D);
                        } else if (event->type == "CTF_CAPTURE") {
                            std::string player = event->fields.size() >= 1 ? event->fields[0] : "Someone";
                            std::string team = event->fields.size() >= 2 ? event->fields[1] : "";
                            embed.set_title("Flag Captured!");
                            embed.set_description("**" + player + "** captured the flag for **" + team + "**!");
                            embed.set_color(0xFEE75C);
                        } else if (event->type == "CTF_DROP") {
                            std::string player = event->fields.size() >= 1 ? event->fields[0] : "Someone";
                            embed.set_title("Flag Dropped");
                            embed.set_description("**" + player + "** dropped the flag.");
                            embed.set_color(0xED4245);
                        } else if (event->type == "CTF_PICKUP") {
                            std::string player = event->fields.size() >= 1 ? event->fields[0] : "Someone";
                            embed.set_title("Flag Taken");
                            embed.set_description("**" + player + "** picked up the flag.");
                            embed.set_color(0xFEE75C);
                        } else if (event->type == "CTF_RETURN") {
                            std::string player = event->fields.size() >= 1 ? event->fields[0] : "Server";
                            embed.set_title("Flag Returned");
                            embed.set_description("The flag was returned by **" + player + "**.");
                            embed.set_color(0x747F8D);
                        } else if (event->type == "PLAYER_JOIN") {
                            std::string player = event->fields.size() >= 1 ? event->fields[0] : "Someone";
                            embed.set_title("Player Joined");
                            embed.set_description("**" + player + "** has joined the game.");
                            embed.set_color(0x57F287);
                        } else if (event->type == "PLAYER_QUIT") {
                            std::string player = event->fields.size() >= 1 ? event->fields[0] : "Someone";
                            embed.set_title("Player Left");
                            embed.set_description("**" + player + "** has left the game.");
                            embed.set_color(0xED4245);
                        } else {
                            embed.set_title(event->type);
                            embed.set_description(line);
                            embed.set_color(0x2F3136);
                        }
                        pending_embeds.push_back(embed);
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
        }
    }, 2);

    bot.start(dpp::st_wait);
}
