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
// sys_sun.h -- Sun system driver

#include "quakedef.h"
#include "../common/system_unix.h"
#include "errno.h"
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>

qboolean isDedicated;

void Sys_Error(const char* error, ...)
{
	va_list argptr;

	if (ttycon_on)
	{
		tty_Hide();
	}

	printf("Sys_Error: ");
	va_start(argptr,error);
	vprintf(error,argptr);
	va_end(argptr);
	printf("\n");
	Sys_ConsoleInputShutdown();
	Host_Shutdown();
	exit(1);
}

void Sys_Quit(void)
{
	Sys_ConsoleInputShutdown();
	Host_Shutdown();
	exit(0);
}

void Sys_Init(void)
{
}

//=============================================================================

int main(int argc, char** argv)
{
	static quakeparms_t parms;
	float time, oldtime, newtime;

	parms.memsize = 16 * 1024 * 1024;
	parms.membase = malloc(parms.memsize);
	parms.basedir = ".";

	COM_InitArgv2(argc, argv);

	parms.argc = c;
	parms.argv = v;

	printf("Host_Init\n");
	Host_Init(&parms);

	Sys_Init();

	// unroll the simulation loop to give the video side a chance to see _vid_default_mode
	Host_Frame(0.1);
	VID_SetDefaultMode();

	Sys_ConsoleInputInit();

	oldtime = Sys_DoubleTime();
	while (1)
	{
		newtime = Sys_DoubleTime();
		Host_Frame(newtime - oldtime);
		oldtime = newtime;
	}
	return 0;
}
