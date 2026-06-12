# NOTES
# - Enable debug mode in CMake with `-D CMAKE_BUILD_TYPE=Debug`
cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

set(CMAKE_CXX_STANDARD 26)
set(PROJECT_AUTHOR "gm3k4g")
project(
    srb2dbot
    VERSION 0.1
    DESCRIPTION "Control SRB2 servers via Discord."
    HOMEPAGE_URL "https://github.com/gm3k4g/srb2dbot"
    LANGUAGES CXX
)

# Let's create a header file to use within the project for our variables.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/source/version.h.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/source/version.h"
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# Linux: include libraries
include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/source"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}/source"
)
# Collect source files
set(SOURCES
    source/main.cpp
    source/utils.cpp
    source/script.cpp
    source/bridge.cpp
    source/server.cpp
    source/module.cpp
    source/modules/script_module.cpp
    source/modules/wad_module.cpp
    source/modules/server_module.cpp
    source/modules/player_module.cpp
    source/modules/relay_module.cpp
    source/modules/round_start_card.cpp
    source/modules/round_end_card.cpp
    source/modules/flag_capture_card.cpp
    source/modules/flag_drop_card.cpp
    source/modules/flag_pickup_card.cpp
    source/modules/flag_return_card.cpp
    source/modules/player_join_card.cpp
    source/modules/player_quit_card.cpp
    source/modules/kick_player_card.cpp
    source/modules/ban_player_card.cpp
    source/modules/chat_card.cpp
    source/modules/csay_card.cpp
    source/modules/server_start_card.cpp
)

message(STATUS "CMake project is building in: ${CMAKE_BUILD_TYPE}")

# ---- Declare executable ----
add_executable(${CMAKE_PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} dpp)

# ---- Install rules ----
if(NOT CMAKE_SKIP_INSTALL_RULES)
    include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----
if(NOT srb2dbot_DEVELOPER_MODE)
    return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
    message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of srb2dbot"
  )
endif()

include(cmake/dev-mode.cmake)
