#include "srb2dbot/utils.hpp"
#include <filesystem>
#include <string>

auto link_filename_str(const std::string& url) -> std::string {
    std::string base = url.substr(0, url.find('?'));
    std::string filename = std::filesystem::path(base).filename().string();
    return filename.empty() ? "downloaded_file" : filename;
}

auto sanitize_filename(const std::string& raw) -> std::string {
    std::string filename = std::filesystem::path(raw).filename().string();
    if (filename.empty() || filename == "." || filename == "..") {
        return "downloaded_file";
    }
    return filename;
}
