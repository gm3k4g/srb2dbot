#include "srb2dbot/utils.hpp"
#include "srb2dbot/script.hpp"
#include <cstdlib>
#include <iostream>
#include <string>

static int failures = 0;

#define TEST(name) \
    std::cout << "  " << name << "... " << std::flush

#define CHECK(expr) \
    do { \
        if (!(expr)) { \
            std::cout << "FAIL [" << __LINE__ << "]: " #expr << "\n"; \
            failures++; \
        } \
    } while(0)

#define PASS() std::cout << "OK\n"

void test_sanitize_filename() {
    TEST("sanitize_filename: normal name");
    CHECK(sanitize_filename("mywad.pk3") == "mywad.pk3");
    PASS();

    TEST("sanitize_filename: path traversal ../");
    CHECK(sanitize_filename("../etc/passwd") == "passwd");
    PASS();

    TEST("sanitize_filename: absolute path");
    CHECK(sanitize_filename("/etc/passwd") == "passwd");
    PASS();

    TEST("sanitize_filename: relative parent");
    CHECK(sanitize_filename("foo/../../bar.pk3") == "bar.pk3");
    PASS();

    TEST("sanitize_filename: empty string");
    CHECK(sanitize_filename("") == "downloaded_file");
    PASS();

    TEST("sanitize_filename: dot only");
    CHECK(sanitize_filename(".") == "downloaded_file");
    PASS();

    TEST("sanitize_filename: dotdot only");
    CHECK(sanitize_filename("..") == "downloaded_file");
    PASS();

    TEST("sanitize_filename: just filename in subdir");
    CHECK(sanitize_filename("subdir/real.pk3") == "real.pk3");
    PASS();
}

void test_link_filename_str() {
    TEST("link_filename_str: simple URL");
    CHECK(link_filename_str("https://example.com/foo.pk3") == "foo.pk3");
    PASS();

    TEST("link_filename_str: URL with query string");
    CHECK(link_filename_str("https://example.com/foo.pk3?token=abc") == "foo.pk3");
    PASS();

    TEST("link_filename_str: URL with no filename");
    CHECK(link_filename_str("https://example.com/") == "downloaded_file");
    PASS();

    TEST("link_filename_str: nested path");
    CHECK(link_filename_str("https://example.com/a/b/c/maps.pk3") == "maps.pk3");
    PASS();
}

void test_script_inspect_str() {
    std::string content =
        "addfile maps.pk3\n"
        "password=secret\n"
        "addfile chars.pk3\n"
        "addfile textures.pk3\n"
        "addfile music.pk3\n"
        "maxplayers 16\n"
        "servername Test SRB2\n"
        "port 5029\n"
        "gametype coop\n"
        "sv_blamecam 1\n";

    TEST("script_inspect_str: target in range");
    std::string result = script_inspect_str(content, 3);
    CHECK(result.find("-> 3: addfile chars.pk3") != std::string::npos);
    CHECK(result.find("2:") != std::string::npos);
    CHECK(result.find("4:") != std::string::npos);
    PASS();

    TEST("script_inspect_str: target below 1 clamps to 1");
    result = script_inspect_str(content, 0);
    CHECK(result.find("-> 1: addfile maps.pk3") != std::string::npos);
    PASS();

    TEST("script_inspect_str: target beyond end shows range ending at max");
    result = script_inspect_str(content, 999);
    CHECK(result.find("-> ") != std::string::npos);
    PASS();
}

void test_script_find_str() {
    std::string content =
        "addfile maps.pk3\n"
        "password=secret\n"
        "addfile chars.pk3\n"
        "addfile textures.pk3\n";

    TEST("script_find_str: finds lines with maps");
    std::string result = script_find_str(content, "maps");
    CHECK(result.find("1: addfile maps.pk3") != std::string::npos);
    PASS();

    TEST("script_find_str: password lines hidden");
    result = script_find_str(content, "password");
    CHECK(result.find("password") == std::string::npos);
    PASS();

    TEST("script_find_str: no matches");
    result = script_find_str(content, "zzznomatch");
    CHECK(result.empty());
    PASS();

    TEST("script_find_str: finds multiple matches");
    result = script_find_str(content, "addfile");
    CHECK(result.find("1:") != std::string::npos);
    CHECK(result.find("3:") != std::string::npos);
    CHECK(result.find("4:") != std::string::npos);
    PASS();
}

void test_script_move_line_str() {
    std::string content =
        "alpha\n"
        "beta\n"
        "gamma\n"
        "delta\n"
        "epsilon\n";

    TEST("script_move_line_str: move down (2->4)");
    std::string result = script_move_line_str(content, 2, 4);
    CHECK(result == "alpha\ngamma\ndelta\nbeta\nepsilon\n");
    PASS();

    TEST("script_move_line_str: move up (4->2)");
    result = script_move_line_str(content, 4, 2);
    CHECK(result == "alpha\ndelta\nbeta\ngamma\nepsilon\n");
    PASS();

    TEST("script_move_line_str: move to same position (no-op)");
    result = script_move_line_str(content, 3, 3);
    CHECK(result == content);
    PASS();

    TEST("script_move_line_str: move first to last");
    result = script_move_line_str(content, 1, 5);
    CHECK(result == "beta\ngamma\ndelta\nepsilon\nalpha\n");
    PASS();

    TEST("script_move_line_str: move last to first");
    result = script_move_line_str(content, 5, 1);
    CHECK(result == "epsilon\nalpha\nbeta\ngamma\ndelta\n");
    PASS();
}

void test_script_add_line_str() {
    std::string content =
        "alpha\n"
        "beta\n"
        "gamma\n";

    TEST("script_add_line_str: insert at position 2");
    std::string result = script_add_line_str(content, "inserted", 2);
    CHECK(result == "alpha\ninserted\nbeta\ngamma\n");
    PASS();

    TEST("script_add_line_str: insert empty line");
    result = script_add_line_str(content, "", 3);
    CHECK(result == "alpha\nbeta\n\ngamma\n");
    PASS();

    TEST("script_add_line_str: insert at end");
    result = script_add_line_str(content, "final", 4);
    CHECK(result == "alpha\nbeta\ngamma\nfinal\n");
    PASS();
}

void test_script_remove_line_str() {
    std::string content =
        "alpha\n"
        "beta\n"
        "gamma\n";

    TEST("script_remove_line_str: remove middle");
    std::string result = script_remove_line_str(content, 2);
    CHECK(result == "alpha\ngamma\n");
    PASS();

    TEST("script_remove_line_str: remove first");
    result = script_remove_line_str(content, 1);
    CHECK(result == "beta\ngamma\n");
    PASS();

    TEST("script_remove_line_str: remove last");
    result = script_remove_line_str(content, 3);
    CHECK(result == "alpha\nbeta\n");
    PASS();

    TEST("script_remove_line_str: out of range does nothing");
    result = script_remove_line_str(content, 999);
    CHECK(result == content);
    PASS();
}

void test_script_change_line_str() {
    std::string content =
        "alpha\n"
        "beta\n"
        "gamma\n";

    TEST("script_change_line_str: change middle");
    std::string result = script_change_line_str(content, "CHANGED", 2);
    CHECK(result == "alpha\nCHANGED\ngamma\n");
    PASS();

    TEST("script_change_line_str: change first");
    result = script_change_line_str(content, "FIRST", 1);
    CHECK(result == "FIRST\nbeta\ngamma\n");
    PASS();

    TEST("script_change_line_str: change last");
    result = script_change_line_str(content, "LAST", 3);
    CHECK(result == "alpha\nbeta\nLAST\n");
    PASS();

    TEST("script_change_line_str: out of range does nothing");
    result = script_change_line_str(content, "NEW", 999);
    CHECK(result == content);
    PASS();
}

auto main() -> int {
    std::cout << "=== srb2dbot test suite ===\n\n";

    test_sanitize_filename();
    test_link_filename_str();
    test_script_inspect_str();
    test_script_find_str();
    test_script_move_line_str();
    test_script_add_line_str();
    test_script_remove_line_str();
    test_script_change_line_str();

    std::cout << "\n=== " << failures << " test(s) failed ===\n";
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
