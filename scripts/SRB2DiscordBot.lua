rawset(_G, "DiscordBot", {})
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
DiscordBot.Data.round_active = false
DiscordBot.Data.current_map = nil
DiscordBot.Data.debug = false

DiscordBot.Data.skin_colors = {
	[0] = "FFFFFF",
	[1] = "E00000",
	[2] = "00C000",
	[3] = "4000FF",
	[4] = "F0F000",
	[5] = "FF8000",
	[6] = "A000FF",
	[7] = "00E8E8",
	[8] = "FF80C0",
	[9] = "D0A070",
	[10] = "C080FF",
	[11] = "008080",
	[12] = "804000",
	[13] = "A0A0A0",
	[14] = "804040",
	[15] = "808000",
}
DiscordBot.Functions = {}
DiscordBot.Functions.get_skin_color = function(player)
	if DiscordBot.Data.skin_colors[player.skincolor]
 then
		return DiscordBot.Data.skin_colors[player.skincolor]
	end
	return "2F3136"
end

DiscordBot.Commands = {}
DiscordBot.Commands.cv_joinquit = CV_RegisterVar({name = "dbot_joinquit", defaultvalue = "On", flags = CV_NETVAR, PossibleValue = CV_OnOff})
DiscordBot.Commands.cv_autopause = CV_RegisterVar({name = "dbot_autopause", defaultvalue = "On", flags = CV_NETVAR, PossibleValue = CV_OnOff})
DiscordBot.Commands.cv_nospamchat = CV_RegisterVar({name = "dbot_nospamchat", defaultvalue = "Off", flags = CV_NETVAR, PossibleValue = CV_OnOff})
DiscordBot.Commands.cv_messagedelay = CV_RegisterVar({name = "dbot_messagedelay", defaultvalue = "On", flags = CV_NETVAR, PossibleValue = CV_OnOff})

DiscordBot.Messages = {}

DiscordBot.Functions.flush_msgsrb2 = function()
    if DiscordBot.Data.msgsrb2 and DiscordBot.Data.msgsrb2 != ''
 then
        if DiscordBot.Data.debug then print("[DEBUG] flush_msgsrb2: writing "..string.len(DiscordBot.Data.msgsrb2).." bytes to Messages.txt") end
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
	if DiscordBot.Commands.cv_nospamchat.value == 0 and not joinquit
 then
		DiscordBot.Data.msgsrb2 = DiscordBot.Data.msgsrb2..msg
		return true
	end
	if player != server
 then
		if not DiscordBot.Messages[player.name] then DiscordBot.Messages[player.name] = DiscordBot.Data.servertime checked = true end
		//if not DiscordBot.Messages[player.name][msg] then DiscordBot.Messages[player.name][msg] = DiscordBot.Data.servertime checked = true end
		if checked == false
 then
			//if DiscordBot.Data.servertime - DiscordBot.Messages[player.name][msg] < 5*TICRATE then return false end
			if DiscordBot.Data.servertime - DiscordBot.Messages[player.name] < 35 then DiscordBot.Messages[player.name] = DiscordBot.Data.servertime return false end
		end
		DiscordBot.Messages[player.name] = DiscordBot.Data.servertime
	end
	DiscordBot.Data.msgsrb2 = DiscordBot.Data.msgsrb2..msg
	if DiscordBot.Commands.cv_messagedelay.value == 0
 then
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
		if player
 then
			count = $ + 1
		end
	end
	return count
end

DiscordBot.Functions.statsofserver = function()
	local playerstats = ''
	for player in players.iterate do
		if player
 then
			local pname = string.gsub(player.name, "`", "")
			local ping = 0
			local statms = ''
			local iconskin = ':unknown:'
			local pffinished = ':black_large_square:'
			local admin = ':black_large_square:'
			if player.ping then ping = player.ping end
			if (ping < 32) then statms = ':ping_blue:'
			elseif (ping < 64) then statms = ':ping_green:'
			elseif (ping < 128) then statms = ':ping_yellow:'
			elseif (ping < 256) then statms = ':ping_red:' end
			if player.mo and player.spectator != true
 then
				iconskin = ":"..player.mo.skin..":"
			end
			if player.spectator == true
 then
				iconskin = ':spectator:'
			elseif player.playerstate == PST_DEAD
 then
				iconskin = ':dead:'
			end
			if player.mo and (player.pflags & PF_FINISHED) then pffinished = ":completed:" end
			if IsPlayerAdmin(player) then admin = ":remote_admin:" end
			if player.playtime == nil then player.playtime = 0 end
			local seconds = G_TicsToSeconds(player.playtime)
			if string.len(seconds) == 1 then seconds = "0"..tostring(seconds) end
			local pptime = G_TicsToMinutes(player.playtime, true)..":"..seconds
			playerstats = $ + statms..iconskin..pffinished..admin.."["..#player.."] `"..pname.."`: Score - "..player.score.."; Time - "..pptime.."\n"
		end
	end
	if playerstats == ''
 then
		playerstats = "There's no one here."
	end
	return playerstats
end

COM_AddCommand("server_log", function(player, arg, text)
	if player != server then return end
	if arg == "msg"
 then
		if DiscordBot.Data.msgsrb2
 then
			local logmsg = io.openlocal("client/DiscordBot/Messages.txt", "a")
			logmsg:write(DiscordBot.Data.msgsrb2)
			logmsg:close()
			DiscordBot.Data.msgsrb2 = ''
		end
	elseif arg == "logcom"
 then
		if DiscordBot.Data.log
 then
			local logcom = io.openlocal("client/DiscordBot/logcom.txt", "a+")
			logcom:write(DiscordBot.Data.log)
			logcom:close()
		end
	elseif arg == "pause"
 then
		if DiscordBot.Data.pcmtsrb2
 then
			local logmsg = io.openlocal("client/DiscordBot/Players.txt", "w")
			logmsg:write("Game is paused, no players")
			logmsg:close()
		end
	elseif arg == "players"
 then
		if DiscordBot.Data.pcmtsrb2
 then
			local logmsg = io.openlocal("client/DiscordBot/Players.txt", "w")
			logmsg:write(DiscordBot.Data.pcmtsrb2)
			logmsg:close()
		end
		if DiscordBot.Data.stats
 then
			-- Server name
			local cv_servername = CV_FindVar("servername")
			local playerskins = ''
			-- skins
			for i = 0, #skins-1 do
				playerskins = playerskins..":"..skins[i].name..": "
			end
			-- Leveltime
			local lseconds = G_TicsToSeconds(leveltime)
			if string.len(lseconds) == 1 then lseconds = "0"..tostring(lseconds) end
			local ltime = G_TicsToMinutes(leveltime, true)..":"..lseconds
			-- Servertime
			local sseconds = G_TicsToSeconds(DiscordBot.Data.servertime)
			if string.len(sseconds) == 1 then sseconds = "0"..tostring(sseconds) end
			local stime = G_TicsToMinutes(DiscordBot.Data.servertime, true)..":"..sseconds
			if DiscordBot.Data.paused == true
 then
				DiscordBot.Data.stats = "There's no one here."
			end
			local logmsg = io.openlocal("client/DiscordBot/Stats.txt", "w")
			logmsg:write(cv_servername.string.."\n"..DiscordBot.Data.maptitle.." ("..gamemap..")\n"..gamemap.."\n"..DiscordBot.Data.nextlevel.."\n"..DiscordBot.Data.iconemeralds.."\n"..playerskins.."\n"..ltime.."\n"..stime.."\n"..DiscordBot.Data.pcmp.."\n"..DiscordBot.Data.stats)
			logmsg:close()
		end
	elseif arg == "console"
 then
		if DiscordBot.Data.console
 then
			local d_console = io.openlocal("client/DiscordBot/console.txt", "r")
			if d_console
 then
				local clear = false
				while true do
					local line = ''
					line = d_console:read("*l") or $
					if line == "" then break end
					line = string.sub(line,1 , 220)
					if string.find(string.sub(string.lower(line),1,5), string.lower("quit")) == nil and string.find(string.sub(string.lower(line),1,9), string.lower("exitgame")) == nil
 then
						COM_BufInsertText(server, line)
					end
					clear = true
				end
				d_console:close()
				if clear == true
 then
					local d_console = io.openlocal("client/DiscordBot/console.txt", "w")
					d_console:write("")
					d_console:close()
				end
			end
		end
	elseif arg == "discord"
 then
		local d_msg = io.openlocal("client/DiscordBot/discordmessage.txt", "r")
		if d_msg
 then
			local clear = false
			while true do
				local line = ''
				line = d_msg:read("*l") or $
				if line == "" then break end
				if #line > 220
 then
					COM_BufInsertText(server, "discord_message "..string.sub(line, 1, 220))
					local remainder = string.sub(line, 221)
					if #remainder > 0
 then
						COM_BufInsertText(server, "discord_message "..remainder)
					end
				else
					COM_BufInsertText(server, "discord_message "..line)
				end
				clear = true
			end
			d_msg:close()
			if clear == true
 then
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
	if player != server then return end
	if not ... then return end
	local args = {...}
	local msg = table.concat(args, " ", 1, #args)
	chatprint("\x89".."[Discord]".."\x80"..msg, true)
end)

COM_AddCommand("dbot_sync", function(player)
	if player != server then return end
	if DiscordBot.Data.debug then print("[DEBUG] dbot_sync: re-emitting server state (map="..tostring(DiscordBot.Data.current_map)..", round_active="..tostring(DiscordBot.Data.round_active)..")") end
	DiscordBot.Data.msgsrb2 = ''
	if DiscordBot.Data.current_map ~= nil and DiscordBot.Data.round_active
 then
		if DiscordBot.Data.debug then print("[DEBUG] dbot_sync: round is active, waiting for next MapLoad to emit ROUND_START") end
	end
	DiscordBot.Functions.flush_msgsrb2()
end, COM_LOCAL)

COM_AddCommand("dbot_debug", function(player, arg)
	if player != server then return end
	DiscordBot.Data.debug = (arg == "on")
end, COM_LOCAL)

local function bot_function()
	DiscordBot.Data.servertime = DiscordBot.Data.servertime + 1
	if (leveltime % 70) == 35
 then
		DiscordBot.Data.stats = DiscordBot.Functions.statsofserver()
		local cv_maxplayer = CV_FindVar("maxplayers")
		local playercount = 0
		DiscordBot.Data.countemeralds = 0
		DiscordBot.Data.iconemeralds = ''
		playercount = DiscordBot.Functions.playerontheserver()
		if not playercount
 then
			playercount = 0
		end
		if DiscordBot.Data.gametype != gametype
 then
			COM_BufInsertText(server, "gametype")
			DiscordBot.Data.gametype = gametype
		end
		if not G_IsSpecialStage(gamemap)
 then
			if mapheaderinfo[gamemap].nextlevel != nil
 then
				if mapheaderinfo[gamemap].nextlevel >= 1100
 then
					DiscordBot.Data.nextlevel = "Ending"
				else
					local nextlevelint = mapheaderinfo[gamemap].nextlevel
					local nextlevel = mapheaderinfo[mapheaderinfo[gamemap].nextlevel]
					if nextlevel != nil
 then
						if nextlevel.actnum == 0
 then
							DiscordBot.Data.nextlevel = (nextlevel.lvlttl.." ("..nextlevelint..")")
						else
							DiscordBot.Data.nextlevel = (nextlevel.lvlttl.." Act "..nextlevel.actnum.." ("..nextlevelint..")")
						end
					else
						DiscordBot.Data.nextlevel =  "None"
					end
				end
			end
			if gametype != GT_COOP
 then
				local cv_advancemap = CV_FindVar("advancemap") 
				if cv_advancemap.value == 0
 then
					if mapheaderinfo[gamemap].actnum == 0
 then
						DiscordBot.Data.nextlevel = (mapheaderinfo[gamemap].lvlttl.." ("..gamemap..")")
					else
						DiscordBot.Data.nextlevel = (mapheaderinfo[gamemap].lvlttl.." Act "..mapheaderinfo[gamemap].actnum.." ("..gamemap..")")
					end
				elseif cv_advancemap.value == 2
 then
					DiscordBot.Data.nextlevel = "Random"
				end
			end
		end
		if mapheaderinfo[gamemap].actnum == 0
 then
			DiscordBot.Data.maptitle = (mapheaderinfo[gamemap].lvlttl)
		else
			DiscordBot.Data.maptitle = (mapheaderinfo[gamemap].lvlttl.." Act "..mapheaderinfo[gamemap].actnum)
		end
		if emeralds & EMERALD1
 then
			DiscordBot.Data.countemeralds = $ + 1
			DiscordBot.Data.iconemeralds = DiscordBot.Data.iconemeralds..":emerald1:"
		end
		if emeralds & EMERALD2
 then
			DiscordBot.Data.countemeralds = $ + 1
			DiscordBot.Data.iconemeralds = DiscordBot.Data.iconemeralds..":emerald2:"
		end
		if emeralds & EMERALD3
 then
			DiscordBot.Data.countemeralds = $ + 1
			DiscordBot.Data.iconemeralds = DiscordBot.Data.iconemeralds..":emerald3:"
		end
		if emeralds & EMERALD4
 then
			DiscordBot.Data.countemeralds = $ + 1
			DiscordBot.Data.iconemeralds = DiscordBot.Data.iconemeralds..":emerald4:"
		end
		if emeralds & EMERALD5
 then
			DiscordBot.Data.countemeralds = $ + 1
			DiscordBot.Data.iconemeralds = DiscordBot.Data.iconemeralds..":emerald5:"
		end
		if emeralds & EMERALD6
 then
			DiscordBot.Data.countemeralds = $ + 1
			DiscordBot.Data.iconemeralds = DiscordBot.Data.iconemeralds..":emerald6:"
		end
		if emeralds & EMERALD7
 then
			DiscordBot.Data.countemeralds = $ + 1
			DiscordBot.Data.iconemeralds = DiscordBot.Data.iconemeralds..":emerald7:"
		end
		if emeralds == 0
 then
			DiscordBot.Data.iconemeralds = "No emeralds"
		end
		if playercount
 then
			DiscordBot.Data.pcmp = (playercount.."/"..cv_maxplayer.value)
			DiscordBot.Data.pcmtsrb2 = ("Players : "..playercount.."/"..cv_maxplayer.value)
		else
			DiscordBot.Data.pcmp = ("0/"..cv_maxplayer.value)
			DiscordBot.Data.pcmtsrb2 = ("Players : 0/"..cv_maxplayer.value)
		end
		COM_BufInsertText(server, "server_log discord")
		COM_BufInsertText(server, "server_log console")
		DiscordBot.Functions.flush_msgsrb2()
		if DiscordBot.Data.log != ''
 then
			COM_BufInsertText(server, "server_log logcom")
			DiscordBot.Data.log = ''
		end
		COM_BufInsertText(server, "server_log players")
		if DiscordBot.Commands.cv_autopause.value == 1
 then
			local count = 0
			count = DiscordBot.Functions.playerontheserver()
			if count == 0
 then
				if paused == false
 then
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
	if type == 0 then
		if server ~= player and target ~= 0 then return end
		local text = nil
		local message = msg
		local sendit = false
		--[[ /*
		message = string.gsub(message, "@owners", "<@&1007014838326796298>")
		message = string.gsub(message, "@moderators", "<@&1007015806716096645>")
		message = string.gsub(message, "@Owners", "<@&1007014838326796298>")
		message = string.gsub(message, "@Moderators", "<@&1007015806716096645>")
		*/ --]]
		if server == player then
			if isdedicatedserver == true then
				text = "[EVENT:SERVER_CHAT]|"..message.."|FEE75C\n"
				DiscordBot.Functions.spamchatbug(player, text)
				DiscordBot.Functions.flush_msgsrb2()
				chatprint("<\x82~\x80Server>".." "..message)
				return true
			end
		end
		text = "[EVENT:CHAT]|["..#player.."]|**<"..player.name..">**|"..message.."|"..DiscordBot.Functions.get_skin_color(player).."\n"
		if IsPlayerAdmin(player) then text = "[EVENT:CHAT]|["..#player.."]|**<@"..player.name..">**|"..message.."|"..DiscordBot.Functions.get_skin_color(player).."\n" end
		if text
 then
			sendit = DiscordBot.Functions.spamchatbug(player, text)
			if sendit == true
 then
				DiscordBot.Functions.flush_msgsrb2()
				return false
			end
			if sendit == false
 then
				//chatprintf(player, "You're repeating yourself, please wait "..((5*TICRATE - (DiscordBot.Data.servertime - DiscordBot.Messages[player.name][text]))/TICRATE).." sec. or send a different message.")
				chatprintf(player, "Wait a second before sending a message and chat again.")
				return true
			end
		end
	elseif type == 3
 then
		local text = nil
		text = "[EVENT:CSAY]|"..msg.."\n"
		DiscordBot.Data.msgsrb2 = DiscordBot.Data.msgsrb2..text
		DiscordBot.Functions.flush_msgsrb2()
	end
end)

addHook("PlayerThink", function(player)
	if not player.playtime then player.playtime = 0 end
	if player.playtime != nil then player.playtime = $ + 1 end
	if not player.oldname then player.oldname = player.name end
	if player.name != player.oldname
 then
	player.name = string.gsub(player.name, "`", "")
		local text = "[EVENT:CHAT]|["..#player.."]|System|:pencil2:"..string.gsub(player.oldname, "*", "").." renamed to "..string.gsub(player.name, "*", "")..":pencil2:\n"
		DiscordBot.Data.msgsrb2 = DiscordBot.Data.msgsrb2..text
		player.oldname = player.name
	end
	if DiscordBot.Commands.cv_joinquit.value == 1
 then
		if player.logjoin != true
 then
			player.logjoin = true
			-- Deduplication guard: track emitted joins by player node
			if not DiscordBot.Data.emitted_joins then DiscordBot.Data.emitted_joins = {} end
			local dup_key = tostring(#player)
			if DiscordBot.Data.emitted_joins[dup_key] then
				if DiscordBot.Data.debug then print("[DEBUG] PlayerThink: skipping duplicate join for node "..dup_key) end
			else
				DiscordBot.Data.emitted_joins[dup_key] = true
				DiscordBot.Data.msgsrb2 = DiscordBot.Data.msgsrb2.."[EVENT:PLAYER_JOIN]|"..player.name.."|"..#player.."\n"
				DiscordBot.Functions.flush_msgsrb2()
			end
		end
	end
end, MT_PLAYER)

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

addHook("PlayerJoin", function(playernum)
	--unpause if player has joined the game
	if DiscordBot.Commands.cv_autopause.value == 1
 then
		if paused == true
 then
			DiscordBot.Data.paused = false
			COM_BufInsertText(server, "pause")
		end
	end
end)

addHook("PlayerQuit", function(player, reason)
	if DiscordBot.Commands.cv_joinquit.value != 1 then return end
	player.quitlog = true
	-- Allow rejoin events for this node
	if DiscordBot.Data.emitted_joins then DiscordBot.Data.emitted_joins[tostring(#player)] = nil end
	DiscordBot.Data.msgsrb2 = DiscordBot.Data.msgsrb2.."[EVENT:PLAYER_QUIT]|"..player.name.."|"..#player.."|"..reason_to_string(reason).."\n"
	if reason == KR_KICK then DiscordBot.Data.msgsrb2 = DiscordBot.Data.msgsrb2.."[EVENT:KICK_PLAYER]|"..player.name.."|"..#player.."|"..reason_to_string(reason).."\n" end
	if reason == KR_BAN then DiscordBot.Data.msgsrb2 = DiscordBot.Data.msgsrb2.."[EVENT:BAN_PLAYER]|"..player.name.."|"..#player.."|"..reason_to_string(reason).."\n" end
	DiscordBot.Functions.flush_msgsrb2()
end)

addHook("NetVars", function(n)
	 DiscordBot.Data = n($)
	 DiscordBot.Messages = n($)
end)

local function get_gametype_name(gt)
	-- Use SRB2's internal gametype name when available. This resolves
	-- all built-in gametypes with their exact 1-to-1 SRB2 names
	-- (Co-op, Competition, Match, Team Match, Tag, Hide & Seek, CTF)
	-- plus any custom gametypes added by WAD files via G_AddGametype
	-- or SOC Gametype blocks (e.g. Survival, or Arena/Team Arena
	-- from battle mod / SRB2 Kart).
	if G_GetGametypeName then
		local name = G_GetGametypeName(gt)
		if name and name ~= "" then return name end
	end
	-- Fallback table (base SRB2 constants only; GT_BATTLE/GT_TEAMBATTLE
	-- are from battle mod and may not exist in vanilla SRB2)
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
	-- Add WAD-defined constants safely (they won't exist in base SRB2)
	if GT_BATTLE then names[GT_BATTLE] = "Arena" end
	if GT_TEAMBATTLE then names[GT_TEAMBATTLE] = "Team Arena" end
	return names[gt] or "Unknown"
end

local function map_num_to_mapstr(n)
	if n < 100 then
		return string.format("MAP%02d", n)
	end
	return "MAP"..n
end

	addHook("MapLoad", function(map)
		local mapname = mapheaderinfo[map]
		local maptitle = "Unknown Map"
		if mapname and mapname.lvlttl
 then
			maptitle = mapname.lvlttl
			if mapname.actnum and mapname.actnum > 0
 then
				maptitle = maptitle.." Act "..mapname.actnum
			end
		end
		local gtname = get_gametype_name(gametype)
		local mapstr = map_num_to_mapstr(map)
		DiscordBot.Data.current_map = map
		if DiscordBot.Data.round_active == false
 then
			DiscordBot.Data.round_active = true
			local event_line = "[EVENT:ROUND_START]|"..gtname.."|"..mapstr.."|"..maptitle.."\n"
			DiscordBot.Data.msgsrb2 = DiscordBot.Data.msgsrb2..event_line
			DiscordBot.Functions.flush_msgsrb2()
		end
	end)

local ok, err = pcall(addHook, "IntermissionThinker", function(stagefailed)
	DiscordBot.Data.servertime = DiscordBot.Data.servertime + 1
	COM_BufInsertText(server, "server_log discord")
	COM_BufInsertText(server, "server_log console")
	DiscordBot.Functions.flush_msgsrb2()
	if not DiscordBot.Data.round_active then return end
	DiscordBot.Data.round_active = false

	local gtname = get_gametype_name(gametype)
	local mapstr = map_num_to_mapstr(gamemap)
	local mapname = mapheaderinfo[gamemap]
	local maptitle = "Unknown"
	if mapname and mapname.lvlttl
 then
		maptitle = mapname.lvlttl
		if mapname.actnum and mapname.actnum > 0
 then
			maptitle = maptitle.." Act "..mapname.actnum
		end
	end
	local event_line = "[EVENT:ROUND_END]|"..gtname.."|"..mapstr.."|MAPNAME:"..maptitle
	if gametype == GT_CTF or gametype == GT_TEAMMATCH or (GT_TEAMBATTLE and gametype == GT_TEAMBATTLE)
 then
		local reds = ""
		local blues = ""
		for player in players.iterate do
			if player and player.spectator != true then
				local pname = string.gsub(player.name, "|", "")
				if player.ctfteam == 1 then
					reds = reds.."|RED:"..pname..":"..player.score
				elseif player.ctfteam == 2
 					then
					blues = blues.."|BLUE:"..pname..":"..player.score
				else
					event_line = event_line.."|PLAYER:"..pname..":"..player.score
				end
			end
		end
		event_line = event_line.."|TEAM:Red:"..GetTeamScore(1)
		event_line = event_line..reds
		event_line = event_line.."|TEAM:Blue:"..GetTeamScore(2)
		event_line = event_line..blues
	end
	if gametype == GT_COMPETITION or gametype == GT_MATCH or (GT_BATTLE and gametype == GT_BATTLE) or gametype == GT_RACE then
		for player in players.iterate do
			if player and player.spectator != true
 then
				local pname = string.gsub(player.name, "|", "")
				event_line = event_line.."|PLAYER:"..pname..":"..player.score
			end
		end
	end
	local specs = ""
	for player in players.iterate do
		if player and player.spectator == true
 then
			local pname = string.gsub(player.name, "|", "")
			specs = specs.."|SPEC:"..pname
		end
	end
	event_line = event_line..specs
	event_line = event_line.."\n"
	DiscordBot.Data.msgsrb2 = DiscordBot.Data.msgsrb2..event_line
	DiscordBot.Functions.flush_msgsrb2()
end)
