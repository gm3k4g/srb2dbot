-- discord_bridge.lua  —  Bridge SRB2 netgame events to Discord
--
-- Hooks fire on in-game events and call write_event() to append a
-- pipe-delimited line to Messages.txt. The C++ bot polls this file
-- every 2 seconds, reads all lines, parses them, and sends Discord
-- embeds.
--
-- NOTE: return true suppresses SRB2's normal chat processing, preventing
-- the engine double-fire of PlayerMsg. chatprintf() emulates the in-game
-- chat display so the player still sees their own message.

if rawget(_G, "DiscordBotMini") then return end
rawset(_G, "DiscordBotMini", true)

local current_map = nil
local join_emitted = {}

-- Shared global for captured gametype data (outside DiscordBot, shared across scripts)
if not rawget(_G, "_DBOT_GT") then rawset(_G, "_DBOT_GT", { names = {}, rules = {} }) end

-- Wrap G_AddGametype to capture custom gametype names and rules at registration time.
if G_AddGametype then
	local orig_G_AddGametype = G_AddGametype
	_G["G_AddGametype"] = function(tabl)
		local gt = orig_G_AddGametype(tabl)
		if tabl and type(gt) == "number" then
			if tabl.name then _DBOT_GT.names[gt] = tabl.name end
			if tabl.rules then _DBOT_GT.rules[gt] = tabl.rules end
		end
		return gt
	end
end

local function write_event(line)
	local f = io.openlocal("client/DiscordBot/Messages.txt", "a+")
	if f then
		f:write(line)
		f:close()
	end
end

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
	-- Check auto-captured names from G_AddGametype wrapper
	if _DBOT_GT then
		local name = _DBOT_GT.names[gt]
		if name then return name end
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
	local known = names[gt]
	if known then return known end
	-- Last resort: derive name from GT_ global constant
	if type(gt) == "number" then
		for k, v in pairs(_G) do
			if type(k) == "string" and k:sub(1, 3) == "GT_" and v == gt then
				local name = k:sub(4):gsub("_", " "):lower()
				name = name:gsub("(%a)([%a]*)", function(f, r) return f:upper()..r end)
				return name
			end
		end
	end
	return "Unknown"
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
		local prev_mapstr = map_num_to_mapstr(current_map)
		local gtname = get_gametype_name(gametype)
		write_event("[EVENT:ROUND_END]|" .. gtname .. "|" .. prev_mapstr .. "|0|0|0|0|0|ffa|0|0|0:00|Unknown|[]|[]\n")
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
	if type ~= 0 then return end
	if target and target ~= 0 then return end
	local skinname = (skins[player.skin] and skins[player.skin].name) or ""
	local flag = (player.gotflag and player.gotflag > 0) and "1" or "0"
	local team = "none"
	if not player.spectator and (player.ctfteam == 1 or player.ctfteam == 2) then
		team = tostring(player.ctfteam)
	end
	local prefix = ""
	if IsPlayerAdmin(player) then prefix = "@" end

	if player == server then
		write_event("[EVENT:CHAT]|[" .. #player .. "]|" .. prefix .. player.name .. "|" .. msg .. "|" .. skinname .. "|0|" .. flag .. "|" .. team .. "\n")
	end
end)

addHook("PlayerJoin", function(playernum)
	if join_emitted[playernum] then return end
	for player in players.iterate do
		if #player == playernum then
			join_emitted[playernum] = true
			write_event("[EVENT:PLAYER_JOIN]|" .. player.name .. "|" .. #player .. "\n")
			chatprint("\x82" .. player.name .. "\x80 entered the game")
			break
		end
	end
end)

addHook("PlayerQuit", function(player, reason)
	join_emitted[#player] = nil
	local reason_str = reason_to_string(reason)
	write_event("[EVENT:PLAYER_QUIT]|" .. player.name .. "|" .. #player .. "|" .. reason_str .. "\n")
	if reason == KR_KICK then
		write_event("[EVENT:KICK_PLAYER]|" .. player.name .. "|" .. #player .. "|" .. reason_str .. "\n")
	elseif reason == KR_BAN then
		write_event("[EVENT:BAN_PLAYER]|" .. player.name .. "|" .. #player .. "|" .. reason_str .. "\n")
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