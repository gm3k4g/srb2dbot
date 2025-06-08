cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    srb2dbot
    VERSION 0.1.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    srb2dbot_lib OBJECT
    source/lib.cpp
)

target_include_directories(
    srb2dbot_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(srb2dbot_lib PUBLIC cxx_std_17)

# ---- Declare executable ----

add_executable(srb2dbot_exe source/main.cpp)
add_executable(srb2dbot::exe ALIAS srb2dbot_exe)

set_property(TARGET srb2dbot_exe PROPERTY OUTPUT_NAME srb2dbot)

target_compile_features(srb2dbot_exe PRIVATE cxx_std_17)

target_link_libraries(srb2dbot_exe PRIVATE srb2dbot_lib)

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
