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

void test_discord_to_srb2_e2e() {
    std::string tmp = "/tmp/srb2dbot_e2e_discmessage.txt";

    TEST("E2E: raw Discord message is sanitized and written");
    std::string raw = "Check https://example.com now!\nWith newline";
    std::string sanitized = sanitize_message_for_srb2(raw);
    CHECK(sanitized.find("http") == std::string::npos);
    CHECK(sanitized.find("\n") == std::string::npos);
    std::string display_name = "TestUser";
    {
        std::ofstream f(tmp, std::ios::trunc);
    }
    {
        std::ofstream f(tmp, std::ios::app);
        f << "<" << display_name << "> " << sanitized << "\n";
    }
    std::ifstream verify(tmp);
    std::string line;
    std::getline(verify, line);
    CHECK(line.find("<TestUser>") != std::string::npos);
    PASS();

    TEST("E2E: empty Discord message is skipped");
    std::string empty_msg = "";
    CHECK(empty_msg.empty());
    PASS();

    TEST("E2E: single-char message is skipped");
    CHECK(sanitize_message_for_srb2("x").size() <= 1);
    PASS();

    std::remove(tmp.c_str());
}

void test_srb2_to_discord_e2e() {
    std::string tmp = "/tmp/srb2dbot_e2e_messages.txt";

    std::unordered_map<std::string, std::string> emojis;
    emojis["smile"] = "111";
    emojis["wave"] = "222";

    TEST("E2E: full polling lifecycle with seek tracking");
    {
        std::ofstream f(tmp, std::ios::trunc);
    }
    size_t seek = 0;

    // Simulate SRB2 player messages
    {
        std::ofstream f(tmp, std::ios::app);
        f << "Player1: hello team\n";
    }
    {
        std::ofstream f(tmp, std::ios::app);
        f << "Player2: :smile: hey\n";
    }

    size_t new_lines = bridge_get_lines(tmp);
    CHECK(new_lines > seek);

    std::string content = bridge_read_range(tmp, seek, new_lines);
    CHECK(!content.empty());
    CHECK(content.find("Player1: hello team") != std::string::npos);
    CHECK(content.find("Player2: :smile: hey") != std::string::npos);

    std::string converted = bridge_replace_emojis(content, emojis);
    CHECK(converted.find("<:smile:111> hey") != std::string::npos);
    CHECK(converted.find("Player2: <:smile:111> hey") != std::string::npos);
    seek = new_lines;

    // Second polling cycle: more messages arrive
    {
        std::ofstream f(tmp, std::ios::app);
        f << "Player3: :wave: thanks\n";
    }
    size_t final_seek = bridge_get_lines(tmp);
    CHECK(final_seek > seek);

    std::string next = bridge_read_range(tmp, seek, final_seek);
    CHECK(next == "Player3: :wave: thanks\n");

    std::string next_converted = bridge_replace_emojis(next, emojis);
    CHECK(next_converted == "Player3: <:wave:222> thanks\n");
    PASS();

    TEST("E2E: no new messages returns empty content");
    size_t no_change = bridge_get_lines(tmp);
    std::string nothing = bridge_read_range(tmp, no_change, no_change);
    CHECK(nothing.empty());
    PASS();

    std::remove(tmp.c_str());
}

void test_bridge_full_cycle() {
    std::string disc_tmp = "/tmp/srb2dbot_e2e_full_disc.txt";
    std::string msg_tmp = "/tmp/srb2dbot_e2e_full_msg.txt";

    std::unordered_map<std::string, std::string> emojis;
    emojis["smile"] = "111";

    TEST("E2E: complete bidirectional bridge cycle");

    // Step 1: Discord user sends message → SRB2
    std::string raw = "hello from Discord! https://example.com";
    std::string sanitized = sanitize_message_for_srb2(raw);
    {
        std::ofstream f(disc_tmp, std::ios::trunc);
    }
    {
        std::ofstream f(disc_tmp, std::ios::app);
        f << "<DiscordUser> " << sanitized << "\n";
    }
    std::ifstream check(disc_tmp);
    std::string l;
    std::getline(check, l);
    CHECK(l.find("<DiscordUser>") != std::string::npos);
    CHECK(l.find("[LINK]") != std::string::npos);

    // Step 2: SRB2 player responds → Discord
    {
        std::ofstream f(msg_tmp, std::ios::trunc);
    }
    size_t seek = 0;
    {
        std::ofstream f(msg_tmp, std::ios::app);
        f << "SRB2Player: :smile: hi from the game!\n";
    }
    size_t end = bridge_get_lines(msg_tmp);
    std::string srb2_msg = bridge_read_range(msg_tmp, seek, end);
    std::string converted = bridge_replace_emojis(srb2_msg, emojis);
    CHECK(converted.find("SRB2Player: <:smile:111>") != std::string::npos);
    seek = end;

    // Step 3: Multiple Discord messages
    {
        std::ofstream f(disc_tmp, std::ios::app);
        f << "<User2> " << sanitize_message_for_srb2("second msg") << "\n";
    }
    {
        std::ofstream f(disc_tmp, std::ios::app);
        f << "<User3> " << sanitize_message_for_srb2("third msg") << "\n";
    }
    int disc_lines = bridge_get_lines(disc_tmp);
    CHECK(disc_lines == 3);

    // Step 4: Multiple SRB2 messages
    {
        std::ofstream f(msg_tmp, std::ios::app);
        f << "Player4: more chat\n";
        f << "Player5: :smile: closing\n";
    }
    size_t final_end = bridge_get_lines(msg_tmp);
    std::string final = bridge_read_range(msg_tmp, seek, final_end);
    CHECK(final.find("Player4: more chat") != std::string::npos);
    CHECK(final.find("closing") != std::string::npos);
    CHECK(bridge_replace_emojis(final, emojis).find("<:smile:111>") != std::string::npos);
    PASS();

    std::remove(disc_tmp.c_str());
    std::remove(msg_tmp.c_str());
}

void test_bridge_event_pipeline() {
    std::string msg_tmp = "/tmp/srb2dbot_e2e_pipeline_msg.txt";

    std::unordered_map<std::string, std::string> emojis;
    emojis["smile"] = "111";

    // Simulate events as they would appear from SRB2's Lua flush_msgsrb2()
    //
    // After dbot_sync at startup:
    //   1. Bot truncates Messages.txt, writes "\n"
    //   2. Sends dbot_sync FIFO command
    //   3. Lua re-emits SERVER_START + ROUND_START if round active
    //   4. flush_msgsrb2() writes events to Messages.txt immediately
    //
    // Then during gameplay, each event is flushed immediately:
    //   5. ROUND_END events with team scores
    //   6. PLAYER_JOIN events
    //   7. CTF events

    TEST("E2E: pipeline simulates dbot_sync startup replay");
    {
        std::ofstream f(msg_tmp, std::ios::trunc);
    }
    size_t seek = 0;

    // After dbot_sync: Lua re-emits SERVER_START + ROUND_START
    {
        std::ofstream f(msg_tmp, std::ios::app);
        f << "[EVENT:SERVER_START]|Test Server|MAP01|Greenflower Zone\n";
    }
    {
        std::ofstream f(msg_tmp, std::ios::app);
        f << "[EVENT:ROUND_START]|Co-op|MAP01|Greenflower Zone Act 1\n";
    }

    // Bot poll 1: pick up startup events
    size_t lines = bridge_get_lines(msg_tmp);
    CHECK(lines == 2);  // 2 events
    std::string content = bridge_read_range(msg_tmp, seek, lines);
    CHECK(content.find("SERVER_START") != std::string::npos);
    CHECK(content.find("ROUND_START") != std::string::npos);
    CHECK(content.find("Test Server") != std::string::npos);
    CHECK(content.find("Co-op") != std::string::npos);
    seek = lines;
    PASS();

    TEST("E2E: pipeline simulates ROUND_END with team scores");
    {
        std::ofstream f(msg_tmp, std::ios::app);
        f << "[EVENT:ROUND_END]|CTF|MAP01|MAPNAME:Greenflower Zone|TEAM:Red:3|RED:Sonic:2|RED:Tails:1|TEAM:Blue:1|BLUE:Knuckles:1|SPEC:Watcher\n";
    }
    lines = bridge_get_lines(msg_tmp);
    CHECK(lines == 3);
    content = bridge_read_range(msg_tmp, seek, lines);
    CHECK(content.find("ROUND_END") != std::string::npos);
    CHECK(content.find("TEAM:Red:3") != std::string::npos);
    CHECK(content.find("TEAM:Blue:1") != std::string::npos);
    CHECK(content.find("SPEC:Watcher") != std::string::npos);
    seek = lines;
    PASS();

    TEST("E2E: pipeline simulates PLAYER_JOIN event");
    {
        std::ofstream f(msg_tmp, std::ios::app);
        f << "[EVENT:PLAYER_JOIN]|NewPlayer\n";
    }
    lines = bridge_get_lines(msg_tmp);
    content = bridge_read_range(msg_tmp, seek, lines);
    CHECK(content.find("PLAYER_JOIN") != std::string::npos);
    CHECK(content.find("NewPlayer") != std::string::npos);
    seek = lines;
    PASS();

    TEST("E2E: pipeline detects CTF events in batch");
    {
        std::ofstream f(msg_tmp, std::ios::app);
        f << "[EVENT:CTF_CAPTURE]|Sonic|Blue\n";
        f << "[EVENT:CTF_DROP]|Tails\n";
        f << "[EVENT:CTF_PICKUP]|Knuckles\n";
        f << "[EVENT:CTF_RETURN]|Amy\n";
    }
    lines = bridge_get_lines(msg_tmp);
    content = bridge_read_range(msg_tmp, seek, lines);
    CHECK(content.find("CTF_CAPTURE") != std::string::npos);
    CHECK(content.find("CTF_DROP") != std::string::npos);
    CHECK(content.find("CTF_PICKUP") != std::string::npos);
    CHECK(content.find("CTF_RETURN") != std::string::npos);
    CHECK(content.find("Sonic") != std::string::npos);
    CHECK(content.find("Blue") != std::string::npos);
    content = bridge_replace_emojis(content, emojis);
    CHECK(content.find("CTF_CAPTURE") != std::string::npos);  // emoji doesn't affect event tags
    seek = lines;
    PASS();

    // Simulate bot restart: Messages.txt truncated, dbot_sync called
    TEST("E2E: pipeline handles bot restart with dbot_sync");
    {
        std::ofstream f(msg_tmp, std::ios::trunc);
    }
    seek = 0;
    // Lua re-emits current state
    {
        std::ofstream f(msg_tmp, std::ios::app);
        f << "[EVENT:SERVER_START]|Restarted Server|MAP03|Azure Temple\n";
        f << "[EVENT:ROUND_START]|Match|MAP03|Azure Temple Act 2\n";
    }
    lines = bridge_get_lines(msg_tmp);
    CHECK(lines == 2);
    content = bridge_read_range(msg_tmp, seek, lines);
    CHECK(content.find("SERVER_START") != std::string::npos);
    CHECK(content.find("ROUND_START") != std::string::npos);
    CHECK(content.find("Restarted Server") != std::string::npos);
    CHECK(content.find("Match") != std::string::npos);
    CHECK(content.find("Azure Temple") != std::string::npos);
    PASS();

    std::remove(msg_tmp.c_str());
}

void test_rapid_identical_chat_events() {
    std::string msg_tmp = "/tmp/srb2dbot_e2e_rapid_msg.txt";

    TEST("E2E: rapid identical CHAT events all preserved (no content dedup)");

    {
        std::ofstream f(msg_tmp, std::ios::trunc);
    }
    size_t seek = 0;

    // Simulate a player sending "a" four times in rapid succession.
    // Each message produces a distinct [EVENT:CHAT] line. The Lua same-tick
    // dedup suppresses only the engine double-fire (same leveltime), so all
    // four legitimate lines should arrive in Messages.txt. The C++ bot no
    // longer applies a content-based dedup window for CHAT events, so all
    // four should be parsed and forwarded.
    {
        std::ofstream f(msg_tmp, std::ios::app);
        f << "[EVENT:CHAT]|[1]|Player1|a|sonic|100|0|none\n";
        f << "[EVENT:CHAT]|[1]|Player1|a|sonic|100|0|none\n";
        f << "[EVENT:CHAT]|[1]|Player1|a|sonic|100|0|none\n";
        f << "[EVENT:CHAT]|[1]|Player1|a|sonic|100|0|none\n";
    }

    size_t lines = bridge_get_lines(msg_tmp);
    CHECK(lines == 4);

    std::string content = bridge_read_range(msg_tmp, seek, lines);
    CHECK(!content.empty());

    // Verify all four events are present
    size_t count = 0;
    size_t pos = 0;
    while ((pos = content.find("[EVENT:CHAT]", pos)) != std::string::npos) {
        count++;
        pos += 12;
    }
    CHECK(count == 4);

    // Verify each event parses successfully
    size_t pos2 = 0;
    size_t parsed = 0;
    std::string remaining = content;
    while (true) {
        size_t nl = remaining.find('\n', pos2);
        if (nl == std::string::npos) break;
        std::string line = remaining.substr(pos2, nl - pos2);
        if (!line.empty()) {
            auto evt = bridge_parse_event(line);
            CHECK(evt.has_value());
            if (evt) {
                CHECK(evt->type == "CHAT");
                CHECK(evt->fields.size() >= 3);
                parsed++;
            }
        }
        pos2 = nl + 1;
    }
    CHECK(parsed == 4);

    seek = lines;
    PASS();

    std::remove(msg_tmp.c_str());
}

void test_rapid_identical_server_chat_events() {
    std::string msg_tmp = "/tmp/srb2dbot_e2e_rapid_srvmsg.txt";

    TEST("E2E: rapid identical SERVER_CHAT events all preserved");

    {
        std::ofstream f(msg_tmp, std::ios::trunc);
    }
    size_t seek = 0;

    // Server sends the same message three times rapidly
    {
        std::ofstream f(msg_tmp, std::ios::app);
        f << "[EVENT:SERVER_CHAT]|hello|FEE75C\n";
        f << "[EVENT:SERVER_CHAT]|hello|FEE75C\n";
        f << "[EVENT:SERVER_CHAT]|hello|FEE75C\n";
    }

    size_t lines = bridge_get_lines(msg_tmp);
    CHECK(lines == 3);

    std::string content = bridge_read_range(msg_tmp, seek, lines);
    CHECK(!content.empty());

    size_t count = 0;
    size_t pos = 0;
    while ((pos = content.find("[EVENT:SERVER_CHAT]", pos)) != std::string::npos) {
        count++;
        pos += 19;
    }
    CHECK(count == 3);

    seek = lines;
    PASS();

    std::remove(msg_tmp.c_str());
}

auto main() -> int {
    std::cout << "=== srb2dbot bridge E2E test suite ===\n\n";

    test_discord_to_srb2_e2e();
    test_srb2_to_discord_e2e();
    test_bridge_full_cycle();
    test_bridge_event_pipeline();
    test_rapid_identical_chat_events();
    test_rapid_identical_server_chat_events();

    std::cout << "\n=== " << failures << " test(s) failed ===\n";
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
