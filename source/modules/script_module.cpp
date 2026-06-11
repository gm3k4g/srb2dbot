#include "srb2dbot/module.hpp"
#include "srb2dbot/script.hpp"
#include "srb2dbot/server.hpp"
#include "version.h"
#include <dpp/dpp.h>
#include <fstream>
#include <sstream>
#include <array>
#include <sys/wait.h>
#include <unistd.h>

static const std::array<std::string, 2> INVALID_ARRAY = {"Invalid filename", "Invalid content"};

class ScriptEditorModule : public Module {
public:
    auto name() const -> std::string_view override { return "script_editor"; }
    auto description() const -> std::string_view override { return "Edit the SRB2 server startup script"; }

    auto commands(dpp::snowflake bot_id, dpp::permission perms) -> std::vector<dpp::slashcommand> override {
        dpp::slashcommand get_script(CMD_GET_SCRIPT, "Gets the entire srb2 script and uploads it to the user.", bot_id);
        get_script.set_default_permissions(perms);

        dpp::slashcommand find_line(CMD_FIND_LINE, "Finds matches of the specified string to look for.", bot_id);
        find_line.add_option(dpp::command_option(dpp::co_string, "target_string", "String to look up in the script", true));
        find_line.set_default_permissions(perms);

        dpp::slashcommand inspect_line(CMD_INSPECT_LINE, "Inspects lines at a given line number from the server script, and shows them.", bot_id);
        inspect_line.add_option(dpp::command_option(dpp::co_integer, "line_num", "Line number to inspect.", true));
        inspect_line.set_default_permissions(perms);

        dpp::slashcommand insert_line(CMD_INSERT_LINE, "Adds a line to the script at a given line number, then prints the result.", bot_id);
        insert_line.add_option(dpp::command_option(dpp::co_integer, "line_num", "The line number at which to insert the content.", true));
        insert_line.add_option(dpp::command_option(dpp::co_string, "line_contents", "The line contents that will be inserted.", false));
        insert_line.set_default_permissions(perms);

        dpp::slashcommand remove_line(CMD_REMOVE_LINE, "Removes a line from the script, then prints the result.", bot_id);
        remove_line.add_option(dpp::command_option(dpp::co_integer, "line_num", "The line to remove.", true));
        remove_line.set_default_permissions(perms);

        dpp::slashcommand move_line(CMD_MOVE_LINE, "Moves a line from a given old line number to a new line number.", bot_id);
        move_line.add_option(dpp::command_option(dpp::co_integer, "old_line_num", "The line to be moved.", true));
        move_line.add_option(dpp::command_option(dpp::co_integer, "new_line_num", "The destination of the line to be moved.", true));
        move_line.set_default_permissions(perms);

        dpp::slashcommand change_line(CMD_CHANGE_LINE, "Change the contents of the given line number.", bot_id);
        change_line.add_option(dpp::command_option(dpp::co_integer, "line_num", "The line to be moved.", true));
        change_line.add_option(dpp::command_option(dpp::co_string, "line_contents", "The new content for the given line.", false));
        change_line.set_default_permissions(perms);

        return {get_script, find_line, inspect_line, insert_line, remove_line, move_line, change_line};
    }

    auto handle_slashcommand(const dpp::slashcommand_t& event) -> bool override {
        auto cmd = event.command.get_command_name();

        if (cmd == CMD_GET_SCRIPT) {
            auto script_contents = script_get_str();
            dpp::message msg(event.command.channel_id, "");
            msg.add_file(script_contents[0], script_contents[1]);
            event.reply(msg.set_flags(dpp::m_ephemeral));
            return true;
        }

        if (cmd == CMD_FIND_LINE) {
            std::string target = std::get<std::string>(event.get_parameter("target_string"));
            std::string found = script_find(target);
            if (found.empty()) {
                found = "```No matches were found!```";
            } else {
                found = "```" + found + "```";
            }
            event.reply(dpp::message(event.command.channel_id, found).set_flags(dpp::m_ephemeral));
            return true;
        }

        if (cmd == CMD_INSPECT_LINE) {
            int64_t line_num = std::get<int64_t>(event.get_parameter("line_num"));
            auto script_contents = script_get_str();
            std::string result = script_inspect(script_contents[1], static_cast<int>(line_num));
            event.reply(dpp::message(event.command.channel_id, result).set_flags(dpp::m_ephemeral));
            return true;
        }

        if (cmd == CMD_INSERT_LINE) {
            int64_t line_num = std::get<int64_t>(event.get_parameter("line_num"));
            std::string line_content;
            if (event.get_parameter("line_contents").index() != 0) {
                line_content = std::get<std::string>(event.get_parameter("line_contents"));
            }
            bool ok = script_add_line(line_content, static_cast<int>(line_num));
            std::stringstream result;
            if (!ok) {
                result << "```Couldn't add line!```";
            } else if (!script_validate()) {
                result << "```Script written but bash validation failed. Check syntax and undo with /remove_line "
                       << line_num << ".```";
            } else {
                auto script_contents = script_get_str();
                auto inspect = script_inspect(script_contents[1], static_cast<int>(line_num));
                result << "```# Successfully added to line number " << line_num << "!```\n" << inspect;
            }
            event.reply(dpp::message(event.command.channel_id, result.str()).set_flags(dpp::m_ephemeral));
            return true;
        }

        if (cmd == CMD_REMOVE_LINE) {
            int64_t line_num = std::get<int64_t>(event.get_parameter("line_num"));
            bool ok = script_remove_line(static_cast<int>(line_num));
            std::stringstream result;
            if (!ok) {
                result << "```Couldn't remove line!```";
            } else if (!script_validate()) {
                result << "```Script written but bash validation failed. Check syntax.```";
            } else {
                auto script_contents = script_get_str();
                auto inspect = script_inspect(script_contents[1], static_cast<int>(line_num));
                result << "```# Successfully removed line number " << line_num << "!```\n" << inspect;
            }
            event.reply(dpp::message(event.command.channel_id, result.str()).set_flags(dpp::m_ephemeral));
            return true;
        }

        if (cmd == CMD_CHANGE_LINE) {
            int64_t line_num = std::get<int64_t>(event.get_parameter("line_num"));
            std::string line_content;
            if (event.get_parameter("line_contents").index() != 0) {
                line_content = std::get<std::string>(event.get_parameter("line_contents"));
            }
            bool ok = script_change_line(line_content, static_cast<int>(line_num));
            std::stringstream result;
            if (!ok) {
                result << "```Couldn't change line!```";
            } else if (!script_validate()) {
                result << "```Script written but bash validation failed. Check syntax and undo with /change_line.```";
            } else {
                auto script_contents = script_get_str();
                auto inspect = script_inspect(script_contents[1], static_cast<int>(line_num));
                result << "```# Successfully changed line at line number " << line_num << "!```\n" << inspect;
            }
            event.reply(dpp::message(event.command.channel_id, result.str()).set_flags(dpp::m_ephemeral));
            return true;
        }

        if (cmd == CMD_MOVE_LINE) {
            int64_t old_line = std::get<int64_t>(event.get_parameter("old_line_num"));
            int64_t new_line = std::get<int64_t>(event.get_parameter("new_line_num"));
            bool ok = script_move_line(static_cast<int>(old_line), static_cast<int>(new_line));
            std::stringstream result;
            if (!ok) {
                result << "```Couldn't move line!```";
            } else if (!script_validate()) {
                result << "```Script written but bash validation failed. Check syntax and undo with /move_line.```";
            } else {
                auto script_contents = script_get_str();
                auto inspect = script_inspect(script_contents[1], static_cast<int>(new_line));
                result << "```# Successfully moved line from " << old_line << " to " << new_line << "!```\n" << inspect;
            }
            event.reply(dpp::message(event.command.channel_id, result.str()).set_flags(dpp::m_ephemeral));
            return true;
        }

        return false;
    }
};

auto create_script_module() -> std::unique_ptr<Module> {
    return std::make_unique<ScriptEditorModule>();
}
