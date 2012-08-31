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
// quakedef.h -- primary header for client

#include "../../client/client.h"
#include "../../client/game/quake/local.h"

//define	PARANOID			// speed sapping error checking


#include <setjmp.h>
#include <time.h>

#include "bothdefs.h"

#include "common.h"
#include "vid.h"
#include "sys.h"
#include "draw.h"
#include "screen.h"
#include "net.h"
#include "protocol.h"
#include "cmd.h"
#include "render.h"
#include "client.h"
#include "keys.h"
#include "console.h"
#include "view.h"
#include "menu.h"

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

//=============================================================================

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use

typedef struct
{
	const char* basedir;
	int argc;
	char** argv;
} quakeparms_t;


//=============================================================================

//
// host
//
extern quakeparms_t host_parms;

extern qboolean host_initialized;			// true if into command execution
extern double host_frametime;
extern int host_framecount;				// incremented every frame, never reset
extern double realtime;					// not bounded in any way, changed at
										// start of every frame, never reset

void Host_InitCommands(void);
void Host_Init(quakeparms_t* parms);
void Host_Shutdown(void);
void Host_FatalError(const char* error, ...);
void Host_EndGame(const char* message, ...);
qboolean Host_SimulationTime(float time);
void Host_Frame(float time);
void Host_Quit_f(void);

extern qboolean msg_suppress_1;			// suppresses resolution and cache size console output
										//  an fullscreen DIB focus gain/loss
