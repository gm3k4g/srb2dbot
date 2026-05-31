# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(srb2dbotTests LANGUAGES CXX)

set(TEST_SOURCES
    source/srb2dbot_test.cpp
    ${CMAKE_SOURCE_DIR}/source/utils.cpp
    ${CMAKE_SOURCE_DIR}/source/script.cpp
    ${CMAKE_SOURCE_DIR}/source/bridge.cpp
)

add_executable(srb2dbot_test ${TEST_SOURCES})
target_include_directories(srb2dbot_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_features(srb2dbot_test PRIVATE cxx_std_26)

add_test(NAME srb2dbot_test COMMAND srb2dbot_test)

add_folders(Test)
