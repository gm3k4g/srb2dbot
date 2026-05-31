#include "srb2dbot/bridge.hpp"
#include <cstdint>
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

auto bridge_decode_patch(const std::string& lump_data, int width, int height) -> std::string {
    if (lump_data.size() < 8) return "";

    std::string pixels(width * height, '\0');
    int col_offset_base = 8;
    int num_columns = width;

    for (int col = 0; col < num_columns; col++) {
        size_t off = col_offset_base + col * 4;
        if (off + 4 > lump_data.size()) break;
        uint32_t col_offset = (uint8_t)lump_data[off]
            | ((uint8_t)lump_data[off+1] << 8)
            | ((uint8_t)lump_data[off+2] << 16)
            | ((uint8_t)lump_data[off+3] << 24);
        if (col_offset == 0 || col_offset >= lump_data.size()) continue;

        size_t pos = col_offset;
        while (pos < lump_data.size()) {
            uint8_t topdelta = (uint8_t)lump_data[pos];
            if (topdelta == 0xFF) break;
            if (pos + 4 > lump_data.size()) break;

            uint8_t length = (uint8_t)lump_data[pos + 1];
            pos += 3;  // skip topdelta, length, unused byte

            int y = topdelta;
            for (int p = 0; p < length && pos < lump_data.size(); p++) {
                if (y < height) {
                    int pixel_idx = y * width + col;
                    if (pixel_idx < width * height) {
                        pixels[pixel_idx] = lump_data[pos];
                    }
                }
                pos++;
                y++;
            }
            pos++; // skip trailing unused byte
        }
    }
    return pixels;
}

auto bridge_extract_thumbnail(const std::string& map, const std::string& outdir) -> void {
    char thumb[256];
    snprintf(thumb, sizeof(thumb), "%s/%s.png", outdir.c_str(), map.c_str());
    if (access(thumb, F_OK) == 0) return;

    std::string lump_name = "Level select pictures/" + map + "P.lmp";
    std::string lump_data;
    std::string search_dir = std::string(getenv("HOME") ? getenv("HOME") : "") + "/.srb2/addons";

    std::string find_cmd = "for pk3 in \"" + search_dir + "\"/*.pk3; do "
        "[ -f \"$pk3\" ] || continue; "
        "if unzip -l \"$pk3\" 2>/dev/null | grep -q '" + lump_name + "'; then "
        "echo \"$pk3\"; break; fi; done";

    FILE* fp = popen(find_cmd.c_str(), "r");
    if (!fp) return;
    char pk3_buf[1024];
    if (!fgets(pk3_buf, sizeof(pk3_buf), fp)) { pclose(fp); return; }
    pclose(fp);
    size_t len = strlen(pk3_buf);
    while (len > 0 && (pk3_buf[len-1] == '\n' || pk3_buf[len-1] == '\r')) pk3_buf[--len] = 0;
    std::string pk3_path = pk3_buf;

    std::string unzip_cmd = "unzip -p \"" + pk3_path + "\" \"" + lump_name + "\" 2>/dev/null";
    fp = popen(unzip_cmd.c_str(), "r");
    if (!fp) return;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        lump_data.append(buf, n);
    }
    pclose(fp);
    if (lump_data.size() < 1000) return;

    // Approximate width/height from lump size: ~17448 bytes → 160x100
    int w = 160, h = 100;
    if (lump_data.size() > 20000) { w = 320; h = 200; }
    std::string pixels = bridge_decode_patch(lump_data, w, h);
    if (pixels.empty()) return;

    // Write PGM and convert
    std::string pgm_data = "P5\n" + std::to_string(w) + " " + std::to_string(h) + "\n255\n" + pixels;
    std::string pgm_path = "/tmp/.srb2_thumb.pgm";
    {
        std::ofstream pgm(pgm_path, std::ios::binary);
        pgm.write(pgm_data.c_str(), pgm_data.size());
    }

    std::string convert_cmd = "if command -v magick >/dev/null 2>&1; then magick '"
        + pgm_path + "' -auto-level '" + thumb + "'; else convert '"
        + pgm_path + "' -auto-level '" + thumb + "'; fi 2>/dev/null";
    if (system(convert_cmd.c_str()) != 0) {/* best effort */}
    remove(pgm_path.c_str());
}
