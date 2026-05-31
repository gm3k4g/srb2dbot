#include "srb2dbot/bridge.hpp"
#include <fstream>
#include <sstream>
#include <string>
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
