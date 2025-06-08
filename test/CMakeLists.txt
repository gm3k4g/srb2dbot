# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(srb2dbotTests LANGUAGES CXX)

# ---- Tests ----

add_executable(srb2dbot_test source/srb2dbot_test.cpp)
target_link_libraries(srb2dbot_test PRIVATE srb2dbot_lib)
target_compile_features(srb2dbot_test PRIVATE cxx_std_17)

add_test(NAME srb2dbot_test COMMAND srb2dbot_test)

# ---- End-of-file commands ----

add_folders(Test)
