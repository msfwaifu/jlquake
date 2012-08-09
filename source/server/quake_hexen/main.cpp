//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../server.h"
#include "../progsvm/progsvm.h"
#include "local.h"
#include "../../common/hexen2strings.h"

Cvar* svqh_deathmatch;			// 0, 1, or 2
Cvar* svqh_coop;				// 0 or 1
Cvar* svqh_teamplay;
Cvar* qh_timelimit;
Cvar* qh_fraglimit;
Cvar* qh_skill;					// 0 - 3
Cvar* h2_randomclass;				// 0, 1, or 2

Cvar* svqh_highchars;
Cvar* hw_spartanPrint;
Cvar* hw_dmMode;
Cvar* svqh_idealpitchscale;
Cvar* svqw_mapcheck;
Cvar* svhw_allowtaunts;
Cvar* svqhw_spectalk;
Cvar* qh_pausable;
Cvar* svhw_namedistance;
Cvar* svqhw_maxspectators;

Cvar* svh2_update_player;
Cvar* svh2_update_monsters;
Cvar* svh2_update_missiles;
Cvar* svh2_update_misc;

Cvar* qhw_allow_download;
Cvar* qhw_allow_download_skins;
Cvar* qhw_allow_download_models;
Cvar* qhw_allow_download_sounds;
Cvar* qhw_allow_download_maps;

Cvar* hw_damageScale;
Cvar* hw_shyRespawn;
Cvar* hw_meleeDamScale;
Cvar* hw_manaScale;
Cvar* hw_tomeMode;
Cvar* hw_tomeRespawn;
Cvar* hw_w2Respawn;
Cvar* hw_altRespawn;
Cvar* hw_fixedLevel;
Cvar* hw_autoItems;
Cvar* hw_easyFourth;
Cvar* hw_patternRunner;

Cvar* svqhw_rcon_password;
Cvar* svqhw_password;
Cvar* svqhw_spectator_password;

int svqh_current_skill;
int svh2_kingofhill;

fileHandle_t svqhw_fraglogfile;

void SVQH_FreeMemory()
{
	if (sv.h2_states)
	{
		Mem_Free(sv.h2_states);
		sv.h2_states = NULL;
	}
	if (sv.qh_edicts)
	{
		Mem_Free(sv.qh_edicts);
		sv.qh_edicts = NULL;
	}
	PR_UnloadProgs();
}

int SVQH_CalcPing(client_t* cl)
{
	float ping = 0;
	int count = 0;
	if (GGameType & GAME_HexenWorld)
	{
		hwclient_frame_t* frame = cl->hw_frames;
		for (int i = 0; i < UPDATE_BACKUP_HW; i++, frame++)
		{
			if (frame->ping_time > 0)
			{
				ping += frame->ping_time;
				count++;
			}
		}
	}
	else
	{
		qwclient_frame_t* frame = cl->qw_frames;
		for (int i = 0; i < UPDATE_BACKUP_QW; i++, frame++)
		{
			if (frame->ping_time > 0)
			{
				ping += frame->ping_time;
				count++;
			}
		}
	}
	if (!count)
	{
		return 9999;
	}
	ping /= count;

	return ping * 1000;
}

//	Writes all update values to a sizebuf
void SVQHW_FullClientUpdate(client_t* client, QMsg* buf)
{
	char info[MAX_INFO_STRING_QW];

	int i = client - svs.clients;

	if (GGameType & GAME_HexenWorld)
	{
		buf->WriteByte(hwsvc_updatedminfo);
		buf->WriteByte(i);
		buf->WriteShort(client->qh_old_frags);
		buf->WriteByte((client->h2_playerclass << 5) | ((int)client->qh_edict->GetLevel() & 31));

		if (hw_dmMode->value == HWDM_SIEGE)
		{
			buf->WriteByte(hwsvc_updatesiegeinfo);
			buf->WriteByte((int)ceil(qh_timelimit->value));
			buf->WriteByte((int)ceil(qh_fraglimit->value));

			buf->WriteByte(hwsvc_updatesiegeteam);
			buf->WriteByte(i);
			buf->WriteByte(client->hw_siege_team);

			buf->WriteByte(hwsvc_updatesiegelosses);
			buf->WriteByte(*pr_globalVars.defLosses);
			buf->WriteByte(*pr_globalVars.attLosses);

			buf->WriteByte(h2svc_time);	//send server time upon connection
			buf->WriteFloat(sv.qh_time);
		}
	}
	else
	{
		buf->WriteByte(q1svc_updatefrags);
		buf->WriteByte(i);
		buf->WriteShort(client->qh_old_frags);
	}

	buf->WriteByte(GGameType & GAME_HexenWorld ? hwsvc_updateping : qwsvc_updateping);
	buf->WriteByte(i);
	buf->WriteShort(SVQH_CalcPing(client));

	if (GGameType & GAME_QuakeWorld)
	{
		buf->WriteByte(qwsvc_updatepl);
		buf->WriteByte(i);
		buf->WriteByte(client->qw_lossage);
	}

	buf->WriteByte(GGameType & GAME_HexenWorld ? hwsvc_updateentertime : qwsvc_updateentertime);
	buf->WriteByte(i);
	buf->WriteFloat(svs.realtime * 0.001 - client->qh_connection_started);

	String::Cpy(info, client->userinfo);
	Info_RemovePrefixedKeys(info, '_', MAX_INFO_STRING_QW);	// server passwords, etc

	buf->WriteByte(GGameType & GAME_HexenWorld ? hwsvc_updateuserinfo : qwsvc_updateuserinfo);
	buf->WriteByte(i);
	buf->WriteLong(client->qh_userid);
	buf->WriteString2(info);
}

//	Writes all update values to a client's reliable stream
void SVQHW_FullClientUpdateToClient(client_t* client, client_t* cl)
{
	SVQH_ClientReliableCheckBlock(cl, 24 + String::Length(client->userinfo));
	if (cl->qw_num_backbuf)
	{
		SVQHW_FullClientUpdate(client, &cl->qw_backbuf);
		SVQH_ClientReliable_FinishWrite(cl);
	}
	else
	{
		SVQHW_FullClientUpdate(client, &cl->netchan.message);
	}
}

//	Pull specific info from a newly changed userinfo string
// into a more C freindly form.
void SVQHW_ExtractFromUserinfo(client_t* cl)
{
	const char* val;
	char* p, * q;
	int i;
	client_t* client;
	int dupc = 1;
	char newname[80];


	// name for C code
	val = Info_ValueForKey(cl->userinfo, "name");

	// trim user name
	String::NCpy(newname, val, sizeof(newname) - 1);
	newname[sizeof(newname) - 1] = 0;

	for (p = newname; (*p == ' ' || *p == '\r' || *p == '\n') && *p; p++)
		;

	if (p != newname && !*p)
	{
		//white space only
		String::Cpy(newname, "unnamed");
		p = newname;
	}

	if (p != newname && *p)
	{
		for (q = newname; *p; *q++ = *p++)
			;
		*q = 0;
	}
	for (p = newname + String::Length(newname) - 1; p != newname && (*p == ' ' || *p == '\r' || *p == '\n'); p--)
		;
	p[1] = 0;

	if (String::Cmp(val, newname))
	{
		Info_SetValueForKey(cl->userinfo, "name", newname, MAX_INFO_STRING_QW, 64, 64, !svqh_highchars->value);
		val = Info_ValueForKey(cl->userinfo, "name");
	}

	if (!val[0] || !String::ICmp(val, "console"))
	{
		Info_SetValueForKey(cl->userinfo, "name", "unnamed", MAX_INFO_STRING_QW, 64, 64, !svqh_highchars->value);
		val = Info_ValueForKey(cl->userinfo, "name");
	}

	// check to see if another user by the same name exists
	while (1)
	{
		for (i = 0, client = svs.clients; i < MAX_CLIENTS_QHW; i++, client++)
		{
			if (client->state != CS_ACTIVE || client == cl)
			{
				continue;
			}
			if (!String::ICmp(client->name, val))
			{
				break;
			}
		}
		if (i != MAX_CLIENTS_QHW)
		{
			// dup name
			char tmp[80];
			String::NCpyZ(tmp, val, sizeof(tmp));
			if (String::Length(tmp) > (int)sizeof(cl->name) - 1)
			{
				tmp[sizeof(cl->name) - 4] = 0;
			}
			p = tmp;

			if (tmp[0] == '(')
			{
				if (tmp[2] == ')')
				{
					p = tmp + 3;
				}
				else if (tmp[3] == ')')
				{
					p = tmp + 4;
				}
			}

			sprintf(newname, "(%d)%-.40s", dupc++, p);
			Info_SetValueForKey(cl->userinfo, "name", newname, MAX_INFO_STRING_QW, 64, 64, !svqh_highchars->value);
			val = Info_ValueForKey(cl->userinfo, "name");
		}
		else
		{
			break;
		}
	}

	if (String::NCmp(val, cl->name, String::Length(cl->name)))
	{
		if (!sv.qh_paused)
		{
			if (!cl->qw_lastnametime || svs.realtime * 0.001 - cl->qw_lastnametime > 5)
			{
				cl->qw_lastnamecount = 0;
				cl->qw_lastnametime = svs.realtime * 0.001;
			}
			else if (cl->qw_lastnamecount++ > 4)
			{
				SVQH_BroadcastPrintf(PRINT_HIGH, "%s was kicked for name spam\n", cl->name);
				SVQH_ClientPrintf(cl, PRINT_HIGH, "You were kicked from the game for name spamming\n");
				SVQHW_DropClient(cl);
				return;
			}
		}

		if (cl->state >= CS_ACTIVE && !cl->qh_spectator)
		{
			SVQH_BroadcastPrintf(PRINT_HIGH, "%s changed name to %s\n", cl->name, val);
		}
	}

	String::NCpy(cl->name, val, sizeof(cl->name) - 1);

	if (GGameType & GAME_Hexen2)
	{
		// playerclass command
		val = Info_ValueForKey(cl->userinfo, "playerclass");
		if (String::Length(val))
		{
			i = String::Atoi(val);
			if (i > CLASS_DEMON && hw_dmMode->value != HWDM_SIEGE)
			{
				i = CLASS_PALADIN;
			}
			if (i < 0 || i > MAX_PLAYER_CLASS || (!cl->hw_portals && i == CLASS_DEMON))
			{
				i = 0;
			}
			cl->hw_next_playerclass =  i;
			cl->qh_edict->SetNextPlayerClass(i);

			if (cl->qh_edict->GetHealth() > 0)
			{
				sprintf(newname,"%d",cl->h2_playerclass);
				Info_SetValueForKey(cl->userinfo, "playerclass", newname, MAX_INFO_STRING_QW, 64, 64, !svqh_highchars->value);
			}
		}
	}

	// msg command
	val = Info_ValueForKey(cl->userinfo, "msg");
	if (String::Length(val))
	{
		cl->messagelevel = String::Atoi(val);
	}
}

const char* SVQ1_GetMapName()
{
	return PR_GetString(sv.qh_edicts->GetMessage());
}

const char* SVH2_GetMapName()
{
	if (sv.qh_edicts->GetMessage() > 0 && sv.qh_edicts->GetMessage() <= prh2_string_count)
	{
		return &prh2_global_strings[prh2_string_index[(int)sv.qh_edicts->GetMessage() - 1]];
	}
	else
	{
		return PR_GetString(sv.qh_edicts->GetNetName());
	}
}

//	Sends the first message from the server to a connected client.
// This will be sent on the initial connection and upon each server load.
void SVQH_SendServerinfo(client_t* client)
{
	SVQH_ClientPrintf(client, 0, "%c\nJLQuake VERSION " JLQUAKE_VERSION_STRING " SERVER (%i CRC)\n", 2, pr_crc);

	client->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_serverinfo : q1svc_serverinfo);
	client->qh_message.WriteLong(GGameType & GAME_Hexen2 ? H2PROTOCOL_VERSION : Q1PROTOCOL_VERSION);
	client->qh_message.WriteByte(svs.qh_maxclients);

	if (!svqh_coop->value && svqh_deathmatch->value)
	{
		client->qh_message.WriteByte(QHGAME_DEATHMATCH);
		if (GGameType & GAME_Hexen2)
		{
			client->qh_message.WriteShort(svh2_kingofhill);
		}
	}
	else
	{
		client->qh_message.WriteByte(QHGAME_COOP);
	}

	client->qh_message.WriteString2(GGameType & GAME_Hexen2 ? SVH2_GetMapName() : SVQ1_GetMapName());

	for (const char** s = sv.qh_model_precache + 1; *s; s++)
	{
		client->qh_message.WriteString2(*s);
	}
	client->qh_message.WriteByte(0);

	for (const char** s = sv.qh_sound_precache + 1; *s; s++)
	{
		client->qh_message.WriteString2(*s);
	}
	client->qh_message.WriteByte(0);

	// send music
	if (GGameType & GAME_Hexen2)
	{
		client->qh_message.WriteByte(h2svc_cdtrack);
		client->qh_message.WriteByte(sv.h2_cd_track);
		client->qh_message.WriteByte(sv.h2_cd_track);

		client->qh_message.WriteByte(h2svc_midi_name);
		client->qh_message.WriteString2(sv.h2_midi_name);
	}
	else
	{
		client->qh_message.WriteByte(q1svc_cdtrack);
		client->qh_message.WriteByte(sv.qh_edicts->GetSounds());
		client->qh_message.WriteByte(sv.qh_edicts->GetSounds());
	}

	// set view
	client->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_setview : q1svc_setview);
	client->qh_message.WriteShort(QH_NUM_FOR_EDICT(client->qh_edict));

	client->qh_message.WriteByte(GGameType & GAME_Hexen2 ? h2svc_signonnum : q1svc_signonnum);
	client->qh_message.WriteByte(1);

	client->qh_sendsignon = true;
	client->state = CS_CONNECTED;		// need prespawn, spawn, etc
}

//	Initializes a client_t for a new net connection.  This will only be called
// once for a player each game, not once for each level change.
static void SVQH_ConnectClient(int clientnum)
{
	client_t* client = svs.clients + clientnum;

	common->DPrintf("Client %s connected\n", client->qh_netconnection->address);

	int edictnum = clientnum + 1;

	qhedict_t* ent = QH_EDICT_NUM(edictnum);

	// set up the client_t
	qsocket_t* netconnection = client->qh_netconnection;
	netchan_t netchan = client->netchan;

	float spawn_parms[NUM_SPAWN_PARMS];
	if (sv.loadgame)
	{
		Com_Memcpy(spawn_parms, client->qh_spawn_parms, sizeof(spawn_parms));
	}
	Com_Memset(client, 0, sizeof(*client));
	if (GGameType & GAME_Hexen2)
	{
		client->h2_send_all_v = true;
	}
	client->qh_netconnection = netconnection;
	client->netchan = netchan;

	String::Cpy(client->name, "unconnected");
	client->state = CS_CONNECTED;
	client->qh_edict = ent;

	client->qh_message.InitOOB(client->qh_messageBuffer, GGameType & GAME_Hexen2 ? MAX_MSGLEN_H2 : MAX_MSGLEN_Q1);
	client->qh_message.allowoverflow = true;		// we can catch it

	if (GGameType & GAME_Hexen2)
	{
		client->datagram.InitOOB(client->datagramBuffer, MAX_MSGLEN_H2);

		Com_Memset(&sv.h2_states[clientnum],0,sizeof(h2client_state2_t));
	}

	if (sv.loadgame)
	{
		Com_Memcpy(client->qh_spawn_parms, spawn_parms, sizeof(spawn_parms));
	}
	else if (GGameType & GAME_Quake)
	{
		// call the progs to get default spawn parms for the new client
		PR_ExecuteProgram(*pr_globalVars.SetNewParms);
		for (int i = 0; i < NUM_SPAWN_PARMS; i++)
		{
			client->qh_spawn_parms[i] = pr_globalVars.parm1[i];
		}
	}

	SVQH_SendServerinfo(client);
}

void SVQH_CheckForNewClients()
{
	//
	// check for new connections
	//
	while (1)
	{
		netadr_t addr;
		qsocket_t* ret = NET_CheckNewConnections(&addr);
		if (!ret)
		{
			break;
		}

		//
		// init a new client structure
		//
		int i;
		for (i = 0; i < svs.qh_maxclients; i++)
		{
			if (svs.clients[i].state == CS_FREE)
			{
				break;
			}
		}
		if (i == svs.qh_maxclients)
		{
			common->FatalError("Host_CheckForNewClients: no free clients");
		}

		Netchan_Setup(NS_SERVER, &svs.clients[i].netchan, addr, 0);
		svs.clients[i].netchan.lastReceived = net_time * 1000;
		svs.clients[i].qh_netconnection = ret;
		SVQH_ConnectClient(i);

		net_activeconnections++;
	}
}

/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

//	Responds with all the info that qplug or qspy can see
// This message can be up to around 5k with worst case string lengths.
static void SVCQHW_Status(const netadr_t& net_from)
{
	Cmd_TokenizeString("status");
	SVQHW_BeginRedirect(net_from);
	common->Printf("%s\n", svs.qh_info);
	for (int i = 0; i < MAX_CLIENTS_QHW; i++)
	{
		client_t* cl = &svs.clients[i];
		if ((cl->state == CS_CONNECTED || cl->state == CS_ACTIVE) && !cl->qh_spectator)
		{
			int top = String::Atoi(Info_ValueForKey(cl->userinfo, "topcolor"));
			int bottom = String::Atoi(Info_ValueForKey(cl->userinfo, "bottomcolor"));
			top = (top < 0) ? 0 : ((top > 13) ? 13 : top);
			bottom = (bottom < 0) ? 0 : ((bottom > 13) ? 13 : bottom);
			int ping = SVQH_CalcPing(cl);
			common->Printf("%i %i %i %i \"%s\" \"%s\" %i %i\n", cl->qh_userid,
				cl->qh_old_frags, (int)(svs.realtime * 0.001 - cl->qh_connection_started) / 60,
				ping, cl->name, Info_ValueForKey(cl->userinfo, "skin"), top, bottom);
		}
	}
	Com_EndRedirect();
}

#define LOG_HIGHWATER   4096
#define LOG_FLUSH       10 * 60
void SVQHW_CheckLog()
{
	QMsg* sz = &svs.qh_log[svs.qh_logsequence & 1];

	// bump sequence if allmost full, or ten minutes have passed and
	// there is something still sitting there
	if (sz->cursize > LOG_HIGHWATER ||
		(svs.realtime * 0.001 - svs.qh_logtime > LOG_FLUSH && sz->cursize))
	{
		// swap buffers and bump sequence
		svs.qh_logtime = svs.realtime * 0.001;
		svs.qh_logsequence++;
		sz = &svs.qh_log[svs.qh_logsequence & 1];
		sz->cursize = 0;
		common->Printf("beginning fraglog sequence %i\n", svs.qh_logsequence);
	}
}

//	Responds with all the logged frags for ranking programs.
// If a sequence number is passed as a parameter and it is
// the same as the current sequence, an A2A_NACK will be returned
// instead of the data.
static void SVCQHW_Log(const netadr_t& net_from)
{
	int seq;
	if (Cmd_Argc() == 2)
	{
		seq = String::Atoi(Cmd_Argv(1));
	}
	else
	{
		seq = -1;
	}

	char data[MAX_DATAGRAM + 64];
	if (seq == svs.qh_logsequence - 1 || !svqhw_fraglogfile)
	{	// they allready have this data, or we aren't logging frags
		data[0] = A2A_NACK;
		NET_SendPacket(NS_SERVER, 1, data, net_from);
		return;
	}

	common->DPrintf("sending log %i to %s\n", svs.qh_logsequence - 1, SOCK_AdrToString(net_from));

	sprintf(data, "stdlog %i\n", svs.qh_logsequence - 1);
	String::Cat(data, sizeof(data), (char*)svs.qh_log_buf[((svs.qh_logsequence - 1) & 1)]);

	NET_SendPacket(NS_SERVER, String::Length(data) + 1, data, net_from);
}

//	Just responds with an acknowledgement
static void SVCQHW_Ping(const netadr_t& net_from)
{
	char data = A2A_ACK;

	NET_SendPacket(NS_SERVER, 1, &data, net_from);
}

//	Returns a challenge number that can be used
// in a subsequent client_connect command.
// We do this to prevent denial of service attacks that
// flood the server with invalid connection IPs.  With a
// challenge, they must give a valid IP address.
static void SVCQW_GetChallenge(const netadr_t& net_from)
{
	int i;
	int oldest;
	int oldestTime;

	oldest = 0;
	oldestTime = 0x7fffffff;

	// see if we already have a challenge for this ip
	for (i = 0; i < MAX_CHALLENGES; i++)
	{
		if (SOCK_CompareBaseAdr(net_from, svs.challenges[i].adr))
		{
			break;
		}
		if (svs.challenges[i].time < oldestTime)
		{
			oldestTime = svs.challenges[i].time;
			oldest = i;
		}
	}

	if (i == MAX_CHALLENGES)
	{
		// overwrite the oldest
		svs.challenges[oldest].challenge = (rand() << 16) ^ rand();
		svs.challenges[oldest].adr = net_from;
		svs.challenges[oldest].time = svs.realtime * 0.001;
		i = oldest;
	}

	// send it back
	NET_OutOfBandPrint(NS_SERVER, net_from, "%c%i", S2C_CHALLENGE,
		svs.challenges[i].challenge);
}

//	A connection request that did not come from the master
static void SVCQHW_DirectConnect(const netadr_t& net_from)
{
	static int userid;

	int qport = 0;
	char userinfo[1024];
	if (GGameType & GAME_HexenWorld)
	{
		String::NCpy(userinfo, Cmd_Argv(2), sizeof(userinfo) - 1);
	}
	else
	{
		int version = String::Atoi(Cmd_Argv(1));
		if (version != QWPROTOCOL_VERSION)
		{
			NET_OutOfBandPrint(NS_SERVER, net_from, "%c\nServer is JLQuake version " JLQUAKE_VERSION_STRING ".\n", A2C_PRINT);
			common->Printf("* rejected connect from version %i\n", version);
			return;
		}

		qport = String::Atoi(Cmd_Argv(2));

		int challenge = String::Atoi(Cmd_Argv(3));

		// note an extra byte is needed to replace spectator key
		String::NCpy(userinfo, Cmd_Argv(4), sizeof(userinfo) - 2);
		userinfo[sizeof(userinfo) - 2] = 0;

		// see if the challenge is valid
		int i;
		for (i = 0; i < MAX_CHALLENGES; i++)
		{
			if (SOCK_CompareBaseAdr(net_from, svs.challenges[i].adr))
			{
				if (challenge == svs.challenges[i].challenge)
				{
					break;		// good
				}
				NET_OutOfBandPrint(NS_SERVER, net_from, "%c\nBad challenge.\n", A2C_PRINT);
				return;
			}
		}
		if (i == MAX_CHALLENGES)
		{
			NET_OutOfBandPrint(NS_SERVER, net_from, "%c\nNo challenge for address.\n", A2C_PRINT);
			return;
		}
	}

	// check for password or spectator_password
	const char* s = Info_ValueForKey(userinfo, "spectator");
	bool spectator;
	if (s[0] && String::Cmp(s, "0"))
	{
		if (svqhw_spectator_password->string[0] &&
			String::ICmp(svqhw_spectator_password->string, "none") &&
			String::Cmp(svqhw_spectator_password->string, s))
		{	// failed
			common->Printf("%s:spectator password failed\n", SOCK_AdrToString(net_from));
			NET_OutOfBandPrint(NS_SERVER, net_from, "%c\nrequires a spectator password\n\n", A2C_PRINT);
			return;
		}
		Info_RemoveKey(userinfo, "spectator", MAX_INFO_STRING_QW);	// remove passwd
		Info_SetValueForKey(userinfo, "*spectator", "1", MAX_INFO_STRING_QW, 64, 64, !svqh_highchars->value);
		spectator = true;
	}
	else
	{
		s = Info_ValueForKey(userinfo, "password");
		if (svqhw_password->string[0] &&
			String::ICmp(svqhw_password->string, "none") &&
			String::Cmp(svqhw_password->string, s))
		{
			common->Printf("%s:password failed\n", SOCK_AdrToString(net_from));
			NET_OutOfBandPrint(NS_SERVER, net_from, "%c\nserver requires a password\n\n", A2C_PRINT);
			return;
		}
		spectator = false;
		Info_RemoveKey(userinfo, "password", MAX_INFO_STRING_QW);	// remove passwd
	}

	netadr_t adr = net_from;
	userid++;	// so every client gets a unique id

	client_t temp;
	client_t* newcl = &temp;
	Com_Memset(newcl, 0, sizeof(client_t));

	newcl->qh_userid = userid;
	if (GGameType & GAME_HexenWorld)
	{
		newcl->hw_portals = atol(Cmd_Argv(1));
	}

	// works properly
	if (!svqh_highchars->value)
	{
		byte* q = reinterpret_cast<byte*>(userinfo);
		for (byte* p = reinterpret_cast<byte*>(newcl->userinfo); *q && p < (byte*)newcl->userinfo + MAX_INFO_STRING_QW - 1; q++)
		{
			if (*q > 31 && *q <= 127)
			{
				*p++ = *q;
			}
		}
	}
	else
	{
		String::NCpy(newcl->userinfo, userinfo, MAX_INFO_STRING_QW - 1);
	}

	// if there is allready a slot for this ip, drop it
	client_t* cl = svs.clients;
	for (int i = 0; i < MAX_CLIENTS_QHW; i++,cl++)
	{
		if (cl->state == CS_FREE)
		{
			continue;
		}
		if (SOCK_CompareBaseAdr(adr, cl->netchan.remoteAddress) &&
			((GGameType & GAME_QuakeWorld && cl->netchan.qport == qport) ||
			adr.port == cl->netchan.remoteAddress.port))
		{
			if (GGameType & GAME_QuakeWorld && cl->state == CS_CONNECTED)
			{
				common->Printf("%s:dup connect\n", SOCK_AdrToString(adr));
				userid--;
				return;
			}

			common->Printf("%s:reconnect\n", SOCK_AdrToString(adr));
			SVQHW_DropClient(cl);
			break;
		}
	}

	// count up the clients and spectators
	int clients = 0;
	int spectators = 0;
	cl = svs.clients;
	for (int i = 0; i < MAX_CLIENTS_QHW; i++,cl++)
	{
		if (cl->state == CS_FREE)
		{
			continue;
		}
		if (cl->qh_spectator)
		{
			spectators++;
		}
		else
		{
			clients++;
		}
	}

	// if at server limits, refuse connection
	if (sv_maxclients->value > MAX_CLIENTS_QHW)
	{
		Cvar_SetValue("maxclients", MAX_CLIENTS_QHW);
	}
	if (svqhw_maxspectators->value > MAX_CLIENTS_QHW)
	{
		Cvar_SetValue("maxspectators", MAX_CLIENTS_QHW);
	}
	if (svqhw_maxspectators->value + sv_maxclients->value > MAX_CLIENTS_QHW)
	{
		Cvar_SetValue("maxspectators", MAX_CLIENTS_QHW - svqhw_maxspectators->value + sv_maxclients->value);
	}
	if ((spectator && spectators >= (int)svqhw_maxspectators->value) ||
		(!spectator && clients >= (int)sv_maxclients->value))
	{
		common->Printf("%s:full connect\n", SOCK_AdrToString(adr));
		NET_OutOfBandPrint(NS_SERVER, adr, "%c\nserver is full\n\n", A2C_PRINT);
		return;
	}

	// find a client slot
	newcl = NULL;
	cl = svs.clients;
	for (int i = 0; i < MAX_CLIENTS_QHW; i++,cl++)
	{
		if (cl->state == CS_FREE)
		{
			newcl = cl;
			break;
		}
	}
	if (!newcl)
	{
		common->Printf("WARNING: miscounted available clients\n");
		return;
	}


	// build a new connection
	// accept the new client
	// this is the only place a client_t is ever initialized
	*newcl = temp;

	NET_OutOfBandPrint(NS_SERVER, adr, "%c", S2C_CONNECTION);

	int edictnum = (newcl - svs.clients) + 1;

	Netchan_Setup(NS_SERVER, &newcl->netchan, adr, qport);
	newcl->netchan.lastReceived = svs.realtime;

	newcl->state = CS_CONNECTED;

	newcl->datagram.InitOOB(newcl->datagramBuffer, GGameType & GAME_HexenWorld ? MAX_DATAGRAM_HW : MAX_DATAGRAM_QW);
	newcl->datagram.allowoverflow = true;

	// spectator mode can ONLY be set at join time
	newcl->qh_spectator = spectator;

	qhedict_t* ent = QH_EDICT_NUM(edictnum);
	newcl->qh_edict = ent;
	if (GGameType & GAME_HexenWorld)
	{
		ED_ClearEdict(ent);
	}

	// parse some info from the info strings
	SVQHW_ExtractFromUserinfo(newcl);

	// JACK: Init the floodprot stuff.
	for (int i = 0; i < 10; i++)
	{
		newcl->qh_whensaid[i] = 0.0;
	}
	newcl->qh_whensaidhead = 0;
	newcl->qh_lockedtill = 0;

	// call the progs to get default spawn parms for the new client
	PR_ExecuteProgram(*pr_globalVars.SetNewParms);
	for (int i = 0; i < NUM_SPAWN_PARMS; i++)
	{
		newcl->qh_spawn_parms[i] = pr_globalVars.parm1[i];
	}

	if (newcl->qh_spectator)
	{
		common->Printf("Spectator %s connected\n", newcl->name);
	}
	else
	{
		common->DPrintf("Client %s connected\n", newcl->name);
	}
	if (GGameType & GAME_QuakeWorld)
	{
		newcl->qh_sendinfo = true;
	}
}

static bool SVQHW_Rcon_Validate()
{
	if (!String::Length(svqhw_rcon_password->string))
	{
		return false;
	}

	if (String::Cmp(Cmd_Argv(1), svqhw_rcon_password->string))
	{
		return false;
	}

	return true;
}

//	A client issued an rcon command.
// Shift down the remaining args
// Redirect all printfs
static void SVCQHW_RemoteCommand(const netadr_t& net_from, QMsg& message)
{
	if (!SVQHW_Rcon_Validate())
	{
		common->Printf("Bad rcon from %s:\n%s\n",
			SOCK_AdrToString(net_from), message._data + 4);

		SVQHW_BeginRedirect(net_from);

		common->Printf("Bad rcon_password.\n");
	}
	else
	{
		common->Printf("Rcon from %s:\n%s\n",
			SOCK_AdrToString(net_from), message._data + 4);

		SVQHW_BeginRedirect(net_from);

		char remaining[1024];
		remaining[0] = 0;

		for (int i = 2; i < Cmd_Argc(); i++)
		{
			String::Cat(remaining, sizeof(remaining), Cmd_Argv(i));
			String::Cat(remaining, sizeof(remaining), " ");
		}

		Cmd_ExecuteString(remaining);
	}

	Com_EndRedirect();
}

//	A connectionless packet has four leading 0xff
// characters to distinguish it from a game channel.
// Clients that are in the game can still send
// connectionless packets.
void SVQHW_ConnectionlessPacket(const netadr_t& net_from, QMsg& message)
{
	message.BeginReadingOOB();
	message.ReadLong();		// skip the -1 marker

	const char* s = message.ReadStringLine2();

	Cmd_TokenizeString(s);

	const char* c = Cmd_Argv(0);

	if (!String::Cmp(c, "ping") || (c[0] == A2A_PING && (c[1] == 0 || c[1] == '\n')))
	{
		SVCQHW_Ping(net_from);
		return;
	}
	if (c[0] == A2A_ACK && (c[1] == 0 || c[1] == '\n'))
	{
		common->Printf("A2A_ACK from %s\n", SOCK_AdrToString(net_from));
		return;
	}
	else if (GGameType & GAME_HexenWorld && c[0] == A2S_ECHO)
	{
		NET_SendPacket(NS_SERVER, message.cursize, message._data, net_from);
		return;
	}
	else if (!String::Cmp(c,"status"))
	{
		SVCQHW_Status(net_from);
		return;
	}
	else if (!String::Cmp(c,"log"))
	{
		SVCQHW_Log(net_from);
		return;
	}
	else if (!String::Cmp(c,"connect"))
	{
		SVCQHW_DirectConnect(net_from);
		return;
	}
	else if (GGameType & GAME_QuakeWorld && !String::Cmp(c,"getchallenge"))
	{
		SVCQW_GetChallenge(net_from);
		return;
	}
	else if (!String::Cmp(c, "rcon"))
	{
		SVCQHW_RemoteCommand(net_from, message);
	}
	else
	{
		common->Printf("bad connectionless packet from %s:\n%s\n",
			SOCK_AdrToString(net_from), s);
	}
}

/*
==============================================================================

PACKET FILTERING


You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and any unspecified digits will match any value, so you can specify an entire class C network with "addip 192.246.40".

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

writeip
Dumps "addip <ip>" commands to listip.cfg so it can be execed at a later date.  The filter lists are not saved and restored by default, because I beleive it would cause too much confusion.

filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.


==============================================================================
*/


struct ipfilter_t
{
	unsigned mask;
	unsigned compare;
};

#define MAX_IPFILTERS   1024

static ipfilter_t qhw_ipfilters[MAX_IPFILTERS];
static int qhw_numipfilters;

Cvar* qhw_filterban;

static bool StringToFilter(char* s, ipfilter_t* f)
{
	byte b[4];
	byte m[4];
	for (int i = 0; i < 4; i++)
	{
		b[i] = 0;
		m[i] = 0;
	}

	for (int i = 0; i < 4; i++)
	{
		if (*s < '0' || *s > '9')
		{
			common->Printf("Bad filter address: %s\n", s);
			return false;
		}

		int j = 0;
		char num[128];
		while (*s >= '0' && *s <= '9')
		{
			num[j++] = *s++;
		}
		num[j] = 0;
		b[i] = String::Atoi(num);
		if (b[i] != 0)
		{
			m[i] = 255;
		}

		if (!*s)
		{
			break;
		}
		s++;
	}

	f->mask = *(unsigned*)m;
	f->compare = *(unsigned*)b;

	return true;
}

void SVQHW_AddIP_f()
{
	int i;
	for (i = 0; i < qhw_numipfilters; i++)
	{
		if (qhw_ipfilters[i].compare == 0xffffffff)
		{
			// free spot
			break;
		}
	}
	if (i == qhw_numipfilters)
	{
		if (qhw_numipfilters == MAX_IPFILTERS)
		{
			common->Printf("IP filter list is full\n");
			return;
		}
		qhw_numipfilters++;
	}

	if (!StringToFilter(Cmd_Argv(1), &qhw_ipfilters[i]))
	{
		qhw_ipfilters[i].compare = 0xffffffff;
	}
}

void SVQHW_RemoveIP_f()
{
	ipfilter_t f;
	if (!StringToFilter(Cmd_Argv(1), &f))
	{
		return;
	}
	for (int i = 0; i < qhw_numipfilters; i++)
	{
		if (qhw_ipfilters[i].mask == f.mask &&
			qhw_ipfilters[i].compare == f.compare)
		{
			for (int j = i + 1; j < qhw_numipfilters; j++)
			{
				qhw_ipfilters[j - 1] = qhw_ipfilters[j];
			}
			qhw_numipfilters--;
			common->Printf("Removed.\n");
			return;
		}
	}
	common->Printf("Didn't find %s.\n", Cmd_Argv(1));
}

void SVQHW_ListIP_f()
{
	common->Printf("Filter list:\n");
	for (int i = 0; i < qhw_numipfilters; i++)
	{
		byte b[4];
		*(unsigned*)b = qhw_ipfilters[i].compare;
		common->Printf("%3i.%3i.%3i.%3i\n", b[0], b[1], b[2], b[3]);
	}
}

void SVQHW_WriteIP_f()
{
	char name[MAX_OSPATH];
	sprintf(name, "listip.cfg");

	common->Printf("Writing %s.\n", name);

	fileHandle_t f = FS_FOpenFileWrite(name);
	if (!f)
	{
		common->Printf("Couldn't open %s\n", name);
		return;
	}

	for (int i = 0; i < qhw_numipfilters; i++)
	{
		byte b[4];
		*(unsigned*)b = qhw_ipfilters[i].compare;
		FS_Printf(f, "addip %i.%i.%i.%i\n", b[0], b[1], b[2], b[3]);
	}

	FS_FCloseFile(f);
}

void SVQHW_SendBan(const netadr_t& net_from)
{
	char data[128];
	data[0] = data[1] = data[2] = data[3] = 0xff;
	data[4] = A2C_PRINT;
	data[5] = 0;
	String::Cat(data, sizeof(data), "\nbanned.\n");

	NET_SendPacket(NS_SERVER, String::Length(data), data, net_from);
}

bool SVQHW_FilterPacket(const netadr_t& net_from)
{
	unsigned in = *(unsigned*)net_from.ip;

	for (int i = 0; i < qhw_numipfilters; i++)
	{
		if ((in & qhw_ipfilters[i].mask) == qhw_ipfilters[i].compare)
		{
			return qhw_filterban->value;
		}
	}

	return !qhw_filterban->value;
}
