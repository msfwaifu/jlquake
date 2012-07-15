/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


#include "server.h"

Cvar* sv_fps;					// time rate for running non-clients
Cvar* sv_rconPassword;			// password for remote server commands
Cvar* sv_privatePassword;		// password for the privateClient slots

Cvar* sv_privateClients;		// number of clients reserved for password
Cvar* sv_master[MAX_MASTER_SERVERS];		// master server ip address
Cvar* sv_reconnectlimit;		// minimum seconds between connect messages
Cvar* sv_tempbanmessage;
Cvar* sv_showloss;				// report when usercmds are lost
Cvar* sv_killserver;			// menu system can set to 1 to shut server down
Cvar* sv_mapChecksum;
Cvar* sv_serverid;
Cvar* sv_minPing;
Cvar* sv_maxPing;
Cvar* sv_floodProtect;
Cvar* sv_allowAnonymous;
Cvar* sv_onlyVisibleClients;	// DHM - Nerve
Cvar* sv_friendlyFire;			// NERVE - SMF
Cvar* sv_maxlives;				// NERVE - SMF
Cvar* sv_needpass;

// Rafael gameskill
//Cvar	*sv_gameskill;
// done

Cvar* sv_reloading;

Cvar* sv_showAverageBPS;		// NERVE - SMF - net debugging

//bani
Cvar* sv_cheats;
Cvar* sv_packetloss;
Cvar* sv_packetdelay;

// fretn
Cvar* sv_fullmsg;

void SVC_GameCompleteStatus(netadr_t from);			// NERVE - SMF

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

/*
==============================================================================

MASTER SERVER FUNCTIONS

==============================================================================
*/

/*
================
SV_MasterHeartbeat

Send a message to the masters every few minutes to
let it know we are alive, and log information.
We will also have a heartbeat sent when a server
changes from empty to non-empty, and full to non-full,
but not on every player enter or exit.
================
*/
#define HEARTBEAT_MSEC  300 * 1000
//#define	HEARTBEAT_GAME	"Wolfenstein-1"
//#define	HEARTBEAT_DEAD	"WolfFlatline-1"			// NERVE - SMF
#define HEARTBEAT_GAME  "EnemyTerritory-1"
#define HEARTBEAT_DEAD  "ETFlatline-1"			// NERVE - SMF

void SV_MasterHeartbeat(const char* hbname)
{
	static netadr_t adr[MAX_MASTER_SERVERS];
	int i;

	if (SVET_GameIsSinglePlayer())
	{
		return;		// no heartbeats for SP
	}

	// "dedicated 1" is for lan play, "dedicated 2" is for inet public play
	if (!com_dedicated || com_dedicated->integer != 2)
	{
		return;		// only dedicated servers send heartbeats
	}

	// if not time yet, don't send anything
	if (svs.q3_time < svs.q3_nextHeartbeatTime)
	{
		return;
	}
	svs.q3_nextHeartbeatTime = svs.q3_time + HEARTBEAT_MSEC;


	// send to group masters
	for (i = 0; i < MAX_MASTER_SERVERS; i++)
	{
		if (!sv_master[i]->string[0])
		{
			continue;
		}

		// see if we haven't already resolved the name
		// resolving usually causes hitches on win95, so only
		// do it when needed
		if (sv_master[i]->modified)
		{
			sv_master[i]->modified = qfalse;

			Com_Printf("Resolving %s\n", sv_master[i]->string);
			if (!SOCK_StringToAdr(sv_master[i]->string, &adr[i], PORT_MASTER))
			{
				// if the address failed to resolve, clear it
				// so we don't take repeated dns hits
				Com_Printf("Couldn't resolve address: %s\n", sv_master[i]->string);
				Cvar_Set(sv_master[i]->name, "");
				sv_master[i]->modified = qfalse;
				continue;
			}
			Com_Printf("%s resolved to %i.%i.%i.%i:%i\n", sv_master[i]->string,
				adr[i].ip[0], adr[i].ip[1], adr[i].ip[2], adr[i].ip[3],
				BigShort(adr[i].port));
		}


		Com_Printf("Sending heartbeat to %s\n", sv_master[i]->string);
		// this command should be changed if the server info / status format
		// ever incompatably changes
		NET_OutOfBandPrint(NS_SERVER, adr[i], "heartbeat %s\n", hbname);
	}
}

/*
=================
SV_MasterGameCompleteStatus

NERVE - SMF - Sends gameCompleteStatus messages to all master servers
=================
*/
void SV_MasterGameCompleteStatus()
{
	static netadr_t adr[MAX_MASTER_SERVERS];
	int i;

	if (SVET_GameIsSinglePlayer())
	{
		return;		// no master game status for SP
	}

	// "dedicated 1" is for lan play, "dedicated 2" is for inet public play
	if (!com_dedicated || com_dedicated->integer != 2)
	{
		return;		// only dedicated servers send master game status
	}

	// send to group masters
	for (i = 0; i < MAX_MASTER_SERVERS; i++)
	{
		if (!sv_master[i]->string[0])
		{
			continue;
		}

		// see if we haven't already resolved the name
		// resolving usually causes hitches on win95, so only
		// do it when needed
		if (sv_master[i]->modified)
		{
			sv_master[i]->modified = qfalse;

			Com_Printf("Resolving %s\n", sv_master[i]->string);
			if (!SOCK_StringToAdr(sv_master[i]->string, &adr[i], PORT_MASTER))
			{
				// if the address failed to resolve, clear it
				// so we don't take repeated dns hits
				Com_Printf("Couldn't resolve address: %s\n", sv_master[i]->string);
				Cvar_Set(sv_master[i]->name, "");
				sv_master[i]->modified = qfalse;
				continue;
			}
			Com_Printf("%s resolved to %i.%i.%i.%i:%i\n", sv_master[i]->string,
				adr[i].ip[0], adr[i].ip[1], adr[i].ip[2], adr[i].ip[3],
				BigShort(adr[i].port));
		}

		Com_Printf("Sending gameCompleteStatus to %s\n", sv_master[i]->string);
		// this command should be changed if the server info / status format
		// ever incompatably changes
		SVC_GameCompleteStatus(adr[i]);
	}
}

/*
=================
SV_MasterShutdown

Informs all masters that this server is going down
=================
*/
void SV_MasterShutdown(void)
{
	// send a hearbeat right now
	svs.q3_nextHeartbeatTime = -9999;
	SV_MasterHeartbeat(HEARTBEAT_DEAD);					// NERVE - SMF - changed to flatline

	// send it again to minimize chance of drops
//	svs.q3_nextHeartbeatTime = -9999;
//	SV_MasterHeartbeat( HEARTBEAT_DEAD );

	// when the master tries to poll the server, it won't respond, so
	// it will be removed from the list
}


/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

//bani - bugtraq 12534
//returns qtrue if valid challenge
//returns qfalse if m4d h4x0rz
qboolean SV_VerifyChallenge(char* challenge)
{
	int i, j;

	if (!challenge)
	{
		return qfalse;
	}

	j = String::Length(challenge);
	if (j > 64)
	{
		return qfalse;
	}
	for (i = 0; i < j; i++)
	{
		if (challenge[i] == '\\' ||
			challenge[i] == '/' ||
			challenge[i] == '%' ||
			challenge[i] == ';' ||
			challenge[i] == '"' ||
			challenge[i] < 32 ||	// non-ascii
			challenge[i] > 126	// non-ascii
			)
		{
			return qfalse;
		}
	}
	return qtrue;
}

/*
================
SVC_Status

Responds with all the info that qplug or qspy can see about the server
and all connected players.  Used for getting detailed information after
the simple info query.
================
*/
void SVC_Status(netadr_t from)
{
	char player[1024];
	char status[MAX_MSGLEN_WOLF];
	int i;
	client_t* cl;
	etplayerState_t* ps;
	int statusLength;
	int playerLength;
	char infostring[MAX_INFO_STRING_Q3];

	// ignore if we are in single player
	if (SVET_GameIsSinglePlayer())
	{
		return;
	}

	//bani - bugtraq 12534
	if (!SV_VerifyChallenge(Cmd_Argv(1)))
	{
		return;
	}

	String::Cpy(infostring, Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE, MAX_INFO_STRING_Q3));

	// echo back the parameter to status. so master servers can use it as a challenge
	// to prevent timed spoofed reply packets that add ghost servers
	Info_SetValueForKey(infostring, "challenge", Cmd_Argv(1), MAX_INFO_STRING_Q3);

	status[0] = 0;
	statusLength = 0;

	for (i = 0; i < sv_maxclients->integer; i++)
	{
		cl = &svs.clients[i];
		if (cl->state >= CS_CONNECTED)
		{
			ps = SVET_GameClientNum(i);
			String::Sprintf(player, sizeof(player), "%i %i \"%s\"\n",
				ps->persistant[ETPERS_SCORE], cl->ping, cl->name);
			playerLength = String::Length(player);
			if (statusLength + playerLength >= (int)sizeof(status))
			{
				break;		// can't hold any more
			}
			String::Cpy(status + statusLength, player);
			statusLength += playerLength;
		}
	}

	NET_OutOfBandPrint(NS_SERVER, from, "statusResponse\n%s\n%s", infostring, status);
}

/*
=================
SVC_GameCompleteStatus

NERVE - SMF - Send serverinfo cvars, etc to master servers when
game complete. Useful for tracking global player stats.
=================
*/
void SVC_GameCompleteStatus(netadr_t from)
{
	char player[1024];
	char status[MAX_MSGLEN_WOLF];
	int i;
	client_t* cl;
	etplayerState_t* ps;
	int statusLength;
	int playerLength;
	char infostring[MAX_INFO_STRING_Q3];

	// ignore if we are in single player
	if (SVET_GameIsSinglePlayer())
	{
		return;
	}

	//bani - bugtraq 12534
	if (!SV_VerifyChallenge(Cmd_Argv(1)))
	{
		return;
	}

	String::Cpy(infostring, Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE, MAX_INFO_STRING_Q3));

	// echo back the parameter to status. so master servers can use it as a challenge
	// to prevent timed spoofed reply packets that add ghost servers
	Info_SetValueForKey(infostring, "challenge", Cmd_Argv(1), MAX_INFO_STRING_Q3);

	status[0] = 0;
	statusLength = 0;

	for (i = 0; i < sv_maxclients->integer; i++)
	{
		cl = &svs.clients[i];
		if (cl->state >= CS_CONNECTED)
		{
			ps = SVET_GameClientNum(i);
			String::Sprintf(player, sizeof(player), "%i %i \"%s\"\n",
				ps->persistant[ETPERS_SCORE], cl->ping, cl->name);
			playerLength = String::Length(player);
			if (statusLength + playerLength >= (int)sizeof(status))
			{
				break;		// can't hold any more
			}
			String::Cpy(status + statusLength, player);
			statusLength += playerLength;
		}
	}

	NET_OutOfBandPrint(NS_SERVER, from, "gameCompleteStatus\n%s\n%s", infostring, status);
}

/*
================
SVC_Info

Responds with a short info message that should be enough to determine
if a user is interested in a server to do a full status
================
*/
void SVC_Info(netadr_t from)
{
	int i, count;
	const char* gamedir;
	char infostring[MAX_INFO_STRING_Q3];
	const char* antilag;
	const char* weaprestrict;
	const char* balancedteams;

	// ignore if we are in single player
	if (SVET_GameIsSinglePlayer())
	{
		return;
	}

	//bani - bugtraq 12534
	if (!SV_VerifyChallenge(Cmd_Argv(1)))
	{
		return;
	}

	// don't count privateclients
	count = 0;
	for (i = sv_privateClients->integer; i < sv_maxclients->integer; i++)
	{
		if (svs.clients[i].state >= CS_CONNECTED)
		{
			count++;
		}
	}

	infostring[0] = 0;

	// echo back the parameter to status. so servers can use it as a challenge
	// to prevent timed spoofed reply packets that add ghost servers
	Info_SetValueForKey(infostring, "challenge", Cmd_Argv(1), MAX_INFO_STRING_Q3);

	Info_SetValueForKey(infostring, "protocol", va("%i", PROTOCOL_VERSION), MAX_INFO_STRING_Q3);
	Info_SetValueForKey(infostring, "hostname", sv_hostname->string, MAX_INFO_STRING_Q3);
	Info_SetValueForKey(infostring, "serverload", va("%i", svs.et_serverLoad), MAX_INFO_STRING_Q3);
	Info_SetValueForKey(infostring, "mapname", svt3_mapname->string, MAX_INFO_STRING_Q3);
	Info_SetValueForKey(infostring, "clients", va("%i", count), MAX_INFO_STRING_Q3);
	Info_SetValueForKey(infostring, "sv_maxclients", va("%i", sv_maxclients->integer - sv_privateClients->integer), MAX_INFO_STRING_Q3);
	Info_SetValueForKey(infostring, "gametype", Cvar_VariableString("g_gametype"), MAX_INFO_STRING_Q3);
	Info_SetValueForKey(infostring, "pure", va("%i", svt3_pure->integer), MAX_INFO_STRING_Q3);

	if (sv_minPing->integer)
	{
		Info_SetValueForKey(infostring, "minPing", va("%i", sv_minPing->integer), MAX_INFO_STRING_Q3);
	}
	if (sv_maxPing->integer)
	{
		Info_SetValueForKey(infostring, "maxPing", va("%i", sv_maxPing->integer), MAX_INFO_STRING_Q3);
	}
	gamedir = Cvar_VariableString("fs_game");
	if (*gamedir)
	{
		Info_SetValueForKey(infostring, "game", gamedir, MAX_INFO_STRING_Q3);
	}
	Info_SetValueForKey(infostring, "sv_allowAnonymous", va("%i", sv_allowAnonymous->integer), MAX_INFO_STRING_Q3);

	// Rafael gameskill
//	Info_SetValueForKey (infostring, "gameskill", va ("%i", sv_gameskill->integer));
	// done

	Info_SetValueForKey(infostring, "friendlyFire", va("%i", sv_friendlyFire->integer), MAX_INFO_STRING_Q3);			// NERVE - SMF
	Info_SetValueForKey(infostring, "maxlives", va("%i", sv_maxlives->integer ? 1 : 0), MAX_INFO_STRING_Q3);			// NERVE - SMF
	Info_SetValueForKey(infostring, "needpass", va("%i", sv_needpass->integer ? 1 : 0), MAX_INFO_STRING_Q3);
	Info_SetValueForKey(infostring, "gamename", GAMENAME_STRING, MAX_INFO_STRING_Q3);									// Arnout: to be able to filter out Quake servers

	// TTimo
	antilag = Cvar_VariableString("g_antilag");
	if (antilag)
	{
		Info_SetValueForKey(infostring, "g_antilag", antilag, MAX_INFO_STRING_Q3);
	}

	weaprestrict = Cvar_VariableString("g_heavyWeaponRestriction");
	if (weaprestrict)
	{
		Info_SetValueForKey(infostring, "weaprestrict", weaprestrict, MAX_INFO_STRING_Q3);
	}

	balancedteams = Cvar_VariableString("g_balancedteams");
	if (balancedteams)
	{
		Info_SetValueForKey(infostring, "balancedteams", balancedteams, MAX_INFO_STRING_Q3);
	}

	NET_OutOfBandPrint(NS_SERVER, from, "infoResponse\n%s", infostring);
}

/*
==============
SV_FlushRedirect

==============
*/
void SV_FlushRedirect(char* outputbuf)
{
	NET_OutOfBandPrint(NS_SERVER, svs.q3_redirectAddress, "print\n%s", outputbuf);
}

/*
===============
SVC_RemoteCommand

An rcon packet arrived from the network.
Shift down the remaining args
Redirect all printfs
===============
*/
void SVC_RemoteCommand(netadr_t from, QMsg* msg)
{
	qboolean valid;
	unsigned int time;
	char remaining[1024];
	// show_bug.cgi?id=376
	// if we send an OOB print message this size, 1.31 clients die in a Com_Printf buffer overflow
	// the buffer overflow will be fixed in > 1.31 clients
	// but we want a server side fix
	// we must NEVER send an OOB message that will be > 1.31 MAXPRINTMSG (4096)
#define SV_OUTPUTBUF_LENGTH (256 - 16)
	char sv_outputbuf[SV_OUTPUTBUF_LENGTH];
	static unsigned int lasttime = 0;
	char* cmd_aux;

	// TTimo - show_bug.cgi?id=534
	time = Com_Milliseconds();
	if (time < (lasttime + 500))
	{
		return;
	}
	lasttime = time;

	if (!String::Length(sv_rconPassword->string) ||
		String::Cmp(Cmd_Argv(1), sv_rconPassword->string))
	{
		valid = qfalse;
		Com_Printf("Bad rcon from %s:\n%s\n", SOCK_AdrToString(from), Cmd_Argv(2));
	}
	else
	{
		valid = qtrue;
		Com_Printf("Rcon from %s:\n%s\n", SOCK_AdrToString(from), Cmd_Argv(2));
	}

	// start redirecting all print outputs to the packet
	svs.q3_redirectAddress = from;
	// FIXME TTimo our rcon redirection could be improved
	//   big rcon commands such as status lead to sending
	//   out of band packets on every single call to Com_Printf
	//   which leads to client overflows
	//   see show_bug.cgi?id=51
	//     (also a Q3 issue)
	Com_BeginRedirect(sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);

	if (!String::Length(sv_rconPassword->string))
	{
		Com_Printf("No rconpassword set on the server.\n");
	}
	else if (!valid)
	{
		Com_Printf("Bad rconpassword.\n");
	}
	else
	{
		remaining[0] = 0;

		// ATVI Wolfenstein Misc #284
		// get the command directly, "rcon <pass> <command>" to avoid quoting issues
		// extract the command by walking
		// since the cmd formatting can fuckup (amount of spaces), using a dumb step by step parsing
		cmd_aux = Cmd_Cmd();
		cmd_aux += 4;
		while (cmd_aux[0] == ' ')
			cmd_aux++;
		while (cmd_aux[0] && cmd_aux[0] != ' ')		// password
			cmd_aux++;
		while (cmd_aux[0] == ' ')
			cmd_aux++;

		String::Cat(remaining, sizeof(remaining), cmd_aux);

		Cmd_ExecuteString(remaining);

	}

	Com_EndRedirect();
}

/*
=================
SV_ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/
void SV_ConnectionlessPacket(netadr_t from, QMsg* msg)
{
	const char* s;
	char* c;

	msg->BeginReadingOOB();
	msg->ReadLong();		// skip the -1 marker

	if (!String::NCmp("connect", (char*)&msg->_data[4], 7))
	{
		Huff_Decompress(msg, 12);
	}

	s = msg->ReadStringLine();

	Cmd_TokenizeString(s);

	c = Cmd_Argv(0);
	Com_DPrintf("SV packet %s : %s\n", SOCK_AdrToString(from), c);

	if (!String::ICmp(c,"getstatus"))
	{
		SVC_Status(from);
	}
	else if (!String::ICmp(c,"getinfo"))
	{
		SVC_Info(from);
	}
	else if (!String::ICmp(c,"getchallenge"))
	{
		SV_GetChallenge(from);
	}
	else if (!String::ICmp(c,"connect"))
	{
		SV_DirectConnect(from);
#ifdef AUTHORIZE_SUPPORT
	}
	else if (!String::ICmp(c,"ipAuthorize"))
	{
		SV_AuthorizeIpPacket(from);
#endif	// AUTHORIZE_SUPPORT
	}
	else if (!String::ICmp(c, "rcon"))
	{
		SVC_RemoteCommand(from, msg);
	}
	else if (!String::ICmp(c,"disconnect"))
	{
		// if a client starts up a local server, we may see some spurious
		// server disconnect messages when their new server sees our final
		// sequenced messages to the old client
	}
	else
	{
		Com_DPrintf("bad connectionless packet from %s:\n%s\n",
			SOCK_AdrToString(from), s);
	}
}

//============================================================================

/*
=================
SV_ReadPackets
=================
*/
void SV_PacketEvent(netadr_t from, QMsg* msg)
{
	int i;
	client_t* cl;
	int qport;

	// check for connectionless packet (0xffffffff) first
	if (msg->cursize >= 4 && *(int*)msg->_data == -1)
	{
		SV_ConnectionlessPacket(from, msg);
		return;
	}

	// read the qport out of the message so we can fix up
	// stupid address translating routers
	msg->BeginReadingOOB();
	msg->ReadLong();				// sequence number
	qport = msg->ReadShort() & 0xffff;

	// find which client the message is from
	for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++,cl++)
	{
		if (cl->state == CS_FREE)
		{
			continue;
		}
		if (!SOCK_CompareBaseAdr(from, cl->netchan.remoteAddress))
		{
			continue;
		}
		// it is possible to have multiple clients from a single IP
		// address, so they are differentiated by the qport variable
		if (cl->netchan.qport != qport)
		{
			continue;
		}

		// the IP port can't be used to differentiate them, because
		// some address translating routers periodically change UDP
		// port assignments
		if (cl->netchan.remoteAddress.port != from.port)
		{
			Com_Printf("SV_PacketEvent: fixing up a translated port\n");
			cl->netchan.remoteAddress.port = from.port;
		}

		// make sure it is a valid, in sequence packet
		if (SV_Netchan_Process(cl, msg))
		{
			// zombie clients still need to do the Netchan_Process
			// to make sure they don't need to retransmit the final
			// reliable message, but they don't do any other processing
			if (cl->state != CS_ZOMBIE)
			{
				cl->q3_lastPacketTime = svs.q3_time;	// don't timeout
				SV_ExecuteClientMessage(cl, msg);
			}
		}
		return;
	}

	// if we received a sequenced packet from an address we don't recognize,
	// send an out of band disconnect packet to it
	NET_OutOfBandPrint(NS_SERVER, from, "disconnect");
}

/*
==================
SV_Frame

Player movement occurs as a result of packet events, which
happen before SV_Frame is called
==================
*/
void SV_Frame(int msec)
{
	int frameMsec;
	int startTime;
	char mapname[MAX_QPATH];
	int frameStartTime = 0, frameEndTime;

	// the menu kills the server with this cvar
	if (sv_killserver->integer)
	{
		SV_Shutdown("Server was killed.\n");
		Cvar_Set("sv_killserver", "0");
		return;
	}

	if (!com_sv_running->integer)
	{
		return;
	}

	// allow pause if only the local client is connected
	if (SVT3_CheckPaused())
	{
		return;
	}

	if (com_dedicated->integer)
	{
		frameStartTime = Sys_Milliseconds();
	}

	// if it isn't time for the next frame, do nothing
	if (sv_fps->integer < 1)
	{
		Cvar_Set("sv_fps", "10");
	}
	frameMsec = 1000 / sv_fps->integer;

	sv.q3_timeResidual += msec;

	if (!com_dedicated->integer)
	{
		SVT3_BotFrame(svs.q3_time + sv.q3_timeResidual);
	}

	if (com_dedicated->integer && sv.q3_timeResidual < frameMsec)
	{
		// NET_Sleep will give the OS time slices until either get a packet
		// or time enough for a server frame has gone by
		NET_Sleep(frameMsec - sv.q3_timeResidual);
		return;
	}

	// if time is about to hit the 32nd bit, kick all clients
	// and clear sv.time, rather
	// than checking for negative time wraparound everywhere.
	// 2giga-milliseconds = 23 days, so it won't be too often
	if (svs.q3_time > 0x70000000)
	{
		String::NCpyZ(mapname, svt3_mapname->string, MAX_QPATH);
		SV_Shutdown("Restarting server due to time wrapping");
		// TTimo
		// show_bug.cgi?id=388
		// there won't be a map_restart if you have shut down the server
		// since it doesn't restart a non-running server
		// instead, re-run the current map
		Cbuf_AddText(va("map %s\n", mapname));
		return;
	}
	// this can happen considerably earlier when lots of clients play and the map doesn't change
	if (svs.q3_nextSnapshotEntities >= 0x7FFFFFFE - svs.q3_numSnapshotEntities)
	{
		String::NCpyZ(mapname, svt3_mapname->string, MAX_QPATH);
		SV_Shutdown("Restarting server due to numSnapshotEntities wrapping");
		// TTimo see above
		Cbuf_AddText(va("map %s\n", mapname));
		return;
	}

	if (sv.q3_restartTime && svs.q3_time >= sv.q3_restartTime)
	{
		sv.q3_restartTime = 0;
		Cbuf_AddText("map_restart 0\n");
		return;
	}

	// update infostrings if anything has been changed
	if (cvar_modifiedFlags & CVAR_SERVERINFO)
	{
		SVT3_SetConfigstring(Q3CS_SERVERINFO, Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE, MAX_INFO_STRING_Q3));
		cvar_modifiedFlags &= ~CVAR_SERVERINFO;
	}
	if (cvar_modifiedFlags & CVAR_SERVERINFO_NOUPDATE)
	{
		SVT3_SetConfigstringNoUpdate(Q3CS_SERVERINFO, Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE, MAX_INFO_STRING_Q3));
		cvar_modifiedFlags &= ~CVAR_SERVERINFO_NOUPDATE;
	}
	if (cvar_modifiedFlags & CVAR_SYSTEMINFO)
	{
		SVT3_SetConfigstring(Q3CS_SYSTEMINFO, Cvar_InfoString(CVAR_SYSTEMINFO, BIG_INFO_STRING));
		cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
	}
	// NERVE - SMF
	if (cvar_modifiedFlags & CVAR_WOLFINFO)
	{
		SVT3_SetConfigstring(ETCS_WOLFINFO, Cvar_InfoString(CVAR_WOLFINFO, MAX_INFO_STRING_Q3));
		cvar_modifiedFlags &= ~CVAR_WOLFINFO;
	}

	if (com_speeds->integer)
	{
		startTime = Sys_Milliseconds();
	}
	else
	{
		startTime = 0;	// quite a compiler warning
	}

	// update ping based on the all received frames
	SVT3_CalcPings();

	if (com_dedicated->integer)
	{
		SVT3_BotFrame(svs.q3_time);
	}

	// run the game simulation in chunks
	while (sv.q3_timeResidual >= frameMsec)
	{
		sv.q3_timeResidual -= frameMsec;
		svs.q3_time += frameMsec;

		// let everything in the world think and move
		VM_Call(gvm, ETGAME_RUN_FRAME, svs.q3_time);
	}

	if (com_speeds->integer)
	{
		time_game = Sys_Milliseconds() - startTime;
	}

	// check timeouts
	SVT3_CheckTimeouts();

	// send messages back to the clients
	SV_SendClientMessages();

	// send a heartbeat to the master if needed
	SV_MasterHeartbeat(HEARTBEAT_GAME);

	if (com_dedicated->integer)
	{
		frameEndTime = Sys_Milliseconds();

		svs.et_totalFrameTime += (frameEndTime - frameStartTime);
		svs.et_currentFrameIndex++;

		//if( svs.et_currentFrameIndex % 50 == 0 )
		//	Com_Printf( "currentFrameIndex: %i\n", svs.et_currentFrameIndex );

		if (svs.et_currentFrameIndex == SERVER_PERFORMANCECOUNTER_FRAMES)
		{
			int averageFrameTime;

			averageFrameTime = svs.et_totalFrameTime / SERVER_PERFORMANCECOUNTER_FRAMES;

			svs.et_sampleTimes[svs.et_currentSampleIndex % SERVER_PERFORMANCECOUNTER_SAMPLES] = averageFrameTime;
			svs.et_currentSampleIndex++;

			if (svs.et_currentSampleIndex > SERVER_PERFORMANCECOUNTER_SAMPLES)
			{
				int totalTime, i;

				totalTime = 0;
				for (i = 0; i < SERVER_PERFORMANCECOUNTER_SAMPLES; i++)
				{
					totalTime += svs.et_sampleTimes[i];
				}

				if (!totalTime)
				{
					totalTime = 1;
				}

				averageFrameTime = totalTime / SERVER_PERFORMANCECOUNTER_SAMPLES;

				svs.et_serverLoad = (averageFrameTime / (float)frameMsec) * 100;
			}

			//Com_Printf( "serverload: %i (%i/%i)\n", svs.et_serverLoad, averageFrameTime, frameMsec );

			svs.et_totalFrameTime = 0;
			svs.et_currentFrameIndex = 0;
		}
	}
	else
	{
		svs.et_serverLoad = -1;
	}
}

//============================================================================

bool CL_IsServerActive()
{
	return sv.state == SS_GAME;
}
