rawset(_G, "DiscordBot", {})
DiscordBot.version = "0.1.35"
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
DiscordBot.Data.current_map = nil
DiscordBot.Data.debug = false

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
				iconskin = ":" .. player.mo.skin .. ":"
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
			logmsg:write(DiscordBot.Data.msgsrb2)
			logmsg:close()
			DiscordBot.Data.msgsrb2 = ''
		end
	elseif arg == "logcom" then
		if DiscordBot.Data.log then
			local logcom = io.openlocal("client/DiscordBot/logcom.txt", "a+")
			logcom:write(DiscordBot.Data.log)
			logcom:close()
		end
	elseif arg == "pause" then
		if DiscordBot.Data.pcmtsrb2 then
			local logmsg = io.openlocal("client/DiscordBot/Players.txt", "w")
			logmsg:write("Game is paused, no players")
			logmsg:close()
		end
	elseif arg == "players" then
		if DiscordBot.Data.pcmtsrb2 then
			local logmsg = io.openlocal("client/DiscordBot/Players.txt", "w")
			logmsg:write(DiscordBot.Data.pcmtsrb2)
			logmsg:close()
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
			local sseconds = G_TicsToSeconds(DiscordBot.Data.servertime)
			if string.len(sseconds) == 1 then sseconds = "0" .. tostring(sseconds) end
			local stime = G_TicsToMinutes(DiscordBot.Data.servertime, true) .. ":" .. sseconds
			if DiscordBot.Data.paused == true then
				DiscordBot.Data.stats = "There's no one here."
			end
			local logmsg = io.openlocal("client/DiscordBot/Stats.txt", "w")
			logmsg:write(cv_servername.string .. "\n" .. DiscordBot.Data.maptitle .. " (" .. gamemap .. ")\n" .. gamemap .. "\n" .. DiscordBot.Data.nextlevel .. "\n" .. DiscordBot.Data.iconemeralds .. "\n" .. playerskins .. "\n" .. ltime .. "\n" .. stime .. "\n" .. DiscordBot.Data.pcmp .. "\n" .. DiscordBot.Data.stats)
			logmsg:close()
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
					local d_console = io.openlocal("client/DiscordBot/console.txt", "w")
					d_console:write("")
					d_console:close()
				end
			end
		end
	elseif arg == "discord" then
		if DiscordBot.Data.debug then COM_BufInsertText(server, "echo [DBOT] server_log discord running") end
		local d_msg = io.openlocal("client/DiscordBot/discordmessage.txt", "r")
		if d_msg then
			local clear = false
			while true do
				local line = d_msg:read("*l") or ""
				if line == "" then break end
				if #line > 220 then
					COM_BufInsertText(server, "discord_message " .. string.sub(line, 1, 220))
					local remainder = string.sub(line, 221)
					if #remainder > 0 then
						COM_BufInsertText(server, "discord_message " .. remainder)
					end
				else
					COM_BufInsertText(server, "discord_message " .. line)
				end
				clear = true
			end
			d_msg:close()
			if clear == true then
				local d_clear = io.openlocal("client/DiscordBot/discordmessage.txt", "w")
				if d_clear then
					d_clear:write("")
					d_clear:close()
				end
			end
		end
	end
end, COM_LOCAL)

COM_AddCommand("discord_message", function(player, ...)
	if player ~= server then return end
	if not ... then return end
	local args = { ... }
	local msg = table.concat(args, " ", 1, #args)
	if DiscordBot.Data.debug then COM_BufInsertText(server, "echo [DBOT] discord_message called: " .. msg) end
	chatprint("\x89" .. "[Discord]" .. "\x80" .. msg, true)
end)

COM_AddCommand("dbot_sync", function(player)
	if player ~= server then return end
	if DiscordBot.Data.debug then print("[DEBUG] dbot_sync: re-emitting server state (map=" .. tostring(DiscordBot.Data.current_map) .. ")") end
	DiscordBot.Data.msgsrb2 = ''
	if DiscordBot.Data.current_map ~= nil then
		if DiscordBot.Data.debug then print("[DEBUG] dbot_sync: re-emitting initial map state") end
	end
	DiscordBot.Functions.flush_msgsrb2()
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

addHook("PlayerMsg", function(player, type, target, msg)
	if not player then return end
	if not DiscordBot._player_msg_cache then DiscordBot._player_msg_cache = {} end
	local cache = DiscordBot._player_msg_cache
	local cache_key = tostring(#player) .. "|" .. tostring(type) .. "|" .. tostring(target) .. "|" .. msg
	if cache[cache_key] == leveltime then
		return true
	end
	cache[cache_key] = leveltime
	if type == 0 then
		if server ~= player and target and target ~= 0 then return end
		local text = nil
		local message = msg
		local sendit = false
		if server == player then
			if isdedicatedserver == true then
				text = "[EVENT:SERVER_CHAT]|" .. message .. "|FEE75C\n"
				DiscordBot.Functions.spamchatbug(player, text)
				DiscordBot.Functions.flush_msgsrb2()
				chatprint("<\x82~\x80Server>" .. " " .. message)
				return true
			end
		end
		local jointime = (DiscordBot.join_times and DiscordBot.join_times[#player]) and tostring(DiscordBot.join_times[#player]) or "0"
		local flag = player.gotflag and player.gotflag > 0 and "1" or "0"
		local team = "none"
		if not player.spectator then
			if gametype == GT_CTF or gametype == GT_TEAMMATCH or gametype == GT_TEAMBATTLE then
				team = tostring(player.ctfteam or 0)
			end
		end
		text = "[EVENT:CHAT]|[" .. #player .. "]|" .. player.name .. "|" .. message .. "|" .. (player.mo and player.mo.skin or "") .. "|" .. jointime .. "|" .. flag .. "|" .. team .. "\n"
		if IsPlayerAdmin(player) then
			text = "[EVENT:CHAT]|[" .. #player .. "]|@" .. player.name .. "|" .. message .. "|" .. (player.mo and player.mo.skin or "") .. "|" .. jointime .. "|" .. flag .. "|" .. team .. "\n"
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
	if not DiscordBot.pending_joins then DiscordBot.pending_joins = {} end
	DiscordBot.pending_joins[playernum] = true
end)

addHook("ThinkFrame", function()
	if not DiscordBot.pending_joins then return end
	for player in players.iterate do
		if DiscordBot.pending_joins[#player] then
			if not DiscordBot.join_emitted then DiscordBot.join_emitted = {} end
			if DiscordBot.join_emitted[#player] then
				-- already emitted for this node, skip duplicate
			else
				DiscordBot.join_emitted[#player] = true
				DiscordBot.join_times = DiscordBot.join_times or {}
				DiscordBot.join_times[#player] = os.time()
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
	if DiscordBot.pending_joins then DiscordBot.pending_joins[#player] = nil end
	if DiscordBot.join_emitted then DiscordBot.join_emitted[#player] = nil end
	if DiscordBot.join_times then DiscordBot.join_times[#player] = nil end
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

addHook("MapChange", function(map)
	-- Emit ROUND_END when the map actually changes after a round end.
	-- Skip death reloads (same map), forced `map` commands (GS_LEVEL),
	-- and gametypes without intermission (co-op, race, tag, hide & seek).
	if DiscordBot.Data.current_map ~= nil and DiscordBot.Data.current_map ~= map
	   and gamestate ~= GS_LEVEL
	   and gametype ~= GT_COOP and gametype ~= GT_COMPETITION
	   and gametype ~= GT_RACE and gametype ~= GT_TAG
	   and gametype ~= GT_HIDEANDSEEK then
		local players_total = 0
		local players_red = 0
		local players_blue = 0
		local players_spec = 0
		for p in players.iterate do
			players_total = $ + 1
			if p.spectator then
				players_spec = $ + 1
			elseif p.ctfteam == 1 then
				players_red = $ + 1
			elseif p.ctfteam == 2 then
				players_blue = $ + 1
			end
		end
		local gtname = get_gametype_name(gametype)
		local mapstr = map_num_to_mapstr(DiscordBot.Data.current_map)
		local end_line = "[EVENT:ROUND_END]|" .. gtname .. "|" .. mapstr .. "|" .. leveltime .. "|" .. DiscordBot.Data.servertime .. "|" .. players_total .. "|" .. players_red .. "|" .. players_blue .. "|" .. players_spec .. "\n"
		DiscordBot.Data.msgsrb2 = DiscordBot.Data.msgsrb2 .. end_line
		DiscordBot.Functions.flush_msgsrb2()
	end

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
	local event_line = "[EVENT:ROUND_START]|" .. gtname .. "|" .. mapstr .. "|" .. maptitle .. "\n"
	DiscordBot.Data.msgsrb2 = DiscordBot.Data.msgsrb2 .. event_line
	DiscordBot.Functions.flush_msgsrb2()
end)
