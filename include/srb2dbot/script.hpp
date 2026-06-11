#pragma once

#include <array>
#include <fstream>
#include <string>

// Pure string manipulation (no file I/O)
auto script_inspect_str(const std::string& content, int target_line) -> std::string;
auto script_find_str(const std::string& content, const std::string& target_string) -> std::string;
auto script_move_line_str(const std::string& content, int old_line, int new_line) -> std::string;
auto script_add_line_str(const std::string& content, const std::string& line_content, int line_number) -> std::string;
auto script_remove_line_str(const std::string& content, int line_number) -> std::string;
auto script_change_line_str(const std::string& content, const std::string& new_line_content, int line_number) -> std::string;

// File I/O wrappers (works with srb2b.sh on disk)
auto script_get(std::ifstream& script_file) -> std::array<std::string, 2>;
auto script_get_str() -> std::array<std::string, 2>;
auto script_write(const std::string& script_name, const std::string& script_contents) -> bool;
auto script_validate() -> bool;
auto script_inspect(std::string& script_content, int target_line) -> std::string;
auto script_find(const std::string& target_string) -> std::string;
auto script_add_line(const std::string& line_content, int line_number) -> bool;
auto script_change_line(const std::string& line_content, int line_number) -> bool;
auto script_move_line(int old_line, int new_line) -> bool;
auto script_remove_line(int line_num) -> bool;
