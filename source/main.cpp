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
#include <fstream>
#include <sstream>
#include <dpp/message.h>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

//#include <absl/strings/match.h>
//#include <cpr/cpr.h>

#include "version.h"
using json = nlohmann::json;

const std::array<std::string, 2>INVALID_ARRAY = {"Invalid filename", "Invalid content"};

namespace {
    auto link_filename_str(const std::string& url) -> std::string {
        // Remove query string if present
        std::string base = url.substr(0, url.find('?'));

        // Extract filename from path
        std::string filename = std::filesystem::path(base).filename().string();

        return filename.empty() ? "downloaded_file" : filename;
    }


    auto dir_srb2_str() -> std::string {
        struct passwd *pw = getpwuid(getuid());
        std::string homedir = pw->pw_dir;

        std::string script_path = homedir;
        std::string srb2_home = "/.srb2"; // TODO: Look at -h parameter (to look at a different srb2 directory)
        script_path.append(srb2_home);
        return script_path;
    }

    auto wads_get() -> std::string {
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

    auto zellij_is_srb2_alive() -> bool {
        std::string cmd = "zellij list-sessions | grep srb2b";
        bool success = system(cmd.c_str());
        return success;
    }

    auto zellij_srb2_server_say(std::string msg) -> bool {
        bool success = false;
        if (zellij_is_srb2_alive() == EXIT_FAILURE) {
            return false;
        }
        std::stringstream cmd;

        cmd << "zellij -s srb2b action write-chars \"say " << msg << "\"";
        success = system(cmd.str().c_str());
        cmd.str(std::string());

        cmd << "zellij -s srb2b action write 13";
        success = system(cmd.str().c_str());
        return success;
    }

    auto zellij_srb2_kick_player(std::string &player) -> bool {
        bool success = zellij_is_srb2_alive();
        if (success == EXIT_FAILURE) {
            return false;
        }
        std::stringstream cmd;

        cmd << "zellij -s srb2b action write-chars " << "\"kick " << player << "\"";
        success = system(cmd.str().c_str());
        cmd.str(std::string());

        cmd << "zellij -s srb2b action write 13";
        success = system(cmd.str().c_str());
        return success;
    }

    auto zellij_srb2_ban_player(std::string &player) -> bool {
        bool success = zellij_is_srb2_alive();
        if (success == EXIT_FAILURE) {
            return false;
        }
        std::stringstream cmd;

        cmd << "zellij -s srb2b action write-chars " << "\"ban " << player << "\"";
        success = system(cmd.str().c_str());
        cmd.str(std::string());

        cmd << "zellij -s srb2b action write 13";
        success = system(cmd.str().c_str());
        return success;
    }

    // returns: [0]: Script name [1]: Script absolute path
    auto script_get(std::ifstream& script_file) -> std::array<std::string, 2> {
        std::string script_path = dir_srb2_str();

        // TODO: Look at -c parameter (to look at a different script file name)
        std::string script_name = "srb2b.sh";

        script_path.append("/srb2_servers.d/").append(script_name);
        std::ifstream file(script_path);
        if (!file.is_open()) {
            return INVALID_ARRAY;
        }
        script_file.open(script_path);

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
            //if (absl::StrContains(line, "password")) {
            if (line.find("password") != std::string::npos) {
                output_buffer << "[REDACTED]\n";
            } else {
                output_buffer << line << "\n";
            }

        }

        std::array<std::string, 2> script = {script_content[0], output_buffer.str()};
        return script;
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
        std::istringstream split_lines(script_content);
        std::vector<std::string> lines;
        std::stringstream inspected_region;
        std::string line;

        int size = 0;
        while (std::getline(split_lines, line)) {
            size += 1;
        }
        split_lines.clear();
        split_lines.seekg(0);

        int i = 1;
        int targeted_line = target_line;
        if (target_line < 1) {
            targeted_line = 1;
        } else if (target_line > size) {
            targeted_line = size;
        }
        const int range = 8;
        int start   = std::max(1, targeted_line - range-1);
        int end     = std::min(size - 1, targeted_line + range);

        inspected_region << "```Showing line " << start << " to " << end << "```\n";
        inspected_region << "```";
        while (std::getline(split_lines, line)) {
            if (i >= start && i <= end) {
                if (i == targeted_line) {
                    inspected_region << "\n-> " << i << ": " << line << "\n\n";
                } else {
                    inspected_region << i << ": " << line << "\n";
                }
            }
            i += 1;
        }
        inspected_region << "```";

        return inspected_region.str();
    }

    auto script_add_line(std::string& line_content, int line_number) -> bool {
        std::ifstream script_file;
        auto script_content = script_get(script_file);
        std::stringstream old_script;
        old_script << script_file.rdbuf();

        std::stringstream new_script;
        std::string line;
        int i = 1;
        while(std::getline(old_script, line)) {
            if (i == line_number) {
                if (line_content.empty()) {
                    new_script << "\n";
                } else {
                    new_script << line_content << "\n";
                }
            }
            new_script << line << "\n";
            i += 1;
        }

        return script_write(script_content[1], new_script.str());
    }

    auto script_change_line(std::string& line_content, int line_number) -> bool {
        std::ifstream script_file;
        auto script_content = script_get(script_file);
        std::stringstream old_script;
        old_script << script_file.rdbuf();

        std::stringstream new_script;
        std::string line;
        int i = 1;
        while(std::getline(old_script, line)) {
            if (i == line_number) {
                if (line_content.empty()) {
                    new_script << "\n";
                } else {
                    new_script << line_content << "\n";
                }
            } else {
                new_script << line << "\n";
            }
            i += 1;
        }

        return script_write(script_content[1], new_script.str());
    }

    auto script_move_line(int old_line, int new_line) -> bool {
        std::ifstream script_file;
        auto script_content = script_get(script_file);
        std::stringstream old_script;
        old_script << script_file.rdbuf();

        std::stringstream new_script;
        std::string line;
        std::string held_line;

        int i = 1;
        while(std::getline(old_script, line)) {
            if (old_line == i) {
                held_line = line;
                break;
            }
            i += 1;
        }

        old_script.seekg(0);
        old_script.clear();
        i = 1;

        while(std::getline(old_script, line)) {
            if (new_line == i) {
                new_script << held_line << "\n";
                //i += 1;
                //new_script << line << "\n";
            }
            else if (old_line == i) {

            } else {
                new_script << line << "\n";
            }
            i += 1;
        }

        return script_write(script_content[1], new_script.str());
    }

    // TODO: parameter that lets you keep an empty line
    // instead of removing the line?
    auto script_remove_line(int line_num) -> bool {
        std::ifstream script_file;
        auto script_content = script_get(script_file);
        std::stringstream old_script;
        old_script << script_file.rdbuf();

        std::stringstream new_script;
        std::string line;
        int i = 1;
        while (std::getline(old_script, line)) {
            if (i != line_num) {
                new_script << line << "\n";
            }
            i += 1;
        }

        return script_write(script_content[1], new_script.str());
    }


} // namespace

// Permissions
const auto PERMS = dpp::p_administrator;

int main() {
    // Read secret
    std::ifstream secret("secret.json");
    json data = json::parse(secret);
    if (data == NULL) {
        std::cout << "ERROR: Secret wasn't found!\n";
        return EXIT_FAILURE;
    }

    // TODO: Create directories if they don't exist.

    std::string guild_id_str = data["guild_id"].dump();
    guild_id_str = guild_id_str.substr(1, guild_id_str.size() - 2);
    auto guild_id = std::stol(guild_id_str);

    // TODO: Bot token will be taken from current directory
    // TODO: Create CLI option that lets you looks elsewhere for token
    // Get bot token and trim the quote marks off the start and end
    std::string bot_token = data["bot_token"].dump();
    bot_token = bot_token.substr(1, bot_token.size() - 2);

    //std::cout << USAGE << std::endl;

    dpp::cluster bot(bot_token);

    bot.on_log(dpp::utility::cout_logger());

    bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
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

        else if (event.command.get_command_name() == CMD_LISTFILES) {
            std::string wad_list = wads_get();
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
                    std::string filename = attachment->second.filename;

                    // Download the file from Discord CDN
                    bot.request(url, dpp::m_get, [&bot, event, filename](const dpp::http_request_completion_t& resp) {
                        if (resp.status == 200) {
                            std::string content = resp.body;
                            std::string wad_dir = dir_srb2_str();
                            wad_dir.append("/addons/").append(filename);

                            std::ofstream buffer(wad_dir);
                            buffer << content;

                            bot.interaction_response_create(
                                event.command.id,
                                event.command.token,
                                dpp::interaction_response(
                                    dpp::ir_channel_message_with_source,
                                    dpp::message("```Successfully uploaded attached file to the server.```\n")
                                )
                            );
                        } else {
                            bot.interaction_response_create(
                                event.command.id,
                                event.command.token,
                                dpp::interaction_response(
                                    dpp::ir_channel_message_with_source,
                                    dpp::message("```Failed to upload the attached file.```\n")
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
                    std::string content = resp.body;
                    std::string wad_dir = dir_srb2_str();
                    wad_dir.append("/addons/").append(filename);

                    std::ofstream buffer(wad_dir);
                    buffer << content;

                    bot.interaction_response_create(
                        event.command.id,
                        event.command.token,
                        dpp::interaction_response(
                            dpp::ir_channel_message_with_source,
                            dpp::message("```Successfully uploaded file from URL to the server.```\n")
                        )
                    );
                } else {
                    bot.interaction_response_create(
                        event.command.id,
                        event.command.token,
                        dpp::interaction_response(
                            dpp::ir_channel_message_with_source,
                            dpp::message("```Failed to upload file from URL.```\n")
                        )
                    );
                }
            });
        }

        else if (event.command.get_command_name() == CMD_INSERTLINE) {

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
        else if (event.command.get_command_name() == CMD_REMOVELINE) {
            int64_t line_num = std::get<int64_t>(event.get_parameter("line_num"));

            bool script_removed_line = script_remove_line(line_num);
            std::stringstream result;
            if (!script_removed_line) {
                result << "```Couldn't add line!```";
            } else {
                auto script_contents = script_get_str();
                auto inspect = script_inspect(script_contents[1], line_num);
                result  << "```# Successfully removed line number " << line_num << "!```\n"
                        << inspect;
            }
            dpp::message msg(event.command.channel_id, result.str());
            event.reply(msg.set_flags(dpp::m_ephemeral));
        }

        else if (event.command.get_command_name() == CMD_CHANGELINE) {
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

        else if (event.command.get_command_name() == CMD_MOVELINE) {
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

        // Inspects a line. This will show the targeted line along with line numbers,
        // as well as 10 lines above and beneath the targeted line.
        else if (event.command.get_command_name() == CMD_INSPECTLINE) {
            int64_t line_num = std::get<int64_t>(event.get_parameter("line_num"));

            auto script_contents = script_get_str();
            std::string script_lines = script_inspect(script_contents[1], line_num);
            dpp::message msg(event.command.channel_id, script_lines);
            event.reply(msg.set_flags(dpp::m_ephemeral));
       }

       // Says something as the server.
       else if (event.command.get_command_name() == CMD_SERVER_SAY) {
           std::string serv_msg = std::get<std::string>(event.get_parameter("server_message"));
           bool success = zellij_srb2_server_say(serv_msg);
           std::string result;
           if (!success) {
               result = "```Failed to say message.```";
           } else {
               result = "```Success```";
           }
           dpp::message msg(event.command.channel_id, result);
           event.reply(msg.set_flags(dpp::m_ephemeral));
       }

       // TODO: find a more concrete way of knowing
       // whether command succeeded or not
       // (we're currently using system())
       else if (event.command.get_command_name() == CMD_KICK_PLAYER) {
           std::string player = std::get<std::string>(event.get_parameter("player"));
           bool kicked_player = zellij_srb2_kick_player(player);
           std::stringstream result;
           if (kicked_player) {
               result << "```Attempted to kick player " << player << " .```\n";
           } else {
               result << "```Failed to kick player " << player << " .```\n";
           }
           dpp::message msg(event.command.channel_id, result.str());
           event.reply(msg.set_flags(dpp::m_ephemeral));

       }

       else if (event.command.get_command_name() == CMD_BAN_PLAYER) {
           std::string player = std::get<std::string>(event.get_parameter("player"));
           bool banned_player = zellij_srb2_ban_player(player);
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
            int ret = system("systemctl restart srb2@srb2b --user");
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
            int ret = system("systemctl stop srb2@srb2b --user");
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

    bot.on_ready([&bot, guild_id](const dpp::ready_t& event) {

        // Used to delete the old global commands
        if (dpp::run_once<struct clear_bot_commands>()) {
            /* Now, we're going to wipe our commands */
            bot.global_bulk_command_delete();
            /* This one requires a guild id, otherwise it won't know what guild's commands it needs to wipe! */
            //bot.guild_bulk_command_delete(guild_id);
        }

        /* Because the run_once above uses a 'clear_bot_commands' struct, you can continue to register commands below! */

        if (dpp::run_once<struct register_bot_commands>()) {

            // TODO: When the function implementations are created,
            // move these comments to their respective functions.
            //
            dpp::slashcommand   get_script(CMD_GET_SCRIPT, "Gets the entire srb2 script and uploads it to the user.", bot.me.id);
            get_script
                .set_default_permissions(PERMS);
            dpp::slashcommand   inspect_line(CMD_INSPECTLINE, "Inspects lines at a given line number from the server script, and shows them.", bot.me.id);
            inspect_line
                .add_option(dpp::command_option(dpp::co_integer, "line_num", "Line number to inspect.", true))
                .set_default_permissions(PERMS);
            dpp::slashcommand   insert_line(CMD_INSERTLINE, "Adds a line to the script at a given line number, then prints the result.", bot.me.id);
            insert_line
                .add_option(dpp::command_option(dpp::co_integer, "line_num", "The line number at which to insert the content.", true))
                .add_option(dpp::command_option(dpp::co_string, "line_contents", "The line contents that will be inserted.", false))
                .set_default_permissions(PERMS);
            dpp::slashcommand   remove_line(CMD_REMOVELINE, "Removes a line from the script, then prints the result.", bot.me.id);
            remove_line
                .add_option(dpp::command_option(dpp::co_integer, "line_num", "The line to remove.", true))
                .set_default_permissions(PERMS);
            dpp::slashcommand   restart_server(CMD_RESTART_SERVER, "Restarts the SRB2 server.", bot.me.id);
            restart_server
                .set_default_permissions(PERMS);
            dpp::slashcommand   stop_server(CMD_STOP_SERVER, "Stops the SRB2 server.", bot.me.id);
            stop_server
                .set_default_permissions(PERMS);
            dpp::slashcommand   move_line(CMD_MOVELINE, "Moves a line from a given old line number to a new line number.", bot.me.id);
            move_line
                .add_option(dpp::command_option(dpp::co_integer, "old_line_num", "The line to be moved.", true))
                .add_option(dpp::command_option(dpp::co_integer, "new_line_num", "The destination of the line to be moved.", true))
                .set_default_permissions(PERMS);
            dpp::slashcommand   change_line(CMD_CHANGELINE, "Change the contents of the given line number.", bot.me.id);
            change_line
                .add_option(dpp::command_option(dpp::co_integer, "line_num", "The line to be moved.", true))
                .add_option(dpp::command_option(dpp::co_string, "line_contents", "The new content for the given line.", false))
                .set_default_permissions(PERMS);
            dpp::slashcommand   list_files(CMD_LISTFILES, "Lists wads in the wads directory.", bot.me.id);
            list_files
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
            dpp::slashcommand   server_say(CMD_SERVER_SAY, "Say something as the server", bot.me.id);
            server_say
                .add_option(dpp::command_option(dpp::co_string, "server_message", "Message that the server will say", true))
                .set_default_permissions(PERMS);

            bot.guild_bulk_command_create({
                get_script,
                inspect_line,
                insert_line,
                remove_line,
                restart_server,
                stop_server,
                move_line,
                change_line,
                list_files,
                addfile_upload,
                addfile_link,
                kick_player,
                ban_player,
                server_say
            }, guild_id);

            // This will make the server download the file(s) provided to SRB2's addon
            // directory, whether in URL form or as files.
            // It will then modify the script to include the new file
            // if it doesn't already exist.
            // A combination of file uploads and urls can also work.
            // /addfiles [FILES/LINK]

            // Examples:
            // /addfiles [URL 1] [URL 2] ... [URL n] Characters
            // /addfiles [FILE UPLOAD 1] [FILE UPLOAD 2] ... [FILE UPLOAD n] Maps
            //bot.guild_command_create(dpp::slashcommand(CMD_ADDFILE, "Adds files to the server's directory and script.", bot.me.id), GUILD_ID);

            // addfiles, but the other way around.
            // If it detects that a category has zero files, the category directory
            // will be removed both from the system and from the directories script file.
            //bot.global_command_create(dpp::slashcommand("removefile", "Removes files from the server's directory and script.", bot.me.id));

            //bot.global_command_create(dpp::slashcommand("changeorder", "Changes order of a wad file on the server.", bot.me.id));

            // Kicks one or more players from the game.
            // Examples:
            // /kick ["Player 1" "Player 2"]        # Multiple players
            // /kick [1,5]                          # Accepts nodes
            // /kick ["Player 5"] "A kick reason"   # Kick reason is optional
            //bot.global_command_create(dpp::slashcommand("kick", "Kicks players from the server.", bot.me.id));
            //bot.global_command_create(dpp::slashcommand("ban", "Bans players from the server.", bot.me.id));

            // Primarily present in case there might be anything admins might want to do.
            // /doas [COMMAND]

            // Examples:
            // /doas nodes
           //bot.global_command_create(dpp::slashcommand("doas", "Executes a command as the server admin.", bot.me.id));

            // Hardcoded, resets the game server, bypassing SRB2 entirely.
            //bot.global_command_create(dpp::slashcommand("restart_server", "Restarts the server.", bot.me.id));

            // For now, this only supports most of the gametypes in SRB2 Battle.
            //bot.global_command_create(dpp::slashcommand("get_stats", "Gets the current game status of the server.", bot.me.id));

            // IDEA: The following could be potentially implemented, but idk lol
            // "randomify [EXCLUDED VARIABLES]" -> immediately changes to a random map of a random gametype with random factors (e.g. random character picks, colors, game settings (time/score)
            //      , battle variables, etc.
            //      Excluded variables could be MAP (doesn't change map), CHARACTER, COLOR, BATTLE_VARIABLES (specify?), GAMETYPE_SETTINGS (gametype specific?), GAME_SETTINGS (time/score?)
            // "copy_config [NAME OF NEW CONFIG]" -> Copies the current server config into another server config (directory) in the `srb2dbot.d` directory.
            //      Can be used mostly as a way to create a copy of the discord server config.
            //

        }
    });

    bot.start(dpp::st_wait);
}
