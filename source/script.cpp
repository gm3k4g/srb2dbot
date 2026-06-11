#include "srb2dbot/script.hpp"
#include "srb2dbot/server.hpp"
#include <algorithm>
#include <sstream>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>

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
            new_script << (line_content.empty() ? "\n" : line_content + "\n");
        }
        new_script << line << "\n";
        i += 1;
        total_lines += 1;
    }
    if (line_number > total_lines) {
        new_script << (line_content.empty() ? "\n" : line_content + "\n");
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

// --- File I/O wrappers ---

static const std::array<std::string, 2> INVALID_ARRAY = {"Invalid filename", "Invalid content"};

auto script_get(std::ifstream& script_file) -> std::array<std::string, 2> {
    std::string script_path = dir_srb2_str();
    script_path.append("/srb2_servers.d/srb2b.d/srb2b.sh");
    script_file.open(script_path);
    if (!script_file.is_open()) {
        return INVALID_ARRAY;
    }
    return {{"srb2b.sh", script_path}};
}

auto script_get_str() -> std::array<std::string, 2> {
    std::ifstream script_file;
    auto script_content = script_get(script_file);
    if (!script_file) {
        return INVALID_ARRAY;
    }
    std::stringstream input_buffer;
    std::stringstream output_buffer;
    std::string line;
    input_buffer << script_file.rdbuf();
    while (std::getline(input_buffer, line)) {
        if (line.find("password") != std::string::npos) {
            output_buffer << "[REDACTED]\n";
        } else {
            output_buffer << line << "\n";
        }
    }
    return {script_content[0], output_buffer.str()};
}

auto script_write(const std::string& script_name, const std::string& script_contents) -> bool {
    std::ofstream script_file(script_name, std::ios::trunc);
    if (!script_file) return false;
    script_file << script_contents;
    return true;
}

auto script_validate() -> bool {
    std::ifstream script_buf;
    auto script_content = script_get(script_buf);
    std::string script_path = script_content[1];
    pid_t pid = fork();
    if (pid == -1) {
        return false;
    }
    if (pid == 0) {
        execlp("bash", "bash", "-n", script_path.c_str(), (char*)nullptr);
        _exit(EXIT_FAILURE);
    }
    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

auto script_inspect(std::string& script_content, int target_line) -> std::string {
    return script_inspect_str(script_content, target_line);
}

auto script_find(const std::string& target_string) -> std::string {
    std::ifstream script_file;
    auto script_content = script_get(script_file);
    if (!script_file) return {};
    std::stringstream buf;
    buf << script_file.rdbuf();
    return script_find_str(buf.str(), target_string);
}

auto script_add_line(const std::string& line_content, int line_number) -> bool {
    std::ifstream script_file;
    auto script_content = script_get(script_file);
    std::stringstream old_script;
    old_script << script_file.rdbuf();
    std::string result = script_add_line_str(old_script.str(), line_content, line_number);
    return script_write(script_content[1], result);
}

auto script_change_line(const std::string& line_content, int line_number) -> bool {
    std::ifstream script_file;
    auto script_content = script_get(script_file);
    std::stringstream old_script;
    old_script << script_file.rdbuf();
    std::string result = script_change_line_str(old_script.str(), line_content, line_number);
    return script_write(script_content[1], result);
}

auto script_move_line(int old_line, int new_line) -> bool {
    std::ifstream script_file;
    auto script_content = script_get(script_file);
    std::stringstream old_script;
    old_script << script_file.rdbuf();
    std::string result = script_move_line_str(old_script.str(), old_line, new_line);
    return script_write(script_content[1], result);
}

auto script_remove_line(int line_num) -> bool {
    std::ifstream script_file;
    auto script_content = script_get(script_file);
    std::stringstream old_script;
    old_script << script_file.rdbuf();
    std::string result = script_remove_line_str(old_script.str(), line_num);
    return script_write(script_content[1], result);
}
