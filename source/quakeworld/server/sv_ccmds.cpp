/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "qwsvdef.h"

qboolean sv_allow_cheats;

extern Cvar* cl_warncmd;
extern redirect_t sv_redirected;


/*
===============================================================================

OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

/*
====================
SV_SetMaster_f

Make a master server current
====================
*/
void SV_SetMaster_f(void)
{
	char data[2];
	int i;

	Com_Memset(&master_adr, 0, sizeof(master_adr));

	for (i = 1; i < Cmd_Argc(); i++)
	{
		if (!String::Cmp(Cmd_Argv(i), "none") || !SOCK_StringToAdr(Cmd_Argv(i), &master_adr[i - 1], 27000))
		{
			common->Printf("Setting nomaster mode.\n");
			return;
		}

		common->Printf("Master server at %s\n", SOCK_AdrToString(master_adr[i - 1]));

		common->Printf("Sending a ping.\n");

		data[0] = A2A_PING;
		data[1] = 0;
		NET_SendPacket(NS_SERVER, 2, data, master_adr[i - 1]);
	}

	svs.qh_last_heartbeat = -99999;
}


/*
==================
SV_Quit_f
==================
*/
void SV_Quit_f(void)
{
	SV_FinalMessage("server shutdown\n");
	common->Printf("Shutting down.\n");
	SV_Shutdown();
	Sys_Quit();
}

/*
============
SV_Logfile_f
============
*/
void SV_Logfile_f(void)
{
	char name[MAX_OSPATH];

	if (sv_logfile)
	{
		common->Printf("File logging off.\n");
		FS_FCloseFile(sv_logfile);
		sv_logfile = 0;
		return;
	}

	sprintf(name, "qconsole.log");
	common->Printf("Logging text to %s.\n", name);
	sv_logfile = FS_FOpenFileWrite(name);
	if (!sv_logfile)
	{
		common->Printf("failed.\n");
	}
}


/*
============
SV_Fraglogfile_f
============
*/
void SV_Fraglogfile_f(void)
{
	char name[MAX_OSPATH];
	int i;

	if (svqhw_fraglogfile)
	{
		common->Printf("Frag file logging off.\n");
		FS_FCloseFile(svqhw_fraglogfile);
		svqhw_fraglogfile = 0;
		return;
	}

	// find an unused name
	for (i = 0; i < 1000; i++)
	{
		sprintf(name, "frag_%i.log", i);
		if (!FS_FileExists(name))
		{
			// can't read it, so create this one
			svqhw_fraglogfile = FS_FOpenFileWrite(name);
			if (!svqhw_fraglogfile)
			{
				i = 1000;	// give error
			}
			break;
		}
	}
	if (i == 1000)
	{
		common->Printf("Can't open any logfiles.\n");
		svqhw_fraglogfile = 0;
		return;
	}

	common->Printf("Logging frags to %s.\n", name);
}


/*
==================
SV_SetPlayer

Sets host_client and sv_player to the player with idnum Cmd_Argv(1)
==================
*/
qboolean SV_SetPlayer(void)
{
	client_t* cl;
	int i;
	int idnum;

	idnum = String::Atoi(Cmd_Argv(1));

	for (i = 0,cl = svs.clients; i < MAX_CLIENTS_QHW; i++,cl++)
	{
		if (!cl->state)
		{
			continue;
		}
		if (cl->qh_userid == idnum)
		{
			host_client = cl;
			sv_player = host_client->qh_edict;
			return true;
		}
	}
	common->Printf("Userid %i is not on the server\n", idnum);
	return false;
}


/*
==================
SV_God_f

Sets client to godmode
==================
*/
void SV_God_f(void)
{
	if (!sv_allow_cheats)
	{
		common->Printf("You must run the server with -cheats to enable this command.\n");
		return;
	}

	if (!SV_SetPlayer())
	{
		return;
	}

	sv_player->SetFlags((int)sv_player->GetFlags() ^ QHFL_GODMODE);
	if (!((int)sv_player->GetFlags() & QHFL_GODMODE))
	{
		SVQH_ClientPrintf(host_client, PRINT_HIGH, "godmode OFF\n");
	}
	else
	{
		SVQH_ClientPrintf(host_client, PRINT_HIGH, "godmode ON\n");
	}
}


void SV_Noclip_f(void)
{
	if (!sv_allow_cheats)
	{
		common->Printf("You must run the server with -cheats to enable this command.\n");
		return;
	}

	if (!SV_SetPlayer())
	{
		return;
	}

	if (sv_player->GetMoveType() != QHMOVETYPE_NOCLIP)
	{
		sv_player->SetMoveType(QHMOVETYPE_NOCLIP);
		SVQH_ClientPrintf(host_client, PRINT_HIGH, "noclip ON\n");
	}
	else
	{
		sv_player->SetMoveType(QHMOVETYPE_WALK);
		SVQH_ClientPrintf(host_client, PRINT_HIGH, "noclip OFF\n");
	}
}


/*
==================
SV_Give_f
==================
*/
void SV_Give_f(void)
{
	char* t;
	int v;

	if (!sv_allow_cheats)
	{
		common->Printf("You must run the server with -cheats to enable this command.\n");
		return;
	}

	if (!SV_SetPlayer())
	{
		return;
	}

	t = Cmd_Argv(2);
	v = String::Atoi(Cmd_Argv(3));

	switch (t[0])
	{
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		sv_player->SetItems((int)sv_player->GetItems() | IT_SHOTGUN << (t[0] - '2'));
		break;

	case 's':
		sv_player->SetAmmoShells(v);
		break;
	case 'n':
		sv_player->SetAmmoNails(v);
		break;
	case 'r':
		sv_player->SetAmmoRockets(v);
		break;
	case 'h':
		sv_player->SetHealth(v);
		break;
	case 'c':
		sv_player->SetAmmoCells(v);
		break;
	}
}


/*
======================
SV_Map_f

handle a
map <mapname>
command from the console or progs.
======================
*/
void SV_Map_f(void)
{
	char level[MAX_QPATH];
	char expanded[MAX_QPATH];
	fileHandle_t f;

	if (Cmd_Argc() != 2)
	{
		common->Printf("map <levelname> : continue game on a new level\n");
		return;
	}
	String::Cpy(level, Cmd_Argv(1));

#if 0
	if (!String::Cmp(level, "e1m8"))
	{	// QuakeWorld can't go to e1m8
		SVQH_BroadcastPrintf(PRINT_HIGH, "can't go to low grav level in QuakeWorld...\n");
		String::Cpy(level, "e1m5");
	}
#endif

	// check to make sure the level exists
	sprintf(expanded, "maps/%s.bsp", level);
	FS_FOpenFileRead(expanded, &f, true);
	if (!f)
	{
		common->Printf("Can't find %s\n", expanded);
		return;
	}
	FS_FCloseFile(f);

	SVQH_BroadcastCommand("changing\n");
	SV_SendMessagesToAll();

	SV_SpawnServer(level);

	SVQH_BroadcastCommand("reconnect\n");
}


/*
==================
SV_Kick_f

Kick a user off of the server
==================
*/
void SV_Kick_f(void)
{
	int i;
	client_t* cl;
	int uid;

	uid = String::Atoi(Cmd_Argv(1));

	for (i = 0, cl = svs.clients; i < MAX_CLIENTS_QHW; i++, cl++)
	{
		if (!cl->state)
		{
			continue;
		}
		if (cl->qh_userid == uid)
		{
			SVQH_BroadcastPrintf(PRINT_HIGH, "%s was kicked\n", cl->name);
			// print directly, because the dropped client won't get the
			// SVQH_BroadcastPrintf message
			SVQH_ClientPrintf(cl, PRINT_HIGH, "You were kicked from the game\n");
			SVQHW_DropClient(cl);
			return;
		}
	}

	common->Printf("Couldn't find user number %i\n", uid);
}


/*
================
SV_Status_f
================
*/
void SV_Status_f(void)
{
	int i, j, l;
	client_t* cl;
	float cpu, avg, pak;
	const char* s;


	cpu = (svs.qh_stats.latched_active + svs.qh_stats.latched_idle);
	if (cpu)
	{
		cpu = 100 * svs.qh_stats.latched_active / cpu;
	}
	avg = 1000 * svs.qh_stats.latched_active / STATFRAMES;
	pak = (float)svs.qh_stats.latched_packets / STATFRAMES;

	SOCK_ShowIP();
	common->Printf("port             : %d\n", sv_net_port);
	common->Printf("cpu utilization  : %3i%%\n",(int)cpu);
	common->Printf("avg response time: %i ms\n",(int)avg);
	common->Printf("packets/frame    : %5.2f\n", pak);

// min fps lat drp
	if (sv_redirected != RD_NONE)
	{
		// most remote clients are 40 columns
		//           0123456789012345678901234567890123456789
		common->Printf("name               userid frags\n");
		common->Printf("  address          ping drop\n");
		common->Printf("  ---------------- ---- -----\n");
		for (i = 0,cl = svs.clients; i < MAX_CLIENTS_QHW; i++,cl++)
		{
			if (!cl->state)
			{
				continue;
			}

			common->Printf("%-16.16s  ", cl->name);

			common->Printf("%6i %5i", cl->qh_userid, (int)cl->qh_edict->GetFrags());
			if (cl->qh_spectator)
			{
				common->Printf(" (s)\n");
			}
			else
			{
				common->Printf("\n");
			}

			s = SOCK_BaseAdrToString(cl->netchan.remoteAddress);
			common->Printf("  %-16.16s", s);
			if (cl->state == CS_CONNECTED)
			{
				common->Printf("CONNECTING\n");
				continue;
			}
			if (cl->state == CS_ZOMBIE)
			{
				common->Printf("ZOMBIE\n");
				continue;
			}
			common->Printf("%4i %5.2f\n",
				(int)SVQH_CalcPing(cl),
				100.0 * cl->netchan.dropCount / cl->netchan.incomingSequence);
		}
	}
	else
	{
		common->Printf("frags userid address         name            ping drop  qport\n");
		common->Printf("----- ------ --------------- --------------- ---- ----- -----\n");
		for (i = 0,cl = svs.clients; i < MAX_CLIENTS_QHW; i++,cl++)
		{
			if (!cl->state)
			{
				continue;
			}
			common->Printf("%5i %6i ", (int)cl->qh_edict->GetFrags(),  cl->qh_userid);

			s = SOCK_BaseAdrToString(cl->netchan.remoteAddress);
			common->Printf("%s", s);
			l = 16 - String::Length(s);
			for (j = 0; j < l; j++)
				common->Printf(" ");

			common->Printf("%s", cl->name);
			l = 16 - String::Length(cl->name);
			for (j = 0; j < l; j++)
				common->Printf(" ");
			if (cl->state == CS_CONNECTED)
			{
				common->Printf("CONNECTING\n");
				continue;
			}
			if (cl->state == CS_ZOMBIE)
			{
				common->Printf("ZOMBIE\n");
				continue;
			}
			common->Printf("%4i %3.1f %4i",
				(int)SVQH_CalcPing(cl),
				100.0 * cl->netchan.dropCount / cl->netchan.incomingSequence,
				cl->netchan.qport);
			if (cl->qh_spectator)
			{
				common->Printf(" (s)\n");
			}
			else
			{
				common->Printf("\n");
			}


		}
	}
	common->Printf("\n");
}

/*
==================
SV_ConSay_f
==================
*/
void SV_ConSay_f(void)
{
	client_t* client;
	int j;
	char* p;
	char text[1024];

	if (Cmd_Argc() < 2)
	{
		return;
	}

	String::Cpy(text, "console: ");
	p = Cmd_ArgsUnmodified();

	if (*p == '"')
	{
		p++;
		p[String::Length(p) - 1] = 0;
	}

	String::Cat(text, sizeof(text), p);

	for (j = 0, client = svs.clients; j < MAX_CLIENTS_QHW; j++, client++)
	{
		if (client->state != CS_ACTIVE)
		{
			continue;
		}
		SVQH_ClientPrintf(client, PRINT_CHAT, "%s\n", text);
	}
}


/*
==================
SV_Heartbeat_f
==================
*/
void SV_Heartbeat_f(void)
{
	svs.qh_last_heartbeat = -9999;
}

void SV_SendServerInfoChange(char* key, char* value)
{
	if (!sv.state)
	{
		return;
	}

	sv.qh_reliable_datagram.WriteByte(qwsvc_serverinfo);
	sv.qh_reliable_datagram.WriteString2(key);
	sv.qh_reliable_datagram.WriteString2(value);
}

/*
===========
SV_Serverinfo_f

  Examine or change the serverinfo string
===========
*/
void SV_Serverinfo_f(void)
{
	if (Cmd_Argc() == 1)
	{
		common->Printf("Server info settings:\n");
		Info_Print(svs.qh_info);
		return;
	}

	if (Cmd_Argc() != 3)
	{
		common->Printf("usage: serverinfo [ <key> <value> ]\n");
		return;
	}

	if (Cmd_Argv(1)[0] == '*')
	{
		common->Printf("Star variables cannot be changed.\n");
		return;
	}
	Info_SetValueForKey(svs.qh_info, Cmd_Argv(1), Cmd_Argv(2), MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value, false);

	// if this is a cvar, change it too
	Cvar_UpdateIfExists(Cmd_Argv(1), Cmd_Argv(2));

	SV_SendServerInfoChange(Cmd_Argv(1), Cmd_Argv(2));
}


/*
===========
SV_Serverinfo_f

  Examine or change the serverinfo string
===========
*/
void SV_Localinfo_f(void)
{
	if (Cmd_Argc() == 1)
	{
		common->Printf("Local info settings:\n");
		Info_Print(qhw_localinfo);
		return;
	}

	if (Cmd_Argc() != 3)
	{
		common->Printf("usage: localinfo [ <key> <value> ]\n");
		return;
	}

	if (Cmd_Argv(1)[0] == '*')
	{
		common->Printf("Star variables cannot be changed.\n");
		return;
	}
	Info_SetValueForKey(qhw_localinfo, Cmd_Argv(1), Cmd_Argv(2), QHMAX_LOCALINFO_STRING, 64, 64, !svqh_highchars->value, false);
}


/*
===========
SV_User_f

Examine a users info strings
===========
*/
void SV_User_f(void)
{
	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: info <userid>\n");
		return;
	}

	if (!SV_SetPlayer())
	{
		return;
	}

	Info_Print(host_client->userinfo);
}

/*
================
SV_Gamedir

Sets the fake *gamedir to a different directory.
================
*/
void SV_Gamedir(void)
{
	char* dir;

	if (Cmd_Argc() == 1)
	{
		common->Printf("Current *gamedir: %s\n", Info_ValueForKey(svs.qh_info, "*gamedir"));
		return;
	}

	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: sv_gamedir <newgamedir>\n");
		return;
	}

	dir = Cmd_Argv(1);

	if (strstr(dir, "..") || strstr(dir, "/") ||
		strstr(dir, "\\") || strstr(dir, ":"))
	{
		common->Printf("*Gamedir should be a single filename, not a path\n");
		return;
	}

	Info_SetValueForKey(svs.qh_info, "*gamedir", dir, MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value);
}

/*
================
SV_Floodport_f

Sets the gamedir and path to a different directory.
================
*/

void SV_Floodprot_f(void)
{
	int arg1, arg2, arg3;

	if (Cmd_Argc() == 1)
	{
		if (qhw_fp_messages)
		{
			common->Printf("Current floodprot settings: \nAfter %d msgs per %d seconds, silence for %d seconds\n",
				qhw_fp_messages, qhw_fp_persecond, qhw_fp_secondsdead);
			return;
		}
		else
		{
			common->Printf("No floodprots enabled.\n");
		}
	}

	if (Cmd_Argc() != 4)
	{
		common->Printf("Usage: floodprot <# of messages> <per # of seconds> <seconds to silence>\n");
		common->Printf("Use floodprotmsg to set a custom message to say to the flooder.\n");
		return;
	}

	arg1 = String::Atoi(Cmd_Argv(1));
	arg2 = String::Atoi(Cmd_Argv(2));
	arg3 = String::Atoi(Cmd_Argv(3));

	if (arg1 <= 0 || arg2 <= 0 || arg3 <= 0)
	{
		common->Printf("All values must be positive numbers\n");
		return;
	}

	if (arg1 > 10)
	{
		common->Printf("Can only track up to 10 messages.\n");
		return;
	}

	qhw_fp_messages = arg1;
	qhw_fp_persecond = arg2;
	qhw_fp_secondsdead = arg3;
}

void SV_Floodprotmsg_f(void)
{
	if (Cmd_Argc() == 1)
	{
		common->Printf("Current msg: %s\n", qhw_fp_msg);
		return;
	}
	else if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: floodprotmsg \"<message>\"\n");
		return;
	}
	sprintf(qhw_fp_msg, "%s", Cmd_Argv(1));
}

/*
================
SV_Gamedir_f

Sets the gamedir and path to a different directory.
================
*/
extern char gamedirfile[MAX_OSPATH];
void SV_Gamedir_f(void)
{
	char* dir;

	if (Cmd_Argc() == 1)
	{
		common->Printf("Current gamedir: %s\n", fs_gamedir);
		return;
	}

	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: gamedir <newdir>\n");
		return;
	}

	dir = Cmd_Argv(1);

	if (strstr(dir, "..") || strstr(dir, "/") ||
		strstr(dir, "\\") || strstr(dir, ":"))
	{
		common->Printf("Gamedir should be a single filename, not a path\n");
		return;
	}

	COM_Gamedir(dir);
	Info_SetValueForKey(svs.qh_info, "*gamedir", dir, MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value);
}

/*
================
SV_Snap
================
*/
void SV_Snap(int uid)
{
	client_t* cl;
	char pcxname[80];
	char checkname[MAX_OSPATH];
	int i;

	for (i = 0, cl = svs.clients; i < MAX_CLIENTS_QHW; i++, cl++)
	{
		if (!cl->state)
		{
			continue;
		}
		if (cl->qh_userid == uid)
		{
			break;
		}
	}
	if (i >= MAX_CLIENTS_QHW)
	{
		common->Printf("userid not found\n");
		return;
	}

	sprintf(pcxname, "%d-00.pcx", uid);

	for (i = 0; i <= 99; i++)
	{
		pcxname[String::Length(pcxname) - 6] = i / 10 + '0';
		pcxname[String::Length(pcxname) - 5] = i % 10 + '0';
		sprintf(checkname, "snap/%s", pcxname);
		if (!FS_FileExists(checkname))
		{
			break;	// file doesn't exist
		}
	}
	if (i == 100)
	{
		common->Printf("Snap: Couldn't create a file, clean some out.\n");
		return;
	}
	sprintf(checkname, "snap/%s", pcxname);
	String::Cpy(cl->qw_uploadfn, checkname);

	Com_Memcpy(&cl->qw_snap_from, &net_from, sizeof(net_from));
	if (sv_redirected != RD_NONE)
	{
		cl->qw_remote_snap = true;
	}
	else
	{
		cl->qw_remote_snap = false;
	}

	SVQH_SendClientCommand(cl, "cmd snap");
	common->Printf("Requesting snap from user %d...\n", uid);
}

/*
================
SV_Snap_f
================
*/
void SV_Snap_f(void)
{
	int uid;

	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage:  snap <userid>\n");
		return;
	}

	uid = String::Atoi(Cmd_Argv(1));

	SV_Snap(uid);
}

/*
================
SV_Snap
================
*/
void SV_SnapAll_f(void)
{
	client_t* cl;
	int i;

	for (i = 0, cl = svs.clients; i < MAX_CLIENTS_QHW; i++, cl++)
	{
		if (cl->state < CS_CONNECTED || cl->qh_spectator)
		{
			continue;
		}
		SV_Snap(cl->qh_userid);
	}
}

/*
==================
SV_InitOperatorCommands
==================
*/
void SV_InitOperatorCommands(void)
{
	if (COM_CheckParm("-cheats"))
	{
		sv_allow_cheats = true;
		Info_SetValueForKey(svs.qh_info, "*cheats", "ON", MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value);
	}

	Cmd_AddCommand("logfile", SV_Logfile_f);
	Cmd_AddCommand("fraglogfile", SV_Fraglogfile_f);

	Cmd_AddCommand("snap", SV_Snap_f);
	Cmd_AddCommand("snapall", SV_SnapAll_f);
	Cmd_AddCommand("kick", SV_Kick_f);
	Cmd_AddCommand("status", SV_Status_f);

	Cmd_AddCommand("map", SV_Map_f);
	Cmd_AddCommand("setmaster", SV_SetMaster_f);

	Cmd_AddCommand("say", SV_ConSay_f);
	Cmd_AddCommand("heartbeat", SV_Heartbeat_f);
	Cmd_AddCommand("quit", SV_Quit_f);
	Cmd_AddCommand("god", SV_God_f);
	Cmd_AddCommand("give", SV_Give_f);
	Cmd_AddCommand("noclip", SV_Noclip_f);
	Cmd_AddCommand("serverinfo", SV_Serverinfo_f);
	Cmd_AddCommand("localinfo", SV_Localinfo_f);
	Cmd_AddCommand("user", SV_User_f);
	Cmd_AddCommand("gamedir", SV_Gamedir_f);
	Cmd_AddCommand("sv_gamedir", SV_Gamedir);
	Cmd_AddCommand("floodprot", SV_Floodprot_f);
	Cmd_AddCommand("floodprotmsg", SV_Floodprotmsg_f);

	cl_warncmd->value = 1;
}
