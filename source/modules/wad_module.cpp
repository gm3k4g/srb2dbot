#include "srb2dbot/module.hpp"
#include "srb2dbot/utils.hpp"
#include "srb2dbot/server.hpp"
#include "version.h"
#include <dpp/dpp.h>
#include <fstream>
#include <sstream>
#include <filesystem>

static const size_t MAX_WAD_SIZE = 100ULL * 1024 * 1024;

class WADManagerModule : public Module {
public:
    auto name() const -> std::string_view override { return "wad_manager"; }
    auto description() const -> std::string_view override { return "List, search, and upload WAD files"; }

    explicit WADManagerModule(dpp::cluster& bot_ref) : bot_(bot_ref) {}

    auto commands(dpp::snowflake bot_id, dpp::permission perms) -> std::vector<dpp::slashcommand> override {
        dpp::slashcommand search_wads(CMD_SEARCH_WADS, "Look up wads that contain a target string in filename.", bot_id);
        search_wads.add_option(dpp::command_option(dpp::co_string, "target_filename", "The string to look for in filenames.", true));
        search_wads.set_default_permissions(perms);

        dpp::slashcommand list_wads(CMD_LIST_WADS, "Lists wads in the wads directory.", bot_id);
        list_wads.set_default_permissions(perms);

        dpp::slashcommand addfile_upload(CMD_ADDFILE_UPLOAD, "Uploads a file to the server's wad directory via an attachment.", bot_id);
        addfile_upload.add_option(dpp::command_option(dpp::co_attachment, "file", "File to upload to the server's wad directory", true));
        addfile_upload.set_default_permissions(perms);

        dpp::slashcommand addfile_link(CMD_ADDFILE_LINK, "Uploads a file to the server's wad directory via a link.", bot_id);
        addfile_link.add_option(dpp::command_option(dpp::co_string, "file_url", "URL of file to upload to the server's wad directory", true));
        addfile_link.set_default_permissions(perms);

        return {search_wads, list_wads, addfile_upload, addfile_link};
    }

    auto handle_slashcommand(const dpp::slashcommand_t& event) -> bool override {
        auto cmd = event.command.get_command_name();

        if (cmd == CMD_LIST_WADS) {
            std::string wad_list = wads_get_str();
            dpp::message msg(event.command.channel_id, "");
            msg.add_file("wads_list.txt", wad_list);
            event.reply(msg.set_flags(dpp::m_ephemeral));
            return true;
        }

        if (cmd == CMD_SEARCH_WADS) {
            std::string target = std::get<std::string>(event.get_parameter("target_filename"));
            std::string wad_list = wads_search_str(target);
            dpp::message msg(event.command.channel_id, "");
            msg.add_file("wads_search.txt", wad_list);
            event.reply(msg.set_flags(dpp::m_ephemeral));
            return true;
        }

        if (cmd == CMD_ADDFILE_UPLOAD) {
            auto wad_file = event.get_parameter("file");
            if (std::holds_alternative<dpp::snowflake>(wad_file)) {
                dpp::snowflake attachment_id = std::get<dpp::snowflake>(wad_file);
                auto attachment = event.command.resolved.attachments.find(attachment_id);
                if (attachment != event.command.resolved.attachments.end()) {
                    std::string url = attachment->second.url;
                    std::string filename = sanitize_filename(attachment->second.filename);
                    bot_.request(url, dpp::m_get, [this, event, filename](const dpp::http_request_completion_t& resp) {
                        handle_upload_response(resp, event, filename, false);
                    });
                }
            }
            return true;
        }

        if (cmd == CMD_ADDFILE_LINK) {
            std::string url = std::get<std::string>(event.get_parameter("file_url"));
            std::string filename = link_filename_str(url);
            bot_.request(url, dpp::m_get, [this, event, filename](const dpp::http_request_completion_t& resp) {
                handle_upload_response(resp, event, filename, true);
            });
            return true;
        }

        return false;
    }

private:
    dpp::cluster& bot_;

    void handle_upload_response(
        const dpp::http_request_completion_t& resp,
        const dpp::slashcommand_t event,
        const std::string& filename,
        bool from_url)
    {
        if (resp.status == 200) {
            if (resp.body.size() > MAX_WAD_SIZE) {
                bot_.interaction_response_create(
                    event.command.id, event.command.token,
                    dpp::interaction_response(
                        dpp::ir_channel_message_with_source,
                        dpp::message("```File " + filename + " exceeds max size of 100MB.```\n").set_flags(dpp::m_ephemeral)
                    )
                );
                return;
            }
            std::string wad_dir = dir_srb2_str();
            wad_dir.append("/addons/").append(filename);
            std::ofstream buffer(wad_dir);
            buffer << resp.body;

            std::string prefix = from_url ? " from URL" : "";
            bot_.interaction_response_create(
                event.command.id, event.command.token,
                dpp::interaction_response(
                    dpp::ir_channel_message_with_source,
                    dpp::message("```Successfully uploaded " + filename + prefix + " to the server.```\n").set_flags(dpp::m_ephemeral)
                )
            );
        } else {
            std::string prefix = from_url ? " from URL" : "";
            bot_.interaction_response_create(
                event.command.id, event.command.token,
                dpp::interaction_response(
                    dpp::ir_channel_message_with_source,
                    dpp::message("```Failed to upload " + filename + prefix + ".```\n").set_flags(dpp::m_ephemeral)
                )
            );
        }
    }

    static auto wads_get_str() -> std::string {
        std::string wads_dir = dir_srb2_str();
        wads_dir.append("/addons");
        std::stringstream wads_list;
        wads_list << "Showing all files in " << wads_dir << ":\n\n";
        int i = 1;
        try {
            for (const auto& entry : std::filesystem::directory_iterator(wads_dir)) {
                wads_list << i << " " << entry.path().filename() << "\n";
                i++;
            }
        } catch (const std::filesystem::filesystem_error&) {
            wads_list << "(directory not found or inaccessible)\n";
        }
        if (i == 1) wads_list << "(empty)\n";
        return wads_list.str();
    }

    static auto wads_search_str(const std::string& target) -> std::string {
        std::string wads_dir = dir_srb2_str();
        wads_dir.append("/addons");
        std::stringstream wads_list;
        wads_list << "Showing all matches in " << wads_dir << ":\n\n";
        int i = 1;
        try {
            for (const auto& entry : std::filesystem::directory_iterator(wads_dir)) {
                std::string name = entry.path().filename().string();
                if (name.find(target) != std::string::npos) {
                    wads_list << i << " " << name << "\n";
                    i++;
                }
            }
        } catch (const std::filesystem::filesystem_error&) {
            wads_list << "(directory not found or inaccessible)\n";
        }
        if (i == 1) wads_list << "(no matches)\n";
        return wads_list.str();
    }
};

auto create_wad_module(dpp::cluster& bot) -> std::unique_ptr<Module> {
    return std::make_unique<WADManagerModule>(bot);
}
