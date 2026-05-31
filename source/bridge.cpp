#include "srb2dbot/bridge.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

auto sanitize_message_for_srb2(const std::string& content) -> std::string {
    std::string result = content;
    for (size_t pos = 0; (pos = result.find('\n', pos)) != std::string::npos; pos += 2) {
        result.replace(pos, 1, "\\n");
    }

    std::string filtered;
    filtered.reserve(result.size());
    for (char c : result) {
        if (c >= 0x20 && c <= 0x7E) {
            filtered.push_back(c);
        }
    }

    std::string no_links = filtered;
    size_t link_pos = 0;
    while (link_pos < no_links.size()) {
        link_pos = no_links.find("http://", link_pos);
        if (link_pos == std::string::npos) {
            link_pos = no_links.find("https://", 0);
            if (link_pos == std::string::npos) break;
        }

        size_t word_start = link_pos;
        while (word_start > 0 && no_links[word_start - 1] != ' ') {
            word_start--;
        }
        size_t word_end = link_pos;
        while (word_end < no_links.size() && no_links[word_end] != ' ') {
            word_end++;
        }

        no_links.replace(word_start, word_end - word_start, "[LINK]");
        link_pos = word_start + 6;
    }

    return no_links;
}

auto bridge_get_lines(const std::string& path) -> std::size_t {
    std::ifstream file(path);
    if (!file.is_open()) return 0;
    std::string line;
    std::size_t count = 0;
    while (std::getline(file, line)) {
        ++count;
    }
    return count;
}

auto bridge_read_range(const std::string& path, std::size_t start, std::size_t end) -> std::string {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::string result;
    result.reserve(1024);
    std::string line;
    std::size_t line_number = 0;
    while (std::getline(file, line)) {
        ++line_number;
        if (line_number > start && line_number <= end) {
            result += line;
            result += '\n';
            if (result.size() > 1000) break;
        }
        if (line_number > end) break;
    }
    return result;
}

auto bridge_replace_emojis(const std::string& content, const std::unordered_map<std::string, std::string>& emojis) -> std::string {
    std::string result = content;
    for (const auto& [name, id] : emojis) {
        std::string pattern = ":" + name + ":";
        std::string replacement = "<:" + name + ":" + id + ">";
        size_t pos = 0;
        while ((pos = result.find(pattern, pos)) != std::string::npos) {
            result.replace(pos, pattern.size(), replacement);
            pos += replacement.size();
        }
    }
    return result;
}

auto bridge_parse_event(const std::string& line) -> std::optional<BridgeEvent> {
    if (line.rfind("[EVENT:", 0) != 0) return std::nullopt;

    BridgeEvent event;
    std::istringstream stream(line);
    std::string token;
    std::getline(stream, token, '|');
    std::string prefix = token;
    if (prefix.size() > 8 && prefix[prefix.size()-1] == ']') {
        event.type = prefix.substr(7, prefix.size() - 8);
    } else {
        event.type = prefix.substr(7);
    }
    while (std::getline(stream, token, '|')) {
        event.fields.push_back(token);
    }
    return event;
}

auto bridge_extract_thumbnail(const std::string& map, const std::string& outdir) -> void {
    char thumb[256];
    snprintf(thumb, sizeof(thumb), "%s/%s.png", outdir.c_str(), map.c_str());
    if (access(thumb, F_OK) == 0) return;

    pid_t pid = fork();
    if (pid == -1) return;
    if (pid != 0) return;

    std::string cmd =
        "for pk3 in \"$HOME\"/.srb2/addons/*.pk3; do "
        "  [ -f \"$pk3\" ] || continue; "
        "  if unzip -l \"$pk3\" 2>/dev/null | grep -q '" + map + "P.lmp'; then "
        "    unzip -p \"$pk3\" 'Level select pictures/" + map + "P.lmp' 2>/dev/null | "
        "    tail -c 16000 2>/dev/null | "
        "    if command -v magick >/dev/null 2>&1; then "
        "      magick -size 160x100 -depth 8 GRAY:- -auto-level '" + thumb + "' 2>/dev/null; "
        "    else "
        "      convert -size 160x100 -depth 8 GRAY:- -auto-level '" + thumb + "' 2>/dev/null; "
        "    fi; "
        "    break; "
        "  fi; "
        "done";
    execl("/bin/sh", "sh", "-c", cmd.c_str(), (char*)NULL);
    _exit(1);
}
