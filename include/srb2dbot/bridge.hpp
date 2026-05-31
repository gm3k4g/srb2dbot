#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>

auto sanitize_message_for_srb2(const std::string& content) -> std::string;
auto bridge_get_lines(const std::string& path) -> std::size_t;
auto bridge_read_range(const std::string& path, std::size_t start, std::size_t end) -> std::string;
auto bridge_replace_emojis(const std::string& content, const std::unordered_map<std::string, std::string>& emojis) -> std::string;
