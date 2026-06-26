#include "srb2dbot/server.hpp"
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <pwd.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

auto dir_srb2_str() -> std::string {
    const char* env = std::getenv("SRB2DBOT_SRB2_HOME");
    if (env && env[0] != '\0') return std::string(env);
    struct passwd *pw = getpwuid(getuid());
    std::string homedir = pw ? pw->pw_dir : "/tmp";
    std::string script_path = homedir;
    script_path.append("/.srb2");
    return script_path;
}

static auto pipe_get() -> std::string {
    std::string srb2_dir = dir_srb2_str();
    std::string pipe_dir = srb2_dir
        .append("/srb2_servers.d")
        .append("/srb2b.d")
        .append("/srb2b.fifo");
    return pipe_dir;
}

auto pipe_write(const std::string& data) -> bool {
    std::string fifo = pipe_get();
    int fd = open(fifo.c_str(), O_WRONLY|O_NONBLOCK);
    if (fd == -1) {
#ifndef NDEBUG
        if (errno != ENOENT && errno != ENXIO) {
            perror("open");
        }
#endif
        return false;
    }
    const char* buf = data.c_str();
    size_t remaining = data.size();
    while (remaining > 0) {
        ssize_t bytes_written = write(fd, buf, remaining);
        if (bytes_written == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(1000);
                continue;
            }
#ifndef NDEBUG
            perror("write");
#endif
            close(fd);
            return false;
        }
        buf += bytes_written;
        remaining -= static_cast<size_t>(bytes_written);
    }
    close(fd);
    return true;
}

auto pipe_srb2_server_do(const std::string& data) -> bool {
    for (unsigned char c : data) {
        if (c == '\n' || c == '\r' || c == ';' || (c < 0x20 && c != '\t') || c > 0x7E) {
            return false;
        }
    }
    return pipe_write(data);
}

auto pipe_srb2_server_say(const std::string& msg) -> bool {
    for (unsigned char c : msg) {
        if (c == '\n' || c == '\r' || (c < 0x20 && c != '\t') || c > 0x7E) {
            return false;
        }
    }
    // Backslash-escape injection chars: \ → \\, ; → \;, ' → \'
    std::string escaped = msg;
    for (size_t i = 0; i < escaped.size(); ++i) {
        if (escaped[i] == '\\') {
            escaped.replace(i, 1, "\\\\");
            ++i;
        } else if (escaped[i] == ';') {
            escaped.replace(i, 1, "\\;");
            ++i;
        } else if (escaped[i] == '\'') {
            escaped.replace(i, 1, "\\'");
            ++i;
        }
    }
    return pipe_write("say '" + escaped + "'");
}

auto pipe_srb2_kick_player(const std::string& player) -> bool {
    for (unsigned char c : player) {
        if (c == '\n' || c == '\r' || c == ';' || c == '"' || (c < 0x20 && c != '\t') || c > 0x7E) {
            return false;
        }
    }
    return pipe_write("kick \"" + player + "\"");
}

auto pipe_srb2_ban_player(const std::string& player) -> bool {
    for (unsigned char c : player) {
        if (c == '\n' || c == '\r' || c == ';' || c == '"' || (c < 0x20 && c != '\t') || c > 0x7E) {
            return false;
        }
    }
    return pipe_write("ban \"" + player + "\"");
}

auto detect_fifo_support() -> bool {
    std::string srb2_dir = dir_srb2_str();
    std::string script_path = srb2_dir
        + "/srb2_servers.d/srb2b.d/srb2b.sh";
    std::ifstream infile(script_path);
    if (!infile.is_open()) return false;
    std::string line;
    while (std::getline(infile, line)) {
        if (line.find("srb2-fifo") != std::string::npos) return true;
    }
    return false;
}
