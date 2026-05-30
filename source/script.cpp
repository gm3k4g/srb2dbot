#include "srb2dbot/script.hpp"
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

auto script_inspect_str(const std::string& content, int target_line) -> std::string {
    std::istringstream split_lines(content);
    std::string line;

    int size = 0;
    while (std::getline(split_lines, line)) {
        size += 1;
    }
    split_lines.clear();
    split_lines.seekg(0);

    int targeted_line = target_line;
    if (target_line < 1) {
        targeted_line = 1;
    } else if (target_line > size) {
        targeted_line = size;
    }
    const int range = 8;
    int start = std::max(1, targeted_line - range - 1);
    int end = std::min(size, targeted_line + range);

    std::stringstream inspected_region;
    inspected_region << "```Showing line " << start << " to " << end << "```\n";
    inspected_region << "```";
    int i = 1;
    while (std::getline(split_lines, line)) {
        if (i >= start && i <= end) {
            if (i == targeted_line) {
                inspected_region << "\n-> " << i << ": " << line << "\n\n";
            } else {
                inspected_region << i << ": " << line << "\n";
            }
        }
        i += 1;
    }
    inspected_region << "```";

    return inspected_region.str();
}

auto script_find_str(const std::string& content, const std::string& target_string) -> std::string {
    std::istringstream split_lines(content);
    std::stringstream output;
    std::string line;
    int i = 1;
    while (std::getline(split_lines, line)) {
        if (line.find("password") != std::string::npos) {
            // skip password lines
        } else if (line.find(target_string) != std::string::npos) {
            output << i << ": " << line << "\n";
        }
        i += 1;
    }
    return output.str();
}

auto script_move_line_str(const std::string& content, int old_line, int new_line) -> std::string {
    std::istringstream split_lines(content);
    std::stringstream new_script;
    std::string line;
    std::string held_line;

    int i = 1;
    while (std::getline(split_lines, line)) {
        if (old_line == i) {
            held_line = line;
            break;
        }
        i += 1;
    }

    split_lines.clear();
    split_lines.seekg(0);
    i = 1;

    while (std::getline(split_lines, line)) {
        if (new_line == i) {
            if (old_line < new_line) {
                new_script << line << "\n" << held_line << "\n";
            } else {
                new_script << held_line << "\n";
                if (old_line != new_line) {
                    new_script << line << "\n";
                }
            }
        } else if (old_line == i) {
            // skip old line
        } else {
            new_script << line << "\n";
        }
        i += 1;
    }

    return new_script.str();
}

auto script_add_line_str(const std::string& content, const std::string& line_content, int line_number) -> std::string {
    std::istringstream split_lines(content);
    std::stringstream new_script;
    std::string line;
    int i = 1;
    int total_lines = 0;
    while (std::getline(split_lines, line)) {
        if (i == line_number) {
            if (line_content.empty()) {
                new_script << "\n";
            } else {
                new_script << line_content << "\n";
            }
        }
        new_script << line << "\n";
        i += 1;
        total_lines += 1;
    }
    if (line_number > total_lines) {
        if (line_content.empty()) {
            new_script << "\n";
        } else {
            new_script << line_content << "\n";
        }
    }
    return new_script.str();
}

auto script_remove_line_str(const std::string& content, int line_number) -> std::string {
    std::istringstream split_lines(content);
    std::stringstream new_script;
    std::string line;
    int i = 1;
    while (std::getline(split_lines, line)) {
        if (line_number != i) {
            new_script << line << "\n";
        }
        i += 1;
    }
    return new_script.str();
}

auto script_change_line_str(const std::string& content, const std::string& new_line_content, int line_number) -> std::string {
    std::istringstream split_lines(content);
    std::stringstream new_script;
    std::string line;
    int i = 1;
    while (std::getline(split_lines, line)) {
        if (line_number == i) {
            new_script << new_line_content << "\n";
        } else {
            new_script << line << "\n";
        }
        i += 1;
    }
    return new_script.str();
}
