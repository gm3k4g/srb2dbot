#include "srb2dbot/utils.hpp"
#include "srb2dbot/script.hpp"
#include "srb2dbot/bridge.hpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

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

void test_sanitize_message_for_srb2() {
    TEST("bridge: normal ASCII message");
    CHECK(sanitize_message_for_srb2("hello world") == "hello world");
    PASS();

    TEST("bridge: strips non-ASCII characters");
    CHECK(sanitize_message_for_srb2("hello\x01world") == "helloworld");
    PASS();

    TEST("bridge: strips Unicode emoji");
    std::string emoji_msg = "hi \xf0\x9f\x98\x80 there";
    std::string result = sanitize_message_for_srb2(emoji_msg);
    CHECK(result == "hi  there");
    PASS();

    TEST("bridge: escapes newlines");
    CHECK(sanitize_message_for_srb2("line1\nline2") == "line1\\nline2");
    PASS();

    TEST("bridge: replaces http link with [LINK]");
    std::string result2 = sanitize_message_for_srb2("check https://example.com now");
    CHECK(result2 == "check [LINK] now");
    PASS();

    TEST("bridge: replaces http link mid-word stays as word boundary");
    result2 = sanitize_message_for_srb2("go to https://x.com/path");
    CHECK(result2 == "go to [LINK]");
    PASS();

    TEST("bridge: multiple links replaced");
    result2 = sanitize_message_for_srb2("a http://a.com b https://b.com c");
    CHECK(result2 == "a [LINK] b [LINK] c");
    PASS();

    TEST("bridge: empty input returns empty");
    CHECK(sanitize_message_for_srb2("") == "");
    PASS();

    TEST("bridge: preserves original characters");
    CHECK(sanitize_message_for_srb2("  hello   world  ") == "  hello   world  ");
    PASS();
}

void test_bridge_get_lines() {
    std::string tmp = "/tmp/srb2dbot_test_getlines.txt";
    {
        std::ofstream f(tmp);
        f << "line1\nline2\nline3\n";
    }
    TEST("bridge_get_lines: counts 3 lines");
    CHECK(bridge_get_lines(tmp) == 3);
    PASS();

    std::remove(tmp.c_str());

    TEST("bridge_get_lines: non-existent file returns 0");
    CHECK(bridge_get_lines("/tmp/srb2dbot_nonexistent_xyz.txt") == 0);
    PASS();
}

void test_bridge_read_range() {
    std::string tmp = "/tmp/srb2dbot_test_readrange.txt";
    {
        std::ofstream f(tmp);
        f << "a\nb\nc\nd\ne\n";
    }
    TEST("bridge_read_range: reads middle lines");
    std::string result = bridge_read_range(tmp, 1, 4);
    CHECK(result == "b\nc\nd\n");
    PASS();

    TEST("bridge_read_range: start 0 reads from first line");
    result = bridge_read_range(tmp, 0, 2);
    CHECK(result == "a\nb\n");
    PASS();

    TEST("bridge_read_range: end beyond file");
    result = bridge_read_range(tmp, 3, 99);
    CHECK(result == "d\ne\n");
    PASS();

    TEST("bridge_read_range: start equals end returns empty");
    result = bridge_read_range(tmp, 2, 2);
    CHECK(result == "");
    PASS();

    std::remove(tmp.c_str());
}

void test_bridge_replace_emojis() {
    std::unordered_map<std::string, std::string> emojis;
    emojis["smile"] = "12345";
    emojis["wave"] = "67890";

    TEST("bridge_replace_emojis: replaces :smile:");
    CHECK(bridge_replace_emojis("hello :smile: world", emojis) == "hello <:smile:12345> world");
    PASS();

    TEST("bridge_replace_emojis: multiple emoji replacements");
    CHECK(bridge_replace_emojis(":wave: hello :smile:", emojis) == "<:wave:67890> hello <:smile:12345>");
    PASS();

    TEST("bridge_replace_emojis: no emojis returns unchanged");
    CHECK(bridge_replace_emojis("plain text", emojis) == "plain text");
    PASS();

    TEST("bridge_replace_emojis: empty map returns unchanged");
    std::unordered_map<std::string, std::string> empty;
    CHECK(bridge_replace_emojis(":smile: test", empty) == ":smile: test");
    PASS();
}

void test_bridge_polling_cycle() {
    std::string tmp = "/tmp/srb2dbot_test_poll.txt";

    TEST("bridge_integration: empty file has zero lines");
    {
        std::ofstream f(tmp, std::ios::trunc);
    }
    CHECK(bridge_get_lines(tmp) == 0);
    PASS();

    TEST("bridge_integration: detect new lines via seek tracking");
    {
        std::ofstream f(tmp, std::ios::trunc);
    }
    {
        size_t seek = bridge_get_lines(tmp);
        {
            std::ofstream f(tmp, std::ios::app);
            f << "Player1: hello\n";
        }
        size_t new_seek = bridge_get_lines(tmp);
        CHECK(new_seek > seek);
        std::string content = bridge_read_range(tmp, seek, new_seek);
        CHECK(content == "Player1: hello\n");
    }
    PASS();

    TEST("bridge_integration: multiple incremental polls");
    {
        std::ofstream f(tmp, std::ios::trunc);
        f << "line1\n";
    }
    {
        size_t s = 0;
        {
            std::ofstream f(tmp, std::ios::app);
            f << "line2\nline3\n";
        }
        size_t e = bridge_get_lines(tmp);
        std::string content = bridge_read_range(tmp, s, e);
        CHECK(content == "line1\nline2\nline3\n");
    }
    PASS();

    TEST("bridge_integration: emoji conversion on full message");
    {
        std::ofstream f(tmp, std::ios::trunc);
    }
    {
        std::ofstream f(tmp, std::ios::app);
        f << ":smile: hello :wave: world\n";
    }
    size_t s = 0;
    {
        std::ofstream f(tmp, std::ios::app);
        f << ":smile: again\n";
    }
    size_t e = bridge_get_lines(tmp);
    std::string content = bridge_read_range(tmp, s, e);
    std::unordered_map<std::string, std::string> emojis;
    emojis["smile"] = "111";
    emojis["wave"] = "222";
    std::string converted = bridge_replace_emojis(content, emojis);
    CHECK(converted == "<:smile:111> hello <:wave:222> world\n<:smile:111> again\n");
    PASS();

    TEST("bridge_integration: character cap within read_range");
    {
        std::string big_line(1200, 'x');
        {
            std::ofstream f(tmp, std::ios::trunc);
            f << big_line << "\n";
        }
        std::string capped = bridge_read_range(tmp, 0, 1);
        CHECK(!capped.empty());
    }
    PASS();

    std::remove(tmp.c_str());
}

void test_bridge_parse_event() {
    TEST("bridge_parse_event: returns nullopt for plain chat");
    CHECK(!bridge_parse_event("Player1: hello world").has_value());
    PASS();

    TEST("bridge_parse_event: parses SERVER_START");
    auto ev = bridge_parse_event("[EVENT:SERVER_START]|My Server|MAP01|Greenflower Zone");
    CHECK(ev.has_value());
    CHECK(ev->type == "SERVER_START");
    CHECK(ev->fields.size() == 3);
    CHECK(ev->fields[0] == "My Server");
    PASS();

    TEST("bridge_parse_event: parses ROUND_END with teams");
    ev = bridge_parse_event("[EVENT:ROUND_END]|CTF|TEAM:Red:3|TEAM:Blue:1");
    CHECK(ev.has_value());
    CHECK(ev->type == "ROUND_END");
    CHECK(ev->fields.size() == 3);
    CHECK(ev->fields[1] == "TEAM:Red:3");
    PASS();

    TEST("bridge_parse_event: parses CTF_CAPTURE");
    ev = bridge_parse_event("[EVENT:CTF_CAPTURE]|PlayerOne|Blue");
    CHECK(ev.has_value());
    CHECK(ev->type == "CTF_CAPTURE");
    CHECK(ev->fields[0] == "PlayerOne");
    CHECK(ev->fields[1] == "Blue");
    PASS();

    TEST("bridge_parse_event: parses PLAYER_JOIN");
    ev = bridge_parse_event("[EVENT:PLAYER_JOIN]|NewPlayer");
    CHECK(ev.has_value());
    CHECK(ev->type == "PLAYER_JOIN");
    CHECK(ev->fields[0] == "NewPlayer");
    PASS();

    TEST("bridge_parse_event: parses MAP_CHANGE");
    ev = bridge_parse_event("[EVENT:MAP_CHANGE]|MAP02|Flame Rift Zone");
    CHECK(ev.has_value());
    CHECK(ev->type == "MAP_CHANGE");
    CHECK(ev->fields[0] == "MAP02");
    CHECK(ev->fields[1] == "Flame Rift Zone");
    PASS();

    TEST("bridge_parse_event: ROUND_END with teams and player red/blue");
    ev = bridge_parse_event("[EVENT:ROUND_END]|CTF|TEAM:Red:3|RED:Alpha:2|RED:Beta:1|TEAM:Blue:1|BLUE:Gamma:1");
    CHECK(ev.has_value());
    CHECK(ev->type == "ROUND_END");
    CHECK(ev->fields.size() == 6);
    size_t col2 = ev->fields[1].find(':');
    CHECK(ev->fields[1].substr(0, col2) == "TEAM");
    col2 = ev->fields[2].find(':');
    CHECK(ev->fields[2].substr(0, col2) == "RED");
    col2 = ev->fields[4].find(':');
    CHECK(ev->fields[4].substr(0, col2) == "TEAM");
    col2 = ev->fields[5].find(':');
    CHECK(ev->fields[5].substr(0, col2) == "BLUE");
    PASS();
}

auto main() -> int {
    std::cout << "=== srb2dbot test suite ===\n\n";

    test_sanitize_filename();
    test_link_filename_str();
    test_sanitize_message_for_srb2();
    test_bridge_get_lines();
    test_bridge_read_range();
    test_bridge_replace_emojis();
    test_bridge_polling_cycle();
    test_bridge_parse_event();
    test_script_inspect_str();
    test_script_find_str();
    test_script_move_line_str();
    test_script_add_line_str();
    test_script_remove_line_str();
    test_script_change_line_str();

    std::cout << "\n=== " << failures << " test(s) failed ===\n";
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
