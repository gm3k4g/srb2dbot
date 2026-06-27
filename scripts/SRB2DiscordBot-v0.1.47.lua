-- Double-load guard: prevent re-initialization if this script somehow
-- executes more than once (e.g. loaded via multiple -file args).
if rawget(_G, "DiscordBot") and DiscordBot.version then return end

rawset(_G, "DiscordBot", {})
DiscordBot.version = "0.1.47"
DiscordBot.Data = {}
DiscordBot.Data.msgsrb2 = ''
DiscordBot.Data.pcmtsrb2 = ''
DiscordBot.Data.stats = ''
DiscordBot.Data.pcmp = ''
DiscordBot.Data.log = ''
DiscordBot.Data.console = ''
DiscordBot.Data.paused = false
DiscordBot.Data.maptitle = ''
DiscordBot.Data.nextlevel = ''
DiscordBot.Data.iconemeralds = ''
DiscordBot.Data.gametype = nil
DiscordBot.Data.countemeralds = 0
DiscordBot.Data.servertime = 0
DiscordBot.Data.server_started = os.time()
DiscordBot.Data.current_map = nil
DiscordBot.Data.debug = false
DiscordBot.Data._discord_seek_pos = 0
DiscordBot.Data._discord_gametypes = nil

-- Shared global table for captured gametype data (outside DiscordBot so
-- it survives NetVar deep-copy; shared across all loaded Lua scripts).
if not rawget(_G, "_DBOT_GT") then rawset(_G, "_DBOT_GT", { names = {}, rules = {} }) end

-- Wrap G_AddGametype to capture custom gametype names and rules at registration time.
if G_AddGametype then
	local orig = G_AddGametype
	_G["G_AddGametype"] = function(tabl, ...)
		local gt = orig(tabl, ...)
		-- Old SRB2 may return nil instead of the GT value; look it up from globals
		if not tonumber(gt) and tabl and tabl.identifier then
			gt = _G["GT_" .. tabl.identifier:upper()]
		end
		gt = tonumber(gt)
		if gt and tabl then
			if tabl.name then _DBOT_GT.names[gt] = tabl.name end
			if tabl.rules then _DBOT_GT.rules[gt] = tabl.rules end
		end
		return gt
	end
end



-- On first ThinkFrame (after all WAD scripts loaded), scan GT_ globals to
-- capture gametype names that the wrapper may have missed due to load order.
addHook("ThinkFrame", function()
	if DiscordBot._gametypes_scanned then return end
	DiscordBot._gametypes_scanned = true
	-- Scan GT_ globals to capture gametype names that the wrapper may have missed
	for k, v in pairs(_G) do
		if type(k) == "string" and k:sub(1, 3) == "GT_" and type(v) == "number" then
			if not _DBOT_GT.names[v] and G_GetGametypeName then
				local name = G_GetGametypeName(v)
				if name and name ~= "" then
					_DBOT_GT.names[v] = name
				end
			end
		end
	end
end)

DiscordBot.Functions = {}
DiscordBot.Commands = {}
DiscordBot.Commands.cv_joinquit = CV_RegisterVar({name = "dbot_joinquit", defaultvalue = "On", flags = CV_NETVAR, PossibleValue = CV_OnOff})
DiscordBot.Commands.cv_autopause = CV_RegisterVar({name = "dbot_autopause", defaultvalue = "On", flags = CV_NETVAR, PossibleValue = CV_OnOff})
DiscordBot.Commands.cv_nospamchat = CV_RegisterVar({name = "dbot_nospamchat", defaultvalue = "Off", flags = CV_NETVAR, PossibleValue = CV_OnOff})
DiscordBot.Commands.cv_messagedelay = CV_RegisterVar({name = "dbot_messagedelay", defaultvalue = "On", flags = CV_NETVAR, PossibleValue = CV_OnOff})

-- Read auto_pause from console.txt (written by C++ bot from modules.json)
DiscordBot.Config = { auto_pause = true }
do
	local f = io.openlocal("client/DiscordBot/console.txt", "r")
	if f then
		for line in f:lines() do
			if line:find("^dbot_autopause ") then
				local val = line:match("^dbot_autopause ([01])$")
				if val then DiscordBot.Config.auto_pause = (val == "1") end
			end
		end
		f:close()
	end
	-- Clear so server_log console doesn't re-process stale commands
	local f = io.openlocal("client/DiscordBot/console.txt", "w")
	if f then f:write("") f:close() end
end

DiscordBot.Messages = {}

DiscordBot.Functions.flush_msgsrb2 = function()
	if DiscordBot.Data.msgsrb2 and DiscordBot.Data.msgsrb2 ~= '' then
		if DiscordBot.Data.debug then
			print("[DEBUG] flush_msgsrb2: writing " .. string.len(DiscordBot.Data.msgsrb2) .. " bytes to Messages.txt")
		end
		local logmsg = io.openlocal("client/DiscordBot/Messages.txt", "a+")
		if logmsg then
			logmsg:write(DiscordBot.Data.msgsrb2)
			logmsg:close()
		end
		DiscordBot.Data.msgsrb2 = ''
	end
end

DiscordBot.Functions.spamchatbug = function(player, msg, joinquit)
	local checked = false
	if DiscordBot.Commands.cv_nospamchat.value == 0 and not joinquit then
		DiscordBot.Data.msgsrb2 = DiscordBot.Data.msgsrb2 .. msg
		return true
	end
	if player ~= server then
		if not DiscordBot.Messages[player.name] then
			DiscordBot.Messages[player.name] = DiscordBot.Data.servertime
			checked = true
		end
		if checked == false then
			if DiscordBot.Data.servertime - DiscordBot.Messages[player.name] < 35 then
				DiscordBot.Messages[player.name] = DiscordBot.Data.servertime
				return false
			end
		end
		DiscordBot.Messages[player.name] = DiscordBot.Data.servertime
	end
	DiscordBot.Data.msgsrb2 = DiscordBot.Data.msgsrb2 .. msg
	if DiscordBot.Commands.cv_messagedelay.value == 0 then
		COM_BufInsertText(server, "server_log msg")
		COM_BufInsertText(server, "server_log discord")
		COM_BufInsertText(server, "server_log console")
		COM_BufInsertText(server, "server_log logcom")
	end
	return true
end

DiscordBot.Functions.playerontheserver = function()
	local count = 0
	for player in players.iterate do
		if player then
			count = $ + 1
		end
	end
	return count
end

DiscordBot.Functions.statsofserver = function()
	local playerstats = ''
	for player in players.iterate do
		if player then
			local pname = string.gsub(player.name, "`", "")
			local ping = 0
			local statms = ''
			local iconskin = ':unknown:'
			local pffinished = ':black_large_square:'
			local admin = ':black_large_square:'
			if player.ping then ping = player.ping end
			if (ping < 32) then
				statms = ':ping_blue:'
			elseif (ping < 64) then
				statms = ':ping_green:'
			elseif (ping < 128) then
				statms = ':ping_yellow:'
			elseif (ping < 256) then
				statms = ':ping_red:'
			end
			if player.mo and player.spectator ~= true then
				iconskin = ":" .. (skins[player.skin] and skins[player.skin].name or "") .. ":"
			end
			if player.spectator == true then
				iconskin = ':spectator:'
			elseif player.playerstate == PST_DEAD then
				iconskin = ':dead:'
			end
			if player.mo and (player.pflags & PF_FINISHED) then pffinished = ":completed:" end
			if IsPlayerAdmin(player) then admin = ":remote_admin:" end
			if player.playtime == nil then player.playtime = 0 end
			local seconds = G_TicsToSeconds(player.playtime)
			if string.len(seconds) == 1 then seconds = "0" .. tostring(seconds) end
			local pptime = G_TicsToMinutes(player.playtime, true) .. ":" .. seconds
			playerstats = $ + statms .. iconskin .. pffinished .. admin .. "[" .. #player .. "] `" .. pname .. "`: Score - " .. player.score .. "; Time - " .. pptime .. "\n"
		end
	end
	if playerstats == '' then
		playerstats = "There's no one here."
	end
	return playerstats
end

COM_AddCommand("server_log", function(player, arg, text)
	if player ~= server then return end
	if arg == "msg" then
		if DiscordBot.Data.msgsrb2 and DiscordBot.Data.msgsrb2 ~= '' then
			local logmsg = io.openlocal("client/DiscordBot/Messages.txt", "a")
			if logmsg then
				logmsg:write(DiscordBot.Data.msgsrb2)
				logmsg:close()
			end
			DiscordBot.Data.msgsrb2 = ''
		end
	elseif arg == "logcom" then
		if DiscordBot.Data.log then
			local logcom = io.openlocal("client/DiscordBot/logcom.txt", "a+")
			if logcom then
				logcom:write(DiscordBot.Data.log)
				logcom:close()
			end
		end
	elseif arg == "pause" then
		if DiscordBot.Data.pcmtsrb2 then
			local logmsg = io.openlocal("client/DiscordBot/Players.txt", "w")
			if logmsg then
				logmsg:write("Game is paused, no players")
				logmsg:close()
			end
		end
	elseif arg == "players" then
		if DiscordBot.Data.pcmtsrb2 then
			local logmsg = io.openlocal("client/DiscordBot/Players.txt", "w")
			if logmsg then
				logmsg:write(DiscordBot.Data.pcmtsrb2)
				logmsg:close()
			end
		end
		if DiscordBot.Data.stats then
			-- Server name
			local cv_servername = CV_FindVar("servername")
			local playerskins = ''
			for i = 0, #skins - 1 do
				playerskins = playerskins .. ":" .. skins[i].name .. ": "
			end
			local lseconds = G_TicsToSeconds(leveltime)
			if string.len(lseconds) == 1 then lseconds = "0" .. tostring(lseconds) end
			local ltime = G_TicsToMinutes(leveltime, true) .. ":" .. lseconds
			local server_up = os.time() - DiscordBot.Data.server_started
			local smins = (server_up - server_up % 60) / 60
			local ssecs = server_up % 60
			if ssecs < 10 then ssecs = "0" .. ssecs end
			local stime = smins .. ":" .. ssecs
			if DiscordBot.Data.paused == true then
				DiscordBot.Data.stats = "There's no one here."
			end
			local logmsg = io.openlocal("client/DiscordBot/Stats.txt", "w")
			if logmsg then
				logmsg:write(cv_servername.string .. "\n" .. DiscordBot.Data.maptitle .. " (" .. gamemap .. ")\n" .. gamemap .. "\n" .. DiscordBot.Data.nextlevel .. "\n" .. DiscordBot.Data.iconemeralds .. "\n" .. playerskins .. "\n" .. ltime .. "\n" .. stime .. "\n" .. DiscordBot.Data.pcmp .. "\n" .. DiscordBot.Data.stats)
				logmsg:close()
			end
		end
	elseif arg == "console" then
		if DiscordBot.Data.console then
			local d_console = io.openlocal("client/DiscordBot/console.txt", "r")
			if d_console then
				local clear = false
				while true do
					local line = d_console:read("*l") or ""
					if line == "" then break end
					line = string.sub(line, 1, 220)
					if string.find(string.sub(string.lower(line), 1, 5), string.lower("quit")) == nil
						and string.find(string.sub(string.lower(line), 1, 9), string.lower("exitgame")) == nil then
						COM_BufInsertText(server, line)
					end
					clear = true
				end
				d_console:close()
				if clear == true then
					local c_clear = io.openlocal("client/DiscordBot/console.txt", "w")
					if c_clear then
						c_clear:write("")
						c_clear:close()
					end
				end
			end
		end
	elseif arg == "discord" then
		if DiscordBot.Data.debug then COM_BufInsertText(server, "echo [DBOT] server_log discord running") end
		local d_msg = io.openlocal("client/DiscordBot/discordmessage.txt", "r")
		if d_msg then
			local d_msgread = d_msg:read("*a") or ""
			d_msg:close()
			if #d_msgread > 220 then
				-- Chunk: process first 220 bytes, write remainder back
				local remainder = string.sub(d_msgread, 221)
				d_msgread = string.sub(d_msgread, 1, 220)
				-- Write remainder back so next poll picks it up
				local d_clear = io.openlocal("client/DiscordBot/discordmessage.txt", "w")
				if d_clear then
					d_clear:write(remainder)
					d_clear:close()
				end
			else
				local d_clear = io.openlocal("client/DiscordBot/discordmessage.txt", "w")
				if d_clear then d_clear:write("") d_clear:close() end
			end
			if d_msgread ~= "" then
				COM_BufInsertText(server, "discord_message " .. d_msgread)
			end
		end
	end
end, COM_LOCAL)

COM_AddCommand("dbot_sync", function(player)
	if player ~= server then return end
	if DiscordBot.Data.debug then print("[DEBUG] dbot_sync: re-emitting server state (map=" .. tostring(DiscordBot.Data.current_map) .. ")") end
	DiscordBot.Data.msgsrb2 = ''
	if DiscordBot.Data.current_map ~= nil then
		if DiscordBot.Data.debug then print("[DEBUG] dbot_sync: re-emitting initial map state") end
	end
	DiscordBot.Functions.flush_msgsrb2()
 end, COM_LOCAL)

COM_AddCommand("discord_message", function(player, ...)
	if player ~= server then return end
	if not ... then return end
	local args = {...}
	local msg = table.concat(args, " ")
	chatprint("\x89" .. "[Discord]" .. "\x80" .. " " .. msg, false)
end, COM_LOCAL)

COM_AddCommand("dbot_debug", function(player, arg)
	if player ~= server then return end
	DiscordBot.Data.debug = (arg == "on")
end, COM_LOCAL)

local function bot_function()
	DiscordBot.Data.servertime = DiscordBot.Data.servertime + 1
	if (leveltime % 70) == 35 then
		DiscordBot.Data.stats = DiscordBot.Functions.statsofserver()
		local cv_maxplayer = CV_FindVar("maxplayers")
		local playercount = 0
		DiscordBot.Data.countemeralds = 0
		DiscordBot.Data.iconemeralds = ''
		playercount = DiscordBot.Functions.playerontheserver()
		if not playercount then
			playercount = 0
		end
		if DiscordBot.Data.gametype ~= gametype then
			DiscordBot.Data.gametype = gametype
		end
		if not G_IsSpecialStage(gamemap) then
			if mapheaderinfo[gamemap].nextlevel ~= nil then
				if mapheaderinfo[gamemap].nextlevel >= 1100 then
					DiscordBot.Data.nextlevel = "Ending"
				else
					local nextlevelint = mapheaderinfo[gamemap].nextlevel
					local nextlevel = mapheaderinfo[mapheaderinfo[gamemap].nextlevel]
					if nextlevel ~= nil then
						if nextlevel.actnum == 0 then
							DiscordBot.Data.nextlevel = (nextlevel.lvlttl .. " (" .. nextlevelint .. ")")
						else
							DiscordBot.Data.nextlevel = (nextlevel.lvlttl .. " Act " .. nextlevel.actnum .. " (" .. nextlevelint .. ")")
						end
					else
						DiscordBot.Data.nextlevel = "None"
					end
				end
			end
			if gametype ~= GT_COOP then
				local cv_advancemap = CV_FindVar("advancemap")
				if cv_advancemap.value == 0 then
					if mapheaderinfo[gamemap].actnum == 0 then
						DiscordBot.Data.nextlevel = (mapheaderinfo[gamemap].lvlttl .. " (" .. gamemap .. ")")
					else
						DiscordBot.Data.nextlevel = (mapheaderinfo[gamemap].lvlttl .. " Act " .. mapheaderinfo[gamemap].actnum .. " (" .. gamemap .. ")")
					end
				elseif cv_advancemap.value == 2 then
					DiscordBot.Data.nextlevel = "Random"
				end
			end
		end
		if mapheaderinfo[gamemap].actnum == 0 then
			DiscordBot.Data.maptitle = (mapheaderinfo[gamemap].lvlttl)
		else
			DiscordBot.Data.maptitle = (mapheaderinfo[gamemap].lvlttl .. " Act " .. mapheaderinfo[gamemap].actnum)
		end
		if emeralds & EMERALD1 then
			DiscordBot.Data.countemeralds = $ + 1
			DiscordBot.Data.iconemeralds = DiscordBot.Data.iconemeralds .. ":emerald1:"
		end
		if emeralds & EMERALD2 then
			DiscordBot.Data.countemeralds = $ + 1
			DiscordBot.Data.iconemeralds = DiscordBot.Data.iconemeralds .. ":emerald2:"
		end
		if emeralds & EMERALD3 then
			DiscordBot.Data.countemeralds = $ + 1
			DiscordBot.Data.iconemeralds = DiscordBot.Data.iconemeralds .. ":emerald3:"
		end
		if emeralds & EMERALD4 then
			DiscordBot.Data.countemeralds = $ + 1
			DiscordBot.Data.iconemeralds = DiscordBot.Data.iconemeralds .. ":emerald4:"
		end
		if emeralds & EMERALD5 then
			DiscordBot.Data.countemeralds = $ + 1
			DiscordBot.Data.iconemeralds = DiscordBot.Data.iconemeralds .. ":emerald5:"
		end
		if emeralds & EMERALD6 then
			DiscordBot.Data.countemeralds = $ + 1
			DiscordBot.Data.iconemeralds = DiscordBot.Data.iconemeralds .. ":emerald6:"
		end
		if emeralds & EMERALD7 then
			DiscordBot.Data.countemeralds = $ + 1
			DiscordBot.Data.iconemeralds = DiscordBot.Data.iconemeralds .. ":emerald7:"
		end
		if emeralds == 0 then
			DiscordBot.Data.iconemeralds = "No emeralds"
		end
		if playercount then
			DiscordBot.Data.pcmp = (playercount .. "/" .. cv_maxplayer.value)
			DiscordBot.Data.pcmtsrb2 = ("Players : " .. playercount .. "/" .. cv_maxplayer.value)
		else
			DiscordBot.Data.pcmp = ("0/" .. cv_maxplayer.value)
			DiscordBot.Data.pcmtsrb2 = ("Players : 0/" .. cv_maxplayer.value)
		end
		COM_BufInsertText(server, "server_log discord")
		COM_BufInsertText(server, "server_log console")
		DiscordBot.Functions.flush_msgsrb2()
		if DiscordBot.Data.log ~= '' then
			COM_BufInsertText(server, "server_log logcom")
			DiscordBot.Data.log = ''
		end
		COM_BufInsertText(server, "server_log players")
		if DiscordBot.Config.auto_pause then
			local count = DiscordBot.Functions.playerontheserver()
			if count == 0 then
				if paused == false then
					DiscordBot.Data.paused = true
					COM_BufInsertText(server, "server_log players")
					COM_BufInsertText(server, "server_log pause")
					COM_BufInsertText(server, "pause")
				end
			end
		end
	end
end

addHook("ThinkFrame", bot_function)

-- Cache player teams each tick (player.ctfteam may not be set at PlayerMsg time
-- for custom gametypes like Battlemod, where team assignment happens in hooks).
local _player_team = {}
addHook("ThinkFrame", function()
	for p in players.iterate do
		local t = tonumber(p.ctfteam)
		_player_team[#p] = (t == 1 or t == 2) and t or 0
	end
end)

addHook("PlayerMsg", function(player, type, target, msg)
	if not player then return end
	if type == 0 then
		if server ~= player and target and target ~= 0 then return end
		local text = nil
		local message = msg
		local sendit = false
		if server == player then
			text = "[EVENT:SERVER_CHAT]|" .. message .. "|FEE75C\n"
			DiscordBot.Functions.spamchatbug(player, text)
			DiscordBot.Functions.flush_msgsrb2()
			chatprint("<\x82~\x80Server>" .. " " .. message)
			return true
		end
		local jointime = (DiscordBot._join_times and DiscordBot._join_times[#player]) and tostring(DiscordBot._join_times[#player]) or "0"
		if DiscordBot.Data.debug then print("[DEBUG] PlayerMsg jointime=" .. jointime .. " (tbl=" .. tostring(DiscordBot._join_times) .. " val=" .. tostring(DiscordBot._join_times and DiscordBot._join_times[#player]) .. ")" .. " node=" .. #player .. " player=" .. player.name) end
		local flag = player.gotflag and player.gotflag > 0 and "1" or "0"
		local team = tostring(_player_team[#player] or 0)
		text = "[EVENT:CHAT]|[" .. #player .. "]|" .. player.name .. "|" .. message .. "|" .. (skins[player.skin] and skins[player.skin].name or "") .. "|" .. jointime .. "|" .. flag .. "|" .. team .. "\n"
		if IsPlayerAdmin(player) then
			text = "[EVENT:CHAT]|[" .. #player .. "]|@" .. player.name .. "|" .. message .. "|" .. (skins[player.skin] and skins[player.skin].name or "") .. "|" .. jointime .. "|" .. flag .. "|" .. team .. "\n"
		end
		if text then
			sendit = DiscordBot.Functions.spamchatbug(player, text)
			if sendit == true then
				DiscordBot.Functions.flush_msgsrb2()
				return false
			end
			if sendit == false then
				chatprintf(player, "Wait a second before sending a message and chat again.")
				return true
			end
		end
	elseif type == 3 then
		local text = "[EVENT:CSAY]|" .. msg .. "\n"
		DiscordBot.Data.msgsrb2 = DiscordBot.Data.msgsrb2 .. text
		DiscordBot.Functions.flush_msgsrb2()
	end
end)

addHook("PlayerJoin", function(playernum)
	if not DiscordBot._pending_joins then DiscordBot._pending_joins = {} end
	DiscordBot._pending_joins[playernum] = true
end)

addHook("ThinkFrame", function()
	if not DiscordBot._pending_joins then return end
	for player in players.iterate do
		if DiscordBot._pending_joins[#player] then
			if not DiscordBot._join_emitted then DiscordBot._join_emitted = {} end
			if not DiscordBot._join_emitted[#player] then
				DiscordBot._join_emitted[#player] = true
				DiscordBot._join_times = DiscordBot._join_times or {}
				DiscordBot._join_times[#player] = os.time()
				if DiscordBot.Data.debug then print("[DEBUG] _join_times[" .. #player .. "] = " .. tostring(DiscordBot._join_times[#player]) .. " (os.time=" .. tostring(os.time()) .. ")" .. " player=" .. player.name) end
				DiscordBot.Data.msgsrb2 = DiscordBot.Data.msgsrb2 .. "[EVENT:PLAYER_JOIN]|" .. player.name .. "|" .. #player .. "\n"
				DiscordBot.Functions.flush_msgsrb2()
			end
			if DiscordBot.Config.auto_pause and paused == true then
				DiscordBot.Data.paused = false
				COM_BufInsertText(server, "pause")
			end
		end
	end
end)

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

addHook("PlayerQuit", function(player, reason)
	if DiscordBot.Commands.cv_joinquit.value ~= 1 then return end
	if DiscordBot._pending_joins then DiscordBot._pending_joins[#player] = nil end
	if DiscordBot._join_emitted then DiscordBot._join_emitted[#player] = nil end
	if DiscordBot._join_times then DiscordBot._join_times[#player] = nil end
	DiscordBot.Data.msgsrb2 = DiscordBot.Data.msgsrb2 .. "[EVENT:PLAYER_QUIT]|" .. player.name .. "|" .. #player .. "|" .. reason_to_string(reason) .. "\n"
	if reason == KR_KICK then
		DiscordBot.Data.msgsrb2 = DiscordBot.Data.msgsrb2 .. "[EVENT:KICK_PLAYER]|" .. player.name .. "|" .. #player .. "|" .. reason_to_string(reason) .. "\n"
	end
	if reason == KR_BAN then
		DiscordBot.Data.msgsrb2 = DiscordBot.Data.msgsrb2 .. "[EVENT:BAN_PLAYER]|" .. player.name .. "|" .. #player .. "|" .. reason_to_string(reason) .. "\n"
	end
	DiscordBot.Functions.flush_msgsrb2()
end)

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

local function json_escape(s)
	s = string.gsub(s, '\\', '\\\\')
	s = string.gsub(s, '"', '\\"')
	s = string.gsub(s, '\n', '\\n')
	s = string.gsub(s, '\r', '\\r')
	s = string.gsub(s, '\t', '\\t')
	return s
end

local function map_num_to_mapstr(n)
	if n < 100 then
		return string.format("MAP%02d", n)
	end
	local val = n - 100
	local first = string.char(((val - val % 36) / 36) + string.byte('A'))
	local r = val % 36
	local second
	if r < 10 then
		second = string.char(r + string.byte('0'))
	else
		second = string.char(r - 10 + string.byte('A'))
	end
	return "MAP" .. first .. second
end

local function emit_round_end(prev_map, prev_maptitle)
	local gtname = get_gametype_name(gametype)
	local mapstr = map_num_to_mapstr(prev_map)

	local players_json = "["
	local spec_json = "["
	local first = true
	local spec_first = true
	local players_total = 0
	local players_red = 0
	local players_blue = 0
	local players_spec = 0
	local has_teams = false
	-- Detect team mode from player data (more reliable than gametype rules bitfield)
	for p in players.iterate do
		if not p.spectator and (p.ctfteam == 1 or p.ctfteam == 2) then
			has_teams = true
			break
		end
	end
	for p in players.iterate do
		players_total = $ + 1
		if p.spectator then
			players_spec = $ + 1
			if not spec_first then spec_json = spec_json .. "," end
			spec_json = spec_json .. '"' .. json_escape(p.name) .. '"'
			spec_first = false
		else
			local team = "ffa"
			if p.ctfteam == 1 then team = "red"; players_red = $ + 1
			elseif p.ctfteam == 2 then team = "blue"; players_blue = $ + 1 end
			if not first then players_json = players_json .. "," end
			players_json = players_json .. '{"name":"' .. json_escape(p.name) .. '","score":' .. (p.score or 0) .. ',"team":"' .. team .. '"}'
			first = false
		end
	end
	players_json = players_json .. "]"
	spec_json = spec_json .. "]"

	local mode = has_teams and "team" or "ffa"

	-- Encode player JSON without | chars so it survives the pipe-delimited event line
	local players_safe = string.gsub(string.gsub(players_json, '|', '\\x7c'), '\n', '\\n')
	local spec_safe = string.gsub(string.gsub(spec_json, '|', '\\x7c'), '\n', '\\n')

	local round_seconds = (leveltime - leveltime % 35) / 35
	local round_mins = (round_seconds - round_seconds % 60) / 60
	local round_secs = round_seconds % 60
	local round_time_str = round_mins .. ":" .. string.format("%02d", round_secs)

	local server_up_tics = (os.time() - DiscordBot.Data.server_started) * 35

	local pointlimit = 0
	pcall(function()
		local battletbl = _G["CBW_Battle"]
		if battletbl and battletbl.GametypeIDtoIdentifier then
			local id = battletbl.GametypeIDtoIdentifier[gametype]
			if id then
				local cv = _G[id .. "_pointlimit"]
				if cv and cv.value then pointlimit = tonumber(cv.value) or 0 end
			end
		end
	end)

	local end_line = "[EVENT:ROUND_END]|" .. gtname .. "|" .. mapstr .. "|" .. leveltime
		.. "|" .. server_up_tics .. "|" .. players_total .. "|" .. players_red
		.. "|" .. players_blue .. "|" .. players_spec .. "|" .. mode
		.. "|" .. (redscore or 0) .. "|" .. (bluescore or 0)
		.. "|" .. round_time_str .. "|" .. json_escape(prev_maptitle)
		.. "|" .. players_safe .. "|" .. spec_safe .. "|" .. pointlimit .. "|" .. gametype .. "\n"
	DiscordBot.Data.msgsrb2 = DiscordBot.Data.msgsrb2 .. end_line
	DiscordBot.Functions.flush_msgsrb2()
	DiscordBot.Data.round_end_emitted = true
end

-- Emit ROUND_END on the first frame of intermission.
-- Uses IntermissionThinker (SRB2 2.2.12+) or falls back to ThinkFrame.
pcall(function() addHook("IntermissionThinker", function()
	if DiscordBot.Data.current_map and not DiscordBot.Data.round_end_emitted then
		local title = DiscordBot.Data.maptitle
		if title == nil or title == "" then title = "Unknown" end
		emit_round_end(DiscordBot.Data.current_map, title)
	end
end) end)

-- Fallback: detect intermission via ThinkFrame (for older SRB2 without IntermissionThinker).
addHook("ThinkFrame", function()
	if DiscordBot.Data.current_map and not DiscordBot.Data.round_end_emitted
	   and (gamestate == GS_INTERMISSION or gamestate == GS_NULL) then
		local title = DiscordBot.Data.maptitle
		if title == nil or title == "" then title = "Unknown" end
		emit_round_end(DiscordBot.Data.current_map, title)
	end
end)

addHook("MapChange", function(map)
	-- Emit ROUND_END if IntermissionThinker didn't fire (skipstats path, GS_NULL).
	-- NOT on `map` command (gamestate is GS_LEVEL).
	if DiscordBot.Data.current_map ~= nil and not DiscordBot.Data.round_end_emitted
	   and gamestate == GS_NULL then
		local title = DiscordBot.Data.maptitle
		if title == nil or title == "" then title = "Unknown" end
		emit_round_end(DiscordBot.Data.current_map, title)
	end
	DiscordBot.Data.round_end_emitted = false
	DiscordBot.Data.current_map = nil

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
	DiscordBot.Data.current_map = map
	local event_line = "[EVENT:ROUND_START]|" .. gtname .. "|" .. mapstr .. "|" .. maptitle .. "|" .. gametype .. "\n"
	DiscordBot.Data.msgsrb2 = DiscordBot.Data.msgsrb2 .. event_line
	DiscordBot.Functions.flush_msgsrb2()
end)
