cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    srb2dbot
    VERSION 0.1.0
    DESCRIPTION ""
    HOMEPAGE_URL ""
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# Linux: include libraries
include_directories(/usr/include)
link_directories(/usr/lib)

# Collect all cpp and hpp files from `source`
file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/source/*.cpp)
file(GLOB HEADERS ${PROJECT_SOURCE_DIR}/source/*.hpp)

# ---- Declare executable ----
add_executable(${CMAKE_PROJECT_NAME} ${SOURCES} ${HEADERS})

#set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY OUTPUT_NAME srb2dbot)
#target_compile_features(srb2dbot_exe PRIVATE cxx_std_17)

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
