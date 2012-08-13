/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// server.h

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../../server/wolfsp/g_public.h"
#include "../../server/server.h"
#include "../../server/tech3/local.h"
#include "../../server/wolfsp/local.h"

//=============================================================================

#define MAX_MASTER_SERVERS  5

extern Cvar* sv_fps;
extern Cvar* sv_rconPassword;
extern Cvar* sv_master[MAX_MASTER_SERVERS];
extern Cvar* sv_showloss;
extern Cvar* sv_killserver;
extern Cvar* sv_mapChecksum;
extern Cvar* sv_serverid;
extern Cvar* sv_allowAnonymous;

// Rafael gameskill
extern Cvar* sv_gameskill;
// done

extern Cvar* sv_reloading;		//----(SA)	added

//===========================================================

//
// sv_main.c
//
void SV_FinalMessage(const char* message);


void SV_AddOperatorCommands(void);
void SV_RemoveOperatorCommands(void);


void SV_MasterHeartbeat(void);
void SV_MasterShutdown(void);




//
// sv_init.c
//
void SV_SpawnServer(char* server, qboolean killBots);
