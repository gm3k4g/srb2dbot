#include "srb2dbot/utils.hpp"
#include <filesystem>
#include <string>

auto substitute_placeholders(const std::string& tmpl, const std::map<std::string, std::string>& params) -> std::string {
    std::string result = tmpl;
    for (auto& [key, val] : params) {
        std::string placeholder = "{" + key + "}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.size(), val);
            pos += val.size();
        }
    }
    return result;
}

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
