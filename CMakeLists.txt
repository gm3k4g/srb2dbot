# NOTES
# - Enable debug mode in CMake with `-D CMAKE_BUILD_TYPE=Debug`
#
# DEPENDENCIES
# - CPR (C++ Requests: Curl for People)
# - abseil-cpp (open-source collection of C++ library code)
cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

set(PROJECT_AUTHOR "gm3k4g")
project(
    srb2dbot
    VERSION 0.1
    DESCRIPTION "Control SRB2 servers via Discord."
    HOMEPAGE_URL ""
    LANGUAGES CXX
)

# Let's create a header file to use within the project for our variables.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/source/version.h.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/version.h"
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# Linux: include libraries
include_directories(
    "/usr/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
link_directories(/usr/lib)

# Collect all cpp and hpp files from `source`
file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/source/*.cpp)
file(GLOB HEADERS ${PROJECT_SOURCE_DIR}/source/*.hpp)

# TODO: Enable sanitizers if in debug mode
message(STATUS,"CMake project is building in: ${CMAKE_BUILD_TYPE}")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O1 -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")
endif()

# ---- Declare executable ----
add_executable(${CMAKE_PROJECT_NAME} ${SOURCES} ${HEADERS})

#set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY OUTPUT_NAME srb2dbot)
#target_compile_features(srb2dbot_exe PRIVATE cxx_std_17)

target_link_libraries(${PROJECT_NAME} dpp )
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
                           "${PROJECT_BINARY_DIR}"
)

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
