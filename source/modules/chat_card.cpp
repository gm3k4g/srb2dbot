#include "srb2dbot/module.hpp"
#include "srb2dbot/bridge.hpp"
#include "srb2dbot/utils.hpp"
#include <dpp/dpp.h>
#include <optional>
#include <ctime>
#include <sstream>

class ChatCardModule : public Module {
public:
    auto name() const -> std::string_view override { return "chat_card"; }
    auto description() const -> std::string_view override { return "Relay SRB2 chat messages as Discord embeds"; }
    explicit ChatCardModule(std::string fmt) : fmt_(std::move(fmt)) {}
    auto commands(dpp::snowflake, dpp::permission) -> std::vector<dpp::slashcommand> override { return {}; }

    auto handle_bridge_event(const BridgeEvent& event) -> std::optional<dpp::embed> override {
        if (event.type != "CHAT") return std::nullopt;
        if (event.fields.size() < 3) return std::nullopt;

        auto& f = event.fields;
        std::string node           = f[0];
        std::string name           = sanitize_for_discord(f[1]);
        std::string message        = f[2];
        for (size_t i = 3; i + 4 < f.size(); i++) {
            message += "|" + f[i];
        }
        message = sanitize_for_discord(message);
        std::string skin          = f.size() >= 6 ? sanitize_for_discord(f[f.size() - 4]) : "";
        std::string jointime_str  = f.size() >= 5 ? f[f.size() - 3] : "";
        std::string flag          = f.size() >= 6 ? f[f.size() - 2] : "0";
        std::string team          = f.size() >= 7 ? f[f.size() - 1] : "none";
        if (message.empty()) return std::nullopt;

        // Build metadata line: [:placard:] PlayerName | :red_square: | 1 | 10m | 2024-01-01 HH:MM:SS
        std::string meta;
        if (flag != "0") meta += ":placard: ";
        meta += fmt_.empty() ? name : substitute_placeholders(fmt_, {{"name", name}, {"node", node}, {"skin", skin}});
        if (team == "1")       meta += " :red_square:";
        else if (team == "2")  meta += " :blue_square:";
        if (!jointime_str.empty() && jointime_str != "0") {
            meta += " | " + format_online_time(jointime_str);
        }
        {
            auto now = std::time(nullptr);
            std::tm tm = *std::localtime(&now);
            std::ostringstream ts;
            char wday[4] = {};
            std::strftime(wday, sizeof(wday), "%a", &tm);
            int day = tm.tm_mday;
            const char* ord = "th";
            if (day % 10 == 1 && day != 11) ord = "st";
            else if (day % 10 == 2 && day != 12) ord = "nd";
            else if (day % 10 == 3 && day != 13) ord = "rd";
            char month[16] = {};
            std::strftime(month, sizeof(month), "%B", &tm);
            char hm[6] = {};
            std::strftime(hm, sizeof(hm), "%H:%M", &tm);
            ts << wday << " " << day << ord << ", " << month << ", " << hm;
            meta += " | " + ts.str();
        }
        meta += " | " + node;

        dpp::embed embed;
        embed.set_title(meta);
        embed.set_description(message);
        if (team == "1")            embed.set_color(0xE74C3C);
        else if (team == "2")       embed.set_color(0x3498DB);
        else                        embed.set_color(0x2F3136);

        return embed;
    }

private:
    std::string fmt_;

    static auto format_online_time(const std::string& jointime_ticks) -> std::string {
        std::time_t join = 0;
        try { join = std::stoll(jointime_ticks); } catch (...) { return ""; }
        auto now = std::time(nullptr);
        std::time_t elapsed = now - join;
        if (elapsed < 0) return "just now";
        if (elapsed < 60) return std::to_string(elapsed) + "s";
        std::int64_t mins = elapsed / 60;
        std::int64_t secs = elapsed % 60;
        if (mins < 60) return std::to_string(mins) + "m " + std::to_string(secs) + "s";
        std::int64_t hrs = mins / 60;
        mins = mins % 60;
        return std::to_string(hrs) + "h " + std::to_string(mins) + "m";
    }
};

auto create_chat_card_module(const std::string& fmt) -> std::unique_ptr<Module> {
    return std::make_unique<ChatCardModule>(fmt);
}
