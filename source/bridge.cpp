#include "srb2dbot/bridge.hpp"
#include <string>

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
