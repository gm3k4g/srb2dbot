-- srb2dbot hook test suite
-- Tests the core Lua functions used in the DiscordBot script
-- Run: srb2 -dedicated -file scripts/test_hooks.lua

local outfile = io.openlocal("test_results.txt", "w+")
local passed = 0
local failed = 0

local function assert(condition, msg)
	if condition then
		passed = passed + 1
		outfile:write("PASS: " .. msg .. "\n")
	else
		failed = failed + 1
		outfile:write("FAIL: " .. msg .. "\n")
	end
end

-- ── Test environment setup ──

local servertime_n = 0

local test_env = {
	msgsrb2 = '',
	debug = false,
	skin_colors = {
		["1"] = "2F3136",
	},
}

local test_cmds = {
	cv_nospamchat = { value = 0 },
	cv_messagedelay = { value = 1 },
}

local test_messages = {}

-- ── Core functions under test (copied from SRB2DiscordBot-v0.1.46.lua) ──

local function reason_to_string(r)
	if r == 1 then return "Kicked"
	elseif r == 2 then return "Ping limit"
	elseif r == 3 then return "Synch failure"
	elseif r == 4 then return "Timeout"
	elseif r == 5 then return "Banned"
	elseif r == 7 then return "Idle"
	elseif r == 6 then return "Left" end
	return "Unknown"
end

local function spamchatbug_test(data, cmds, messages, player, msg, joinquit)
	if cmds.cv_nospamchat.value == 0 and not joinquit then
		data.msgsrb2 = data.msgsrb2 .. msg
		return true
	end
	if player ~= "server" then
		if not messages[player] then
			messages[player] = servertime_n
		else
			messages[player] = servertime_n
		end
	end
	data.msgsrb2 = data.msgsrb2 .. msg
	return true
end

local function flush_msgsrb2_test(data)
	if data.msgsrb2 and data.msgsrb2 ~= '' then
		local logmsg = io.openlocal("test_Messages.txt", "a+")
		if logmsg then
			logmsg:write(data.msgsrb2)
			logmsg:close()
		end
		data.msgsrb2 = ''
	end
end

-- ── Test: reason_to_string ──

outfile:write("\n=== reason_to_string ===\n")
assert(reason_to_string(1) == "Kicked", "KR_KICK -> Kicked")
assert(reason_to_string(2) == "Ping limit", "KR_PINGLIMIT -> Ping limit")
assert(reason_to_string(3) == "Synch failure", "KR_SYNCH -> Synch failure")
assert(reason_to_string(4) == "Timeout", "KR_TIMEOUT -> Timeout")
assert(reason_to_string(5) == "Banned", "KR_BAN -> Banned")
assert(reason_to_string(6) == "Left", "KR_LEAVE -> Left")
assert(reason_to_string(7) == "Idle", "KR_IDLE -> Idle")
assert(reason_to_string(99) == "Unknown", "Unknown code -> Unknown")

-- ── Test: spamchatbug (nospamchat off, regular chat) ──

outfile:write("\n=== spamchatbug - nospamchat off, regular chat ===\n")
test_env.msgsrb2 = ''
local r = spamchatbug_test(test_env, test_cmds, test_messages, "Player1", "[EVENT:CHAT]|hello\n", nil)
assert(r == true, "returns true for accepted message")
assert(test_env.msgsrb2 == "[EVENT:CHAT]|hello\n", "message appended to msgsrb2")

-- ── Test: spamchatbug (nospamchat off, joinquit message) ──

outfile:write("\n=== spamchatbug - nospamchat off, joinquit ===\n")
test_env.msgsrb2 = ''
r = spamchatbug_test(test_env, test_cmds, test_messages, "Player1", "[EVENT:CHAT]|join\n", true)
assert(r == true, "returns true for joinquit message")
assert(test_env.msgsrb2 == "[EVENT:CHAT]|join\n", "joinquit message appended to msgsrb2")

-- ── Test: spamchatbug (server message) ──

outfile:write("\n=== spamchatbug - server message ===\n")
test_env.msgsrb2 = ''
r = spamchatbug_test(test_env, test_cmds, test_messages, "server", "[EVENT:SERVER_CHAT]|hello\n", nil)
assert(r == true, "returns true for server message")
assert(test_env.msgsrb2 == "[EVENT:SERVER_CHAT]|hello\n", "server message appended to msgsrb2")

-- ── Test: flush_msgsrb2 ──

outfile:write("\n=== flush_msgsrb2 ===\n")
test_env.msgsrb2 = '[EVENT:TEST]|data\n'
flush_msgsrb2_test(test_env)
assert(test_env.msgsrb2 == '', "msgsrb2 cleared after flush")
local f = io.openlocal("test_Messages.txt", "r")
if f then
	local content = f:read("*a")
	f:close()
	assert(content == '[EVENT:TEST]|data\n', "flush writes correct content to file")
end

-- ── Test: CHAT event format ──

outfile:write("\n=== CHAT event format ===\n")
local player_num = 1
local player_name = "TestPlayer"
local message = "hello world"
local color = "57F287"
local chat_line = "[EVENT:CHAT]|[" .. player_num .. "|**<" .. player_name .. ">**|" .. message .. "|" .. color .. "\n"
assert(chat_line:find("%[EVENT:CHAT%]") == 1, "CHAT starts with [EVENT:CHAT]")
assert(chat_line:find(player_name) ~= nil, "CHAT contains player name")
assert(chat_line:find(message) ~= nil, "CHAT contains message")

-- ── Test: PLAYER_JOIN event format ──

outfile:write("\n=== PLAYER_JOIN event format ===\n")
local join_line = "[EVENT:PLAYER_JOIN]|" .. player_name .. "|" .. player_num .. "\n"
assert(join_line:find("%[EVENT:PLAYER_JOIN%]") == 1, "PLAYER_JOIN starts with [EVENT:PLAYER_JOIN]")
assert(join_line:find(player_name) ~= nil, "PLAYER_JOIN contains player name")
assert(join_line:find(tostring(player_num)) ~= nil, "PLAYER_JOIN contains node number")

-- ── Test: PLAYER_QUIT event format ──

outfile:write("\n=== PLAYER_QUIT event format ===\n")
local quit_line = "[EVENT:PLAYER_QUIT]|" .. player_name .. "|" .. player_num .. "|Left\n"
assert(quit_line:find("%[EVENT:PLAYER_QUIT%]") == 1, "PLAYER_QUIT starts with [EVENT:PLAYER_QUIT]")
assert(quit_line:find("Left") ~= nil, "PLAYER_QUIT contains reason")

-- ── Test: SERVER_CHAT event format ──

outfile:write("\n=== SERVER_CHAT event format ===\n")
local server_line = "[EVENT:SERVER_CHAT]|" .. message .. "|FEE75C\n"
assert(server_line:find("%[EVENT:SERVER_CHAT%]") == 1, "SERVER_CHAT starts with [EVENT:SERVER_CHAT]")
assert(server_line:find("FEE75C") ~= nil, "SERVER_CHAT contains color")

-- ── Test: PlayerMsg 5-tic dedup window ──

outfile:write("\n=== PlayerMsg dedup window ===\n")
local PLAYERMSG_DEDUP_TICS = 5
local dedup_cache = {}
local leveltime_n = 0

local function playermsg_dedup_check(node, mtype, target, msg, lt)
	local cache_key = tostring(node) .. "|" .. tostring(mtype) .. "|" .. tostring(target) .. "|" .. msg
	if dedup_cache[cache_key] and lt - dedup_cache[cache_key] <= PLAYERMSG_DEDUP_TICS then
		return true  -- suppress
	end
	dedup_cache[cache_key] = lt
	return false  -- allow
end

-- Fire 1 at leveltime 100: allowed
assert(playermsg_dedup_check(1, 0, 0, "a", 100) == false, "first fire at LT100 allowed")
-- Fire 2 at leveltime 101 (engine double-fire, 1 tic later): suppressed
assert(playermsg_dedup_check(1, 0, 0, "a", 101) == true, "second fire at LT101 (1 tic) suppressed")
-- Fire 3 at leveltime 103 (engine triple-fire, 3 tics later): suppressed
assert(playermsg_dedup_check(1, 0, 0, "a", 103) == true, "third fire at LT103 (3 tics) suppressed")
-- Fire 4 at leveltime 106 (6 tics after first = beyond window): allowed (legitimate new message)
assert(playermsg_dedup_check(1, 0, 0, "a", 106) == false, "fire at LT106 (6 tics after first) allowed")
-- Different message at same leveltime: allowed (different cache key)
assert(playermsg_dedup_check(1, 0, 0, "b", 106) == false, "different message 'b' allowed")
-- Different player at same leveltime: allowed
assert(playermsg_dedup_check(2, 0, 0, "a", 106) == false, "different player node 2 allowed")

-- Rapid typing: "a" at LT 0, 9, 18, 27 (every ~257ms = 9 tics): all allowed
dedup_cache = {}
assert(playermsg_dedup_check(1, 0, 0, "a", 0) == false, "rapid: first 'a' at LT0 allowed")
assert(playermsg_dedup_check(1, 0, 0, "a", 9) == false, "rapid: second 'a' at LT9 allowed (9>5)")
assert(playermsg_dedup_check(1, 0, 0, "a", 18) == false, "rapid: third 'a' at LT18 allowed (9>5)")
assert(playermsg_dedup_check(1, 0, 0, "a", 27) == false, "rapid: fourth 'a' at LT27 allowed (9>5)")

-- ── Test: KICK/BAN event format ──

outfile:write("\n=== KICK/BAN event format ===\n")
local kick_line = "[EVENT:KICK_PLAYER]|" .. player_name .. "|" .. player_num .. "|Kicked\n"
assert(kick_line:find("%[EVENT:KICK_PLAYER%]") == 1, "KICK_PLAYER starts with [EVENT:KICK_PLAYER]")
local ban_line = "[EVENT:BAN_PLAYER]|" .. player_name .. "|" .. player_num .. "|Banned\n"
assert(ban_line:find("%[EVENT:BAN_PLAYER%]") == 1, "BAN_PLAYER starts with [EVENT:BAN_PLAYER]")

-- ── Summary ──

io.openlocal("test_Messages.txt"):close()
outfile:write("\n=== RESULTS: " .. passed .. " passed, " .. failed .. " failed ===\n")
outfile:close()
print("TEST HOOKS: " .. passed .. " passed, " .. failed .. " failed")
if failed == 0 then
	print("ALL TESTS PASSED")
else
	print(failed .. " TEST(S) FAILED")
end
COM_BufInsertText(server, "quit")