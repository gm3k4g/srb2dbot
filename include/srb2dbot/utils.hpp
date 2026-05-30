#pragma once

#include <string>

auto sanitize_filename(const std::string& raw) -> std::string;
auto link_filename_str(const std::string& url) -> std::string;
