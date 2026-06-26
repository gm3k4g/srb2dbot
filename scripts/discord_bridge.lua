-- discord_bridge.lua  —  Bridge SRB2 netgame events to Discord
--
-- Hooks fire on in-game events and call write_event() to append a
-- pipe-delimited line to Messages.txt. The C++ bot polls this file
-- every 2 seconds, reads all lines, parses them, and sends Discord
-- embeds.
--
-- NOTE: PlayerMsg fires twice in SRB2's engine for player chat (return
-- false continues normal processing which triggers a second fire).
-- The C++ bot dedups consecutive identical CHAT lines so Discord only
-- receives one message per chat.

if rawget(_G, "DiscordBotMini") then return end
rawset(_G, "DiscordBotMini", true)

-- ── Locals ─────────────────────────────────────────────────────────────
local current_map

-- ── write_event: append a line to Messages.txt ─────────────────────────
local function write_event(line)
	local f = io.openlocal("client/DiscordBot/Messages.txt", "a+")
	if f then
		f:write(line)
		f:close()
	end
end

-- ── Helpers ────────────────────────────────────────────────────────────
local function map_num_to_mapstr(n)
	local val, first, r, second
	if n < 100 then return string.format("MAP%02d", n) end
	val = n - 100
	first = string.char(((val - val % 36) / 36) + string.byte('A'))
	r = val % 36
	if r < 10 then second = string.char(r + string.byte('0'))
	else second = string.char(r - 10 + string.byte('A')) end
	return "MAP" .. first .. second
end

local function get_gametype_name(gt)
	local name, names
	if G_GetGametypeName then
		name = G_GetGametypeName(gt)
		if name and name ~= "" then return name end
	end
	names = {
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

-- ── Hooks ──────────────────────────────────────────────────────────────

current_map = nil

addHook("MapChange", function(map)
	local prev_mapstr, gtname, mapname, maptitle, mapstr
	if current_map ~= nil then
		prev_mapstr = map_num_to_mapstr(current_map)
		gtname = get_gametype_name(gametype)
		write_event("[EVENT:ROUND_END]|" .. gtname .. "|" .. prev_mapstr .. "|0|0|0|0|0|ffa|0|0|0:00|Unknown|[]|[]\n")
	end
	current_map = map
	mapname = mapheaderinfo[map]
	maptitle = "Unknown Map"
	if mapname and mapname.lvlttl then
		maptitle = mapname.lvlttl
		if mapname.actnum and mapname.actnum > 0 then
			maptitle = maptitle .. " Act " .. mapname.actnum
		end
	end
	gtname = get_gametype_name(gametype)
	mapstr = map_num_to_mapstr(map)
	write_event("[EVENT:ROUND_START]|" .. gtname .. "|" .. mapstr .. "|" .. maptitle .. "\n")
end)

addHook("PlayerMsg", function(player, type, target, msg)
	if not player then return end
	if type ~= 0 then return end
	if server ~= player and target and target ~= 0 then return end
	if server == player then return end

	local node, name, skinname, flag, team, prefix

	node = #player
	name = player.name
	skinname = (skins[player.skin] and skins[player.skin].name) or ""
	flag = (player.gotflag and player.gotflag > 0) and "1" or "0"
	team = "none"
	if not player.spectator then
		if gametype == GT_CTF or gametype == GT_TEAMMATCH or gametype == GT_TEAMBATTLE then
			team = tostring(player.ctfteam or 0)
		end
	end
	prefix = ""
	if IsPlayerAdmin(player) then prefix = "@" end

	write_event("[EVENT:CHAT]|[" .. node .. "]|" .. prefix .. name .. "|" .. msg .. "|" .. skinname .. "|0|" .. flag .. "|" .. team .. "\n")
	return false
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
	local reason_str
	reason_str = reason_to_string(reason)
	write_event("[EVENT:PLAYER_QUIT]|" .. player.name .. "|" .. #player .. "|" .. reason_str .. "\n")
	if reason == KR_KICK then
		write_event("[EVENT:KICK_PLAYER]|" .. player.name .. "|" .. #player .. "|" .. reason_str .. "\n")
	elseif reason == KR_BAN then
		write_event("[EVENT:BAN_PLAYER]|" .. player.name .. "|" .. #player .. "|" .. reason_str .. "\n")
	end
end)

COM_AddCommand("dbot_sync", function(player)
	local mapstr, gtname, maptitle
	if player ~= server then return end
	mapstr = current_map and map_num_to_mapstr(current_map) or ""
	gtname = get_gametype_name(gametype)
	maptitle = "Unknown"
	if current_map and mapheaderinfo[current_map] then
		maptitle = mapheaderinfo[current_map].lvlttl or "Unknown"
	end
	if current_map then
		write_event("[EVENT:ROUND_START]|" .. gtname .. "|" .. mapstr .. "|" .. maptitle .. "\n")
	end
end, COM_LOCAL)