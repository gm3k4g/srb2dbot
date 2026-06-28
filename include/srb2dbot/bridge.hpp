#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

auto sanitize_message_for_srb2(const std::string& content) -> std::string;
auto sanitize_for_discord(const std::string& input) -> std::string;
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
auto bridge_get_loaded_pk3s() -> const std::vector<std::string>&;
void bridge_invalidate_pk3_cache();
auto bridge_find_pk3_for_lump(const std::string& lump_name) -> std::string;
auto bridge_get_map_source(const std::string& map) -> std::string;
auto bridge_get_gametypes_from_log() -> std::unordered_map<int, std::string>;
