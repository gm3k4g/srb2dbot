#pragma once

#include <string>

auto script_inspect_str(const std::string& content, int target_line) -> std::string;
auto script_find_str(const std::string& content, const std::string& target_string) -> std::string;
auto script_move_line_str(const std::string& content, int old_line, int new_line) -> std::string;
auto script_add_line_str(const std::string& content, const std::string& line_content, int line_number) -> std::string;
auto script_remove_line_str(const std::string& content, int line_number) -> std::string;
auto script_change_line_str(const std::string& content, const std::string& new_line_content, int line_number) -> std::string;
