-- srb2dbot-mini.lua  —  Minimal SRB2→Discord bridge (hooks only, no file I/O)
--
-- All hooks are registered but write_event() is a no-op.
-- Implement write_event() yourself to send events to Discord.
--
-- See the bottom of this file for implementation instructions.

if rawget(_G, "DiscordBotMini") then return end
rawset(_G, "DiscordBotMini", true)

local current_map = nil

-- ═══════════════════════════════════════════════════════════════════════
-- WRITE EVENT — implement this yourself
-- ═══════════════════════════════════════════════════════════════════════
-- `line` is a pipe-delimited string ending in \n, e.g.:
--   [EVENT:CHAT]|[1]|Sonic|hello|sonic|0|0|none
--   [EVENT:SERVER_CHAT]|text|FEE75C
--   [EVENT:PLAYER_JOIN]|Sonic|1
--   [EVENT:PLAYER_QUIT]|Sonic|1|Left
--   [EVENT:ROUND_START]|CTF|MAPF0|Lime Forest
--   [EVENT:ROUND_END]|CTF|MAPF0|0|0|0|0|0|ffa|0|0|0:00|Unknown|[]|[]
--   [EVENT:CSAY]|text
--   [EVENT:KICK_PLAYER]|Sonic|1|Kicked
--   [EVENT:BAN_PLAYER]|Sonic|1|Banned
--
-- The C++ bot polls this file every 2 seconds, renames it to .tmp,
-- reads all lines, parses each one, and sends Discord embeds.
-- Your job: make write_event() append `line` to Messages.txt.
--
-- File path: ~/.srb2/luafiles/client/DiscordBot/Messages.txt
-- Use io.openlocal() which is relative to ~/.srb2/luafiles/
--
-- Example implementation:
--   local function write_event(line)
--       local f = io.openlocal("client/DiscordBot/Messages.txt", "a+")
--       if f then
--           f:write(line)
--           f:close()
--       end
--   end

local function write_event(line)
    local f = io.openlocal("client/DiscordBot/Messages.txt", "a+")
    if f then
        f:write(line)
        f:close()
    end
end

-- ═══════════════════════════════════════════════════════════════════════
-- HOOKS — these call write_event() when events happen in-game
-- ═══════════════════════════════════════════════════════════════════════

local function map_num_to_mapstr(n)
	if n < 100 then return string.format("MAP%02d", n) end
	local val = n - 100
	local first = string.char(((val - val % 36) / 36) + string.byte('A'))
	local r = val % 36
	local second
	if r < 10 then second = string.char(r + string.byte('0'))
	else second = string.char(r - 10 + string.byte('A')) end
	return "MAP" .. first .. second
end

local function get_gametype_name(gt)
	if G_GetGametypeName then
		local name = G_GetGametypeName(gt)
		if name and name ~= "" then return name end
	end
	local names = {
		[GT_COOP] = "Co-op",
		[GT_COMPETITION] = "Competition",
		[GT_RACE] = "Race",
		[GT_MATCH] = "Match",
		[GT_TEAMMATCH] = "Team Match",
		[GT_TAG] = "Tag",
		[GT_HIDEANDSEEK] = "Hide & Seek",
		[GT_CTF] = "CTF",
	}
	if GT_BATTLE then names[GT_BATTLE] = "Arena" end
	if GT_TEAMBATTLE then names[GT_TEAMBATTLE] = "Team Arena" end
	return names[gt] or "Unknown"
end

local function reason_to_string(r)
	if r == KR_KICK then return "Kicked"
	elseif r == KR_PINGLIMIT then return "Ping limit"
	elseif r == KR_SYNCH then return "Synch failure"
	elseif r == KR_TIMEOUT then return "Timeout"
	elseif r == KR_BAN then return "Banned"
	elseif r == KR_IDLE then return "Idle"
	elseif r == KR_LEAVE then return "Left" end
	return "Unknown"
end

addHook("MapChange", function(map)
	if current_map ~= nil then
		local mapstr = map_num_to_mapstr(current_map)
		local gtname = get_gametype_name(gametype)
		write_event("[EVENT:ROUND_END]|" .. gtname .. "|" .. mapstr .. "|0|0|0|0|0|ffa|0|0|0:00|Unknown|[]|[]\n")
	end
	current_map = map
	local mapname = mapheaderinfo[map]
	local maptitle = "Unknown Map"
	if mapname and mapname.lvlttl then
		maptitle = mapname.lvlttl
		if mapname.actnum and mapname.actnum > 0 then
			maptitle = maptitle .. " Act " .. mapname.actnum
		end
	end
	local gtname = get_gametype_name(gametype)
	local mapstr = map_num_to_mapstr(map)
	write_event("[EVENT:ROUND_START]|" .. gtname .. "|" .. mapstr .. "|" .. maptitle .. "\n")
end)

addHook("PlayerMsg", function(player, type, target, msg)
	if not player then return end
	if type == 0 then
		if server ~= player and target and target ~= 0 then return end
		if server == player then
			write_event("[EVENT:SERVER_CHAT]|" .. msg .. "|FEE75C\n")
			chatprint("<\x82~\x80Server> " .. msg)
			return true
		end
		local skinname = (skins[player.skin] and skins[player.skin].name) or ""
		local flag = (player.gotflag and player.gotflag > 0) and "1" or "0"
		local team = "none"
		if not player.spectator then
			if gametype == GT_CTF or gametype == GT_TEAMMATCH or gametype == GT_TEAMBATTLE then
				team = tostring(player.ctfteam or 0)
			end
		end
		local prefix = ""
		if IsPlayerAdmin(player) then prefix = "@" end
		write_event("[EVENT:CHAT]|[" .. #player .. "]|" .. prefix .. player.name .. "|" .. msg .. "|" .. skinname .. "|0|" .. flag .. "|" .. team .. "\n")
		return false
	elseif type == 3 then
		write_event("[EVENT:CSAY]|" .. msg .. "\n")
	end
end)

addHook("PlayerJoin", function(playernum)
	for player in players.iterate do
		if #player == playernum then
			write_event("[EVENT:PLAYER_JOIN]|" .. player.name .. "|" .. #player .. "\n")
			break
		end
	end
end)

addHook("PlayerQuit", function(player, reason)
	write_event("[EVENT:PLAYER_QUIT]|" .. player.name .. "|" .. #player .. "|" .. reason_to_string(reason) .. "\n")
	if reason == KR_KICK then
		write_event("[EVENT:KICK_PLAYER]|" .. player.name .. "|" .. #player .. "|" .. reason_to_string(reason) .. "\n")
	elseif reason == KR_BAN then
		write_event("[EVENT:BAN_PLAYER]|" .. player.name .. "|" .. #player .. "|" .. reason_to_string(reason) .. "\n")
	end
end)

COM_AddCommand("dbot_sync", function(player)
	if player ~= server then return end
	local mapstr = current_map and map_num_to_mapstr(current_map) or ""
	local gtname = get_gametype_name(gametype)
	local maptitle = "Unknown"
	if current_map and mapheaderinfo[current_map] then
		maptitle = mapheaderinfo[current_map].lvlttl or "Unknown"
	end
	if current_map then
		write_event("[EVENT:ROUND_START]|" .. gtname .. "|" .. mapstr .. "|" .. maptitle .. "\n")
	end
end, COM_LOCAL)