/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cmd.c -- Quake script command processing module

#include "quakedef.h"

QCvar* cl_warncmd;

/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/

#ifndef SERVERONLY		// FIXME
/*
===================
Cmd_ForwardToServer

adds the current command line as a clc_stringcmd to the client message.
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void Cmd_ForwardToServer (void)
{
	if (cls.state == ca_disconnected)
	{
		Con_Printf ("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}
	
	if (cls.demoplayback)
		return;		// not really connected

	cls.netchan.message.WriteByte(clc_stringcmd);
	cls.netchan.message.Print(Cmd_Argv(0));
	if (Cmd_Argc() > 1)
	{
		cls.netchan.message.Print(" ");
		cls.netchan.message.Print(Cmd_ArgsUnmodified());
	}
}

// don't forward the first argument
void Cmd_ForwardToServer_f (void)
{
	if (cls.state == ca_disconnected)
	{
		Con_Printf ("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}

	if (String::ICmp(Cmd_Argv(1), "snap") == 0) {
		Cbuf_InsertText ("snap\n");
		return;
	}
	
	if (cls.demoplayback)
		return;		// not really connected

	if (Cmd_Argc() > 1)
	{
		cls.netchan.message.WriteByte(clc_stringcmd);
		cls.netchan.message.Print(Cmd_ArgsUnmodified());
	}
}
#else
void Cmd_ForwardToServer (void)
{
}
#endif

bool Cmd_HandleNullCommand(const char* text)
{
	Cmd_ForwardToServer();
    return true;
}

void Cmd_HandleUnknownCommand()
{
    if (cl_warncmd->value || developer->value)
		Con_Printf ("Unknown command \"%s\"\n", Cmd_Argv(0));
}

/*
============
Cmd_Init
============
*/
void Cmd_Init (void)
{
	Cmd_SharedInit();
#ifndef SERVERONLY
	Cmd_AddCommand ("cmd", Cmd_ForwardToServer_f);
#endif

	cl_warncmd = Cvar_Get("cl_warncmd", "0", 0);
}

