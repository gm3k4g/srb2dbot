#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

auto sanitize_message_for_srb2(const std::string& content) -> std::string;
auto bridge_get_lines(const std::string& path) -> std::size_t;
auto bridge_read_range(const std::string& path, std::size_t start, std::size_t end) -> std::string;
auto bridge_replace_emojis(const std::string& content, const std::unordered_map<std::string, std::string>& emojis) -> std::string;

struct BridgeEvent {
    std::string type;
    std::vector<std::string> fields;
};

auto bridge_parse_event(const std::string& line) -> std::optional<BridgeEvent>;

auto bridge_extract_thumbnail(const std::string& map, const std::string& outdir) -> void;
auto bridge_decode_patch(const std::string& lump_data, int width, int height) -> std::string;
