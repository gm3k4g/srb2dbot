#pragma once

#include <string>
#include <map>

auto sanitize_filename(const std::string& raw) -> std::string;
auto link_filename_str(const std::string& url) -> std::string;
auto substitute_placeholders(const std::string& tmpl, const std::map<std::string, std::string>& params) -> std::string;
