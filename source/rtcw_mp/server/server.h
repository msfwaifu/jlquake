/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// server.h

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../../server/wolfmp/g_public.h"
#include "../../server/server.h"
#include "../../server/tech3/local.h"
#include "../../server/wolfmp/local.h"

//=============================================================================

#define AUTHORIZE_TIMEOUT   5000

//=============================================================================

#define MAX_MASTER_SERVERS  5

extern Cvar* sv_fps;
extern Cvar* sv_rconPassword;
extern Cvar* sv_privatePassword;
extern Cvar* sv_friendlyFire;			// NERVE - SMF
extern Cvar* sv_maxlives;				// NERVE - SMF
extern Cvar* sv_tourney;				// NERVE - SMF

extern Cvar* sv_privateClients;
extern Cvar* sv_master[MAX_MASTER_SERVERS];
extern Cvar* sv_reconnectlimit;
extern Cvar* sv_showloss;
extern Cvar* sv_killserver;
extern Cvar* sv_mapChecksum;
extern Cvar* sv_serverid;
extern Cvar* sv_minPing;
extern Cvar* sv_maxPing;
extern Cvar* sv_floodProtect;
extern Cvar* sv_allowAnonymous;
extern Cvar* sv_onlyVisibleClients;

// Rafael gameskill
extern Cvar* sv_gameskill;
// done

//===========================================================

//
// sv_main.c
//
void SV_FinalMessage(const char* message);


void SV_AddOperatorCommands(void);
void SV_RemoveOperatorCommands(void);


void SV_MasterHeartbeat(const char* hbname);
void SV_MasterShutdown(void);

void SV_MasterGameCompleteStatus();		// NERVE - SMF



//
// sv_init.c
//
void SV_SpawnServer(char* server, qboolean killBots);



//
// sv_client.c
//
void SV_GetChallenge(netadr_t from);

void SV_DirectConnect(netadr_t from);

void SV_AuthorizeIpPacket(netadr_t from);

void SV_ExecuteClientMessage(client_t* cl, QMsg* msg);

void SV_ExecuteClientCommand(client_t* cl, const char* s, bool clientOK, bool preMapRestart);
