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
#include "../../server/public.h"

quakeparms_t host_parms;

qboolean host_initialized;			// true if into command execution (compatability)

double host_frametime;

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
	virtual void ServerDisconnected(const char* format, ...) id_attribute((format(printf, 2, 3)));
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

void idCommonLocal::ServerDisconnected(const char* format, ...)
{
}

//============================================================================

void CL_Disconnect()
{
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

	if (com_errorEntered)
	{
		Sys_Error("SV_Error: recursively entered (%s)", string);
	}

	com_errorEntered = true;

	va_start(argptr,error);
	Q_vsnprintf(string, 1024, error, argptr);
	va_end(argptr);

	common->Printf("SV_Error: %s\n",string);

	SV_Shutdown(va("server crashed: %s\n", string));
	if (sv_logfile)
	{
		FS_FCloseFile(sv_logfile);
		sv_logfile = 0;
	}

	Sys_Error("SV_Error: %s\n",string);
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
COM_ServerFrame

==================
*/
void COM_ServerFrame(float time)
{
	try
	{
// keep the random time dependent
		rand();

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

		SV_Frame(time * 1000);
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

//============================================================================

/*
====================
COM_InitServer
====================
*/
void COM_InitServer(quakeparms_t* parms)
{
	try
	{
		GGameType = GAME_Quake | GAME_QuakeWorld;
		Sys_SetHomePathSuffix("jlquake");

		COM_InitArgv2(parms->argc, parms->argv);
		COM_AddParm("-game");
		COM_AddParm("qw");

		host_parms = *parms;

		Cbuf_Init();
		Cmd_Init();
		Cvar_Init();

		com_dedicated = Cvar_Get("dedicated", "1", CVAR_ROM);

		COM_Init();

		NET_Init();

		SV_InitOperatorCommands();

		SV_Init();
		Sys_Init();
		PMQH_Init();

		Cbuf_InsertText("exec server.cfg\n");

		host_initialized = true;

		common->Printf("Exe: "__TIME__ " "__DATE__ "\n");

		common->Printf("\nServer Version %4.2f (Build %04d)\n\n", VERSION, build_number());

		common->Printf("======== QuakeWorld Initialized ========\n");

// process command line arguments
		Cbuf_AddLateCommands();
		Cbuf_Execute();

// if a map wasn't specified on the command line, spawn start.map
		if (!SV_IsServerActive())
		{
			Cmd_ExecuteString("map start");
		}
		if (!SV_IsServerActive())
		{
			common->Error("Couldn't spawn a server");
		}
	}
	catch (Exception& e)
	{
		Sys_Error("%s", e.What());
	}
}

void CL_EstablishConnection(const char* name)
{
}
void Host_Reconnect_f()
{
}

void CL_MapLoading(void)
{
}
void CL_ShutdownAll(void)
{
}
void CL_Disconnect(qboolean showMainMenu)
{
}
void CL_Drop()
{
}
