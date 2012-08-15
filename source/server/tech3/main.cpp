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
#include "local.h"
#include "../../common/file_formats/tag.h"

Cvar* svt3_gametype;
Cvar* svt3_pure;
Cvar* svt3_padPackets;		// add nop bytes to messages
Cvar* svt3_maxRate;
Cvar* svt3_dl_maxRate;
Cvar* svt3_mapname;
Cvar* svt3_timeout;			// seconds without any message
Cvar* svt3_zombietime;		// seconds to sink messages after disconnect
Cvar* svt3_allowDownload;
Cvar* svet_wwwFallbackURL;	// URL to send to if an http/ftp fails or is refused client side
Cvar* svet_wwwDownload;		// server does a www dl redirect
Cvar* svet_wwwBaseURL;	// base URL for redirect
// tell clients to perform their downloads while disconnected from the server
// this gets you a better throughput, but you loose the ability to control the download usage
Cvar* svet_wwwDlDisconnected;
Cvar* svt3_lanForceRate;		// dedicated 1 (LAN) server forces local client rates to 99999 (bug #491)
Cvar* svwm_showAverageBPS;		// NERVE - SMF - net debugging
Cvar* svet_tempbanmessage;
Cvar* svwm_onlyVisibleClients;
Cvar* svq3_strictAuth;
Cvar* svet_fullmsg;
Cvar* svt3_reconnectlimit;		// minimum seconds between connect messages
Cvar* svt3_minPing;
Cvar* svt3_maxPing;
Cvar* svt3_privatePassword;		// password for the privateClient slots
Cvar* svt3_privateClients;		// number of clients reserved for password
Cvar* svt3_floodProtect;
Cvar* svt3_reloading;

//	Converts newlines to "\n" so a line prints nicer
static const char* SVT3_ExpandNewlines(const char* in)
{
	static char string[1024];
	int l = 0;
	while (*in && l < (int)sizeof(string) - 3)
	{
		if (*in == '\n')
		{
			string[l++] = '\\';
			string[l++] = 'n';
		}
		else
		{
			// NERVE - SMF - HACK - strip out localization tokens before string command is displayed in syscon window
			if (GGameType & (GAME_WolfMP | GAME_ET) &&
				(!String::NCmp(in, "[lon]", 5) || !String::NCmp(in, "[lof]", 5)))
			{
				in += 5;
				continue;
			}

			string[l++] = *in;
		}
		in++;
	}
	string[l] = 0;

	return string;
}

//	The given command will be transmitted to the client, and is guaranteed to
// not have future snapshot_t executed before it is executed
void SVT3_AddServerCommand(client_t* client, const char* cmd)
{
	client->q3_reliableSequence++;
	// if we would be losing an old command that hasn't been acknowledged,
	// we must drop the connection
	// we check == instead of >= so a broadcast print added by SVT3_DropClient()
	// doesn't cause a recursive drop client
	int maxReliableCommands = GGameType & GAME_Quake3 ? MAX_RELIABLE_COMMANDS_Q3 : MAX_RELIABLE_COMMANDS_WOLF;
	if (client->q3_reliableSequence - client->q3_reliableAcknowledge == maxReliableCommands + 1)
	{
		common->Printf("===== pending server commands =====\n");
		int i;
		for (i = client->q3_reliableAcknowledge + 1; i <= client->q3_reliableSequence; i++)
		{
			common->Printf("cmd %5d: %s\n", i, SVT3_GetReliableCommand(client, i & (maxReliableCommands - 1)));
		}
		common->Printf("cmd %5d: %s\n", i, cmd);
		SVT3_DropClient(client, "Server command overflow");
		return;
	}
	int index = client->q3_reliableSequence & (maxReliableCommands - 1);
	SVT3_AddReliableCommand(client, index, cmd);
}

//	Sends a reliable command string to be interpreted by
// the client game module: "cp", "print", "chat", etc
// A NULL client will broadcast to all clients
void SVT3_SendServerCommand(client_t* cl, const char* fmt, ...)
{
	va_list argptr;
	byte message[MAX_MSGLEN];
	va_start(argptr,fmt);
	Q_vsnprintf((char*)message, sizeof(message), fmt, argptr);
	va_end(argptr);

	// do not forward server command messages that would be too big to clients
	// ( q3infoboom / q3msgboom stuff )
	if (GGameType & (GAME_WolfMP | GAME_ET) && String::Length((char*)message) > 1022)
	{
		return;
	}

	if (cl != NULL)
	{
		SVT3_AddServerCommand(cl, (char*)message);
		return;
	}

	// hack to echo broadcast prints to console
	if (com_dedicated->integer && !String::NCmp((char*)message, "print", 5))
	{
		common->Printf("broadcast: %s\n", SVT3_ExpandNewlines((char*)message));
	}

	// send the data to all relevent clients
	client_t* client = svs.clients;
	for (int j = 0; j < sv_maxclients->integer; j++, client++)
	{
		if (client->state < CS_PRIMED)
		{
			continue;
		}
		// Ridah, don't need to send messages to AI
		if (client->q3_entity && client->q3_entity->GetSvFlagCastAI())
		{
			continue;
		}
		if (GGameType & GAME_ET && client->q3_entity && client->q3_entity->GetSvFlags() & Q3SVF_BOT)
		{
			continue;
		}
		SVT3_AddServerCommand(client, (char*)message);
	}
}

int SVET_LoadTag(const char* mod_name)
{
	for (int i = 0; i < sv.et_num_tagheaders; i++)
	{
		if (!String::ICmp(mod_name, sv.et_tagHeadersExt[i].filename))
		{
			return i + 1;
		}
	}

	unsigned char* buffer;
	FS_ReadFile(mod_name, (void**)&buffer);

	if (!buffer)
	{
		return 0;
	}

	tagHeader_t* pinmodel = (tagHeader_t*)buffer;

	int version = LittleLong(pinmodel->version);
	if (version != TAG_VERSION)
	{
		common->Printf(S_COLOR_YELLOW "WARNING: SVET_LoadTag: %s has wrong version (%i should be %i)\n", mod_name, version, TAG_VERSION);
		FS_FreeFile(buffer);
		return 0;
	}

	if (sv.et_num_tagheaders >= MAX_TAG_FILES_ET)
	{
		common->Error("MAX_TAG_FILES_ET reached\n");

		FS_FreeFile(buffer);
		return 0;
	}

#define LL(x) x = LittleLong(x)
	LL(pinmodel->ident);
	LL(pinmodel->numTags);
	LL(pinmodel->ofsEnd);
	LL(pinmodel->version);
#undef LL

	String::NCpyZ(sv.et_tagHeadersExt[sv.et_num_tagheaders].filename, mod_name, MAX_QPATH);
	sv.et_tagHeadersExt[sv.et_num_tagheaders].start = sv.et_num_tags;
	sv.et_tagHeadersExt[sv.et_num_tagheaders].count = pinmodel->numTags;

	if (sv.et_num_tags + pinmodel->numTags >= MAX_SERVER_TAGS_ET)
	{
		common->Error("MAX_SERVER_TAGS_ET reached\n");

		FS_FreeFile(buffer);
		return 0;
	}

	// swap all the tags
	md3Tag_t* tag = &sv.et_tags[sv.et_num_tags];
	sv.et_num_tags += pinmodel->numTags;
	md3Tag_t* readTag = (md3Tag_t*)(buffer + sizeof(tagHeader_t));
	for (int i = 0; i < pinmodel->numTags; i++, tag++, readTag++)
	{
		for (int j = 0; j < 3; j++)
		{
			tag->origin[j] = LittleFloat(readTag->origin[j]);
			tag->axis[0][j] = LittleFloat(readTag->axis[0][j]);
			tag->axis[1][j] = LittleFloat(readTag->axis[1][j]);
			tag->axis[2][j] = LittleFloat(readTag->axis[2][j]);
		}
		String::NCpyZ(tag->name, readTag->name, 64);
	}

	FS_FreeFile(buffer);
	return ++sv.et_num_tagheaders;
}

//	Updates the cl->ping variables
void SVT3_CalcPings()
{
	for (int i = 0; i < sv_maxclients->integer; i++)
	{
		client_t* cl = &svs.clients[i];
		if (cl->state != CS_ACTIVE)
		{
			cl->ping = 999;
			continue;
		}
		if (!cl->q3_entity)
		{
			cl->ping = 999;
			continue;
		}
		if (cl->q3_entity->GetSvFlags() & Q3SVF_BOT)
		{
			cl->ping = 0;
			continue;
		}

		int total = 0;
		int count = 0;
		for (int j = 0; j < PACKET_BACKUP_Q3; j++)
		{
			if (cl->q3_frames[j].messageAcked <= 0)
			{
				continue;
			}
			int delta = cl->q3_frames[j].messageAcked - cl->q3_frames[j].messageSent;
			count++;
			total += delta;
		}
		if (!count)
		{
			cl->ping = 999;
		}
		else
		{
			cl->ping = total / count;
			if (cl->ping > 999)
			{
				cl->ping = 999;
			}
		}

		// let the game dll know about the ping
		idPlayerState3* ps = SVT3_GameClientNum(i);
		ps->SetPing(cl->ping);
	}
}

//	If a packet has not been received from a client for timeout->integer
// seconds, drop the conneciton.  Server time is used instead of
// realtime to avoid dropping the local client while debugging.
//	When a client is normally dropped, the client_t goes into a zombie state
// for a few seconds to make sure any final reliable message gets resent
// if necessary
void SVT3_CheckTimeouts()
{
	int droppoint = svs.q3_time - 1000 * svt3_timeout->integer;
	int zombiepoint = svs.q3_time - 1000 * svt3_zombietime->integer;

	client_t* cl = svs.clients;
	for (int i = 0; i < sv_maxclients->integer; i++,cl++)
	{
		// message times may be wrong across a changelevel
		if (cl->q3_lastPacketTime > svs.q3_time)
		{
			cl->q3_lastPacketTime = svs.q3_time;
		}

		if (cl->state == CS_ZOMBIE &&
			cl->q3_lastPacketTime < zombiepoint)
		{
			// using the client id cause the cl->name is empty at this point
			common->DPrintf("Going from CS_ZOMBIE to CS_FREE for client %d\n", i);
			cl->state = CS_FREE;	// can now be reused
			continue;
		}
		// Ridah, AI's don't time out
		if (!(GGameType & GAME_WolfSP) || (cl->q3_entity && !cl->q3_entity->GetSvFlagCastAI()))
		{
			if (cl->state >= CS_CONNECTED && cl->q3_lastPacketTime < droppoint)
			{
				// wait several frames so a debugger session doesn't
				// cause a timeout
				if (++cl->q3_timeoutCount > 5)
				{
					SVT3_DropClient(cl, "timed out");
					cl->state = CS_FREE;	// don't bother with zombie state
				}
			}
			else
			{
				cl->q3_timeoutCount = 0;
			}
		}
	}
}

bool SVT3_CheckPaused()
{
	if (!cl_paused->integer)
	{
		return false;
	}

	// only pause if there is just a single client connected
	int count = 0;
	client_t* cl = svs.clients;
	for (int i = 0; i < sv_maxclients->integer; i++,cl++)
	{
		if (cl->state >= CS_CONNECTED && cl->netchan.remoteAddress.type != NA_BOT)
		{
			count++;
		}
	}

	if (count > 1)
	{
		// don't pause
		if (sv_paused->integer)
		{
			Cvar_Set("sv_paused", "0");
		}
		return false;
	}

	if (!sv_paused->integer)
	{
		Cvar_Set("sv_paused", "1");
	}
	return true;
}
