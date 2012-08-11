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
#ifdef _WIN32
#include "../../client/windows_shared.h"
#else
#include <unistd.h>
#endif
#include <time.h>

quakeparms_t host_parms;

qboolean host_initialized;			// true if into command execution (compatability)

double host_frametime;
double realtime;					// without any filtering or bounding

int host_hunklevel;

client_t* host_client;				// current client

//
// game rules mirrored in svs.qh_info
//
Cvar* samelevel;
Cvar* spawn;
Cvar* watervis;

fileHandle_t sv_logfile;

int sv_net_port;

void SV_AcceptClient(netadr_t adr, int userid, char* userinfo);
void SVQHW_Master_Shutdown(void);

class idCommonLocal : public idCommon
{
public:
	virtual void Printf(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void DPrintf(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void Error(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void FatalError(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void EndGame(const char* format, ...) id_attribute((format(printf, 2, 3)));
};

static idCommonLocal commonLocal;
idCommon* common = &commonLocal;

void idCommonLocal::Printf(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	Con_Printf("%s", string);
}

void idCommonLocal::DPrintf(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	Con_DPrintf("%s", string);
}

void idCommonLocal::Error(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	throw DropException(string);
}

void idCommonLocal::FatalError(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	throw Exception(string);
}

void idCommonLocal::EndGame(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	throw EndGameException(string);
}

//============================================================================

void CL_Disconnect()
{
}

/*
================
SVQHW_Shutdown

Quake calls this before calling Sys_Quit or Sys_Error
================
*/
void SVQHW_Shutdown()
{
	SVQHW_Master_Shutdown();
	if (sv_logfile)
	{
		FS_FCloseFile(sv_logfile);
		sv_logfile = 0;
	}
	if (svqhw_fraglogfile)
	{
		FS_FCloseFile(svqhw_fraglogfile);
		svqhw_fraglogfile = 0;
	}
	NET_Shutdown();
}

/*
================
SV_Error

Sends a datagram to all the clients informing them of the server crash,
then exits
================
*/
void SV_Error(const char* error, ...)
{
	va_list argptr;
	static char string[1024];
	static qboolean inerror = false;

	if (inerror)
	{
		Sys_Error("SV_Error: recursively entered (%s)", string);
	}

	inerror = true;

	va_start(argptr,error);
	Q_vsnprintf(string, 1024, error, argptr);
	va_end(argptr);

	common->Printf("SV_Error: %s\n",string);

	SV_FinalMessage(va("server crashed: %s\n", string));

	SVQHW_Shutdown();

	Sys_Error("SV_Error: %s\n",string);
}

/*
==================
SV_FinalMessage

Used by common->Error and SV_Quit_f to send a final message to all connected
clients before the server goes down.  The messages are sent immediately,
not just stuck on the outgoing message list, because the server is going
to totally exit after returning from this function.
==================
*/
void SV_FinalMessage(const char* message)
{
	int i;
	client_t* cl;

	net_message.Clear();
	net_message.WriteByte(q1svc_print);
	net_message.WriteByte(PRINT_HIGH);
	net_message.WriteString2(message);
	net_message.WriteByte(q1svc_disconnect);

	for (i = 0, cl = svs.clients; i < MAX_CLIENTS_QHW; i++, cl++)
		if (cl->state >= CS_ACTIVE)
		{
			Netchan_Transmit(&cl->netchan, net_message.cursize,
				net_message._data);
		}
}

//============================================================================

/*
===================
SV_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
void SV_GetConsoleCommands(void)
{
	char* cmd;

	while (1)
	{
		cmd = Sys_ConsoleInput();
		if (!cmd)
		{
			break;
		}
		Cbuf_AddText(cmd);
	}
}

/*
==================
SV_Frame

==================
*/
void SV_Frame(float time)
{
	try
	{
		static double start, end;

		start = Sys_DoubleTime();
		svs.qh_stats.idle += start - end;

// keep the random time dependent
		rand();

// decide the simulation time
		if (!sv.qh_paused)
		{
			realtime += time;
			sv.qh_time += time;
			svs.realtime = realtime * 1000;
		}

		for (sysEvent_t ev = Sys_SharedGetEvent(); ev.evType; ev = Sys_SharedGetEvent())
		{
			switch (ev.evType)
			{
			case SE_CONSOLE:
				Cbuf_AddText((char*)ev.evPtr);
				Cbuf_AddText("\n");
				break;
			default:
				break;
			}

			// free any block data
			if (ev.evPtr)
			{
				Mem_Free(ev.evPtr);
			}
		}

// check for commands typed to the host
		SV_GetConsoleCommands();

// process console commands
		Cbuf_Execute();

		SVQHW_ServerFrame();

// collect timing statistics
		end = Sys_DoubleTime();
		svs.qh_stats.active += end - start;
		if (++svs.qh_stats.count == STATFRAMES)
		{
			svs.qh_stats.latched_active = svs.qh_stats.active;
			svs.qh_stats.latched_idle = svs.qh_stats.idle;
			svs.qh_stats.latched_packets = svs.qh_stats.packets;
			svs.qh_stats.active = 0;
			svs.qh_stats.idle = 0;
			svs.qh_stats.packets = 0;
			svs.qh_stats.count = 0;
		}
	}
	catch (DropException& e)
	{
		SV_Error("%s", e.What());
	}
	catch (Exception& e)
	{
		Sys_Error("%s", e.What());
	}
}

/*
===============
SV_InitLocal
===============
*/
void SV_InitLocal(void)
{
	int i;

	SV_InitOperatorCommands();

	VQH_InitRollCvars();

	svqhw_spectalk = Cvar_Get("sv_spectalk", "1", 0);
	svqw_mapcheck = Cvar_Get("sv_mapcheck", "1", 0);

	svqhw_rcon_password = Cvar_Get("rcon_password", "", 0);	// password for remote server commands
	svqhw_password = Cvar_Get("password", "", 0);	// password for entering the game
	svqhw_spectator_password = Cvar_Get("spectator_password", "", 0);	// password for entering as a sepctator

	qh_fraglimit = Cvar_Get("fraglimit", "0", CVAR_SERVERINFO);
	qh_timelimit = Cvar_Get("timelimit","0", CVAR_SERVERINFO);
	svqh_teamplay = Cvar_Get("teamplay","0", CVAR_SERVERINFO);
	samelevel = Cvar_Get("samelevel","0", CVAR_SERVERINFO);
	sv_maxclients = Cvar_Get("maxclients","8", CVAR_SERVERINFO);
	svqhw_maxspectators = Cvar_Get("maxspectators","8", CVAR_SERVERINFO);
	svqh_deathmatch = Cvar_Get("deathmatch","1", CVAR_SERVERINFO);			// 0, 1, or 2
	spawn = Cvar_Get("spawn","0", CVAR_SERVERINFO);
	watervis = Cvar_Get("watervis", "0", CVAR_SERVERINFO);
	sv_hostname = Cvar_Get("hostname","unnamed", CVAR_SERVERINFO);

	svqhw_timeout = Cvar_Get("timeout", "65", 0);		// seconds without any message
	svqhw_zombietime = Cvar_Get("zombietime", "2", 0);	// seconds to sink messages
	// after disconnect

	SVQH_RegisterPhysicsCvars();

	svqh_aim = Cvar_Get("sv_aim", "2", 0);

	qhw_filterban = Cvar_Get("filterban", "1", 0);

	qhw_allow_download = Cvar_Get("allow_download", "1", 0);
	qhw_allow_download_skins = Cvar_Get("allow_download_skins", "1", 0);
	qhw_allow_download_models = Cvar_Get("allow_download_models", "1", 0);
	qhw_allow_download_sounds = Cvar_Get("allow_download_sounds", "1", 0);
	qhw_allow_download_maps = Cvar_Get("allow_download_maps", "1", 0);

	svqh_highchars = Cvar_Get("sv_highchars", "1", 0);

	sv_phs = Cvar_Get("sv_phs", "1", 0);

	qh_pausable    = Cvar_Get("pausable", "1", 0);

	Cmd_AddCommand("addip", SVQHW_AddIP_f);
	Cmd_AddCommand("removeip", SVQHW_RemoveIP_f);
	Cmd_AddCommand("listip", SVQHW_ListIP_f);
	Cmd_AddCommand("writeip", SVQHW_WriteIP_f);

	for (i = 0; i < MAX_MODELS_Q1; i++)
		sprintf(svqh_localmodels[i], "*%i", i);

	Info_SetValueForKey(svs.qh_info, "*version", va("%4.2f", VERSION), MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value);

	svs.clients = new client_t[MAX_CLIENTS_QHW];
	Com_Memset(svs.clients, 0, sizeof(client_t) * MAX_CLIENTS_QHW);

	// init fraglog stuff
	svs.qh_logsequence = 1;
	svs.qh_logtime = realtime;
	svs.qh_log[0].InitOOB(svs.qh_log_buf[0], sizeof(svs.qh_log_buf[0]));
	svs.qh_log[0].allowoverflow = true;
	svs.qh_log[1].InitOOB(svs.qh_log_buf[1], sizeof(svs.qh_log_buf[1]));
	svs.qh_log[1].allowoverflow = true;
}


//============================================================================

/*
====================
SV_InitNet
====================
*/
void SV_InitNet(void)
{
	int p;

	sv_net_port = PORT_SERVER;
	p = COM_CheckParm("-port");
	if (p && p < COM_Argc())
	{
		sv_net_port = String::Atoi(COM_Argv(p + 1));
		common->Printf("Port: %i\n", sv_net_port);
	}
	NET_Init(sv_net_port);

	// pick a port value that should be nice and random
#ifdef _WIN32
	Netchan_Init((int)(timeGetTime() * 1000) * time(NULL));
#else
	Netchan_Init((int)(getpid() + getuid() * 1000) * time(NULL));
#endif

	// heartbeats will allways be sent to the id master
	svs.qh_last_heartbeat = -99999;		// send immediately
}


/*
====================
SV_Init
====================
*/
void SV_Init(quakeparms_t* parms)
{
	try
	{
		GGameType = GAME_Quake | GAME_QuakeWorld;
		Sys_SetHomePathSuffix("jlquake");

		COM_InitArgv2(parms->argc, parms->argv);
		COM_AddParm("-game");
		COM_AddParm("qw");

		if (COM_CheckParm("-minmemory"))
		{
			parms->memsize = MINIMUM_MEMORY;
		}

		host_parms = *parms;

		if (parms->memsize < MINIMUM_MEMORY)
		{
			common->Error("Only %4.1f megs of memory reported, can't execute game", parms->memsize / (float)0x100000);
		}

		Memory_Init(parms->membase, parms->memsize);
		Cbuf_Init();
		Cmd_Init();
		Cvar_Init();

		com_dedicated = Cvar_Get("dedicated", "1", CVAR_ROM);

		COM_Init();

		PR_Init();

		SV_InitNet();

		SV_InitLocal();
		Sys_Init();
		PMQH_Init();

		Hunk_AllocName(0, "-HOST_HUNKLEVEL-");
		host_hunklevel = Hunk_LowMark();

		Cbuf_InsertText("exec server.cfg\n");

		host_initialized = true;

		common->Printf("Exe: "__TIME__ " "__DATE__ "\n");
		common->Printf("%4.1f megabyte heap\n",parms->memsize / (1024 * 1024.0));

		common->Printf("\nServer Version %4.2f (Build %04d)\n\n", VERSION, build_number());

		common->Printf("======== QuakeWorld Initialized ========\n");

// process command line arguments
		Cbuf_AddLateCommands();
		Cbuf_Execute();

// if a map wasn't specified on the command line, spawn start.map
		if (sv.state == SS_DEAD)
		{
			Cmd_ExecuteString("map start");
		}
		if (sv.state == SS_DEAD)
		{
			common->Error("Couldn't spawn a server");
		}
	}
	catch (Exception& e)
	{
		Sys_Error("%s", e.What());
	}
}

void SCRQH_BeginLoadingPlaque()
{
}
