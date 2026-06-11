#pragma once

#include <string>

auto dir_srb2_str() -> std::string;
auto detect_fifo_support() -> bool;
auto pipe_write(const std::string& data) -> bool;
auto pipe_srb2_server_do(const std::string& data) -> bool;
auto pipe_srb2_server_say(const std::string& msg) -> bool;
auto pipe_srb2_kick_player(const std::string& player) -> bool;
auto pipe_srb2_ban_player(const std::string& player) -> bool;
