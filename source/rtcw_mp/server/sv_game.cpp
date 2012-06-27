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

// sv_game.c -- interface to the game dll

#include "server.h"

void SV_GameError(const char* string)
{
	Com_Error(ERR_DROP, "%s", string);
}

void SV_GamePrint(const char* string)
{
	Com_Printf("%s", string);
}

/*
===============
SV_GameSendServerCommand

Sends a command string to a client
===============
*/
void SV_GameSendServerCommand(int clientNum, const char* text)
{
	if (clientNum == -1)
	{
		SV_SendServerCommand(NULL, "%s", text);
	}
	else
	{
		if (clientNum < 0 || clientNum >= sv_maxclients->integer)
		{
			return;
		}
		SV_SendServerCommand(svs.clients + clientNum, "%s", text);
	}
}


/*
===============
SV_GameDropClient

Disconnects the client with a message
===============
*/
void SV_GameDropClient(int clientNum, const char* reason)
{
	if (clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		return;
	}
	SV_DropClient(svs.clients + clientNum, reason);
}


/*
=================
SV_SetBrushModel

sets mins and maxs for inline bmodels
=================
*/
void SV_SetBrushModel(wmsharedEntity_t* ent, const char* name)
{
	clipHandle_t h;
	vec3_t mins, maxs;

	if (!name)
	{
		Com_Error(ERR_DROP, "SV_SetBrushModel: NULL");
	}

	if (name[0] != '*')
	{
		Com_Error(ERR_DROP, "SV_SetBrushModel: %s isn't a brush model", name);
	}


	ent->s.modelindex = String::Atoi(name + 1);

	h = CM_InlineModel(ent->s.modelindex);
	CM_ModelBounds(h, mins, maxs);
	VectorCopy(mins, ent->r.mins);
	VectorCopy(maxs, ent->r.maxs);
	ent->r.bmodel = qtrue;

	ent->r.contents = -1;		// we don't know exactly what is in the brushes

	SVWM_LinkEntity(ent);			// FIXME: remove
}



/*
========================
SV_AdjustAreaPortalState
========================
*/
void SV_AdjustAreaPortalState(wmsharedEntity_t* ent, qboolean open)
{
	q3svEntity_t* svEnt;

	svEnt = SVWM_SvEntityForGentity(ent);
	if (svEnt->areanum2 == -1)
	{
		return;
	}
	CM_AdjustAreaPortalState(svEnt->areanum, svEnt->areanum2, open);
}


/*
==================
SV_GameAreaEntities
==================
*/
qboolean    SV_EntityContact(const vec3_t mins, const vec3_t maxs, const wmsharedEntity_t* gEnt, const int capsule)
{
	const float* origin, * angles;
	clipHandle_t ch;
	q3trace_t trace;

	// check for exact collision
	origin = gEnt->r.currentOrigin;
	angles = gEnt->r.currentAngles;

	ch = SVWM_ClipHandleForEntity(gEnt);
	CM_TransformedBoxTraceQ3(&trace, vec3_origin, vec3_origin, mins, maxs,
		ch, -1, origin, angles, capsule);

	return trace.startsolid;
}


/*
===============
SV_GetServerinfo

===============
*/
void SV_GetServerinfo(char* buffer, int bufferSize)
{
	if (bufferSize < 1)
	{
		Com_Error(ERR_DROP, "SV_GetServerinfo: bufferSize == %i", bufferSize);
	}
	String::NCpyZ(buffer, Cvar_InfoString(CVAR_SERVERINFO, MAX_INFO_STRING_Q3), bufferSize);
}

/*
===============
SV_LocateGameData

===============
*/
void SV_LocateGameData(wmsharedEntity_t* gEnts, int numGEntities, int sizeofGEntity_t,
	wmplayerState_t* clients, int sizeofGameClient)
{
	sv.wm_gentities = gEnts;
	sv.q3_gentitySize = sizeofGEntity_t;
	sv.q3_num_entities = numGEntities;

	for (int i = 0; i < MAX_GENTITIES_Q3; i++)
	{
		SVT3_EntityNum(i)->SetGEntity(SVWM_GentityNum(i));
	}

	sv.wm_gameClients = clients;
	sv.q3_gameClientSize = sizeofGameClient;
}


/*
===============
SV_GetUsercmd

===============
*/
void SV_GetUsercmd(int clientNum, wmusercmd_t* cmd)
{
	if (clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "SV_GetUsercmd: bad clientNum:%i", clientNum);
	}
	*cmd = svs.clients[clientNum].wm_lastUsercmd;
}

//==============================================

/*
====================
SV_GameSystemCalls

The module is making a system call
====================
*/
qintptr SV_GameSystemCalls(qintptr* args)
{
	switch (args[0])
	{
//------
	case WMG_LOCATE_GAME_DATA:
		SV_LocateGameData((wmsharedEntity_t*)VMA(1), args[2], args[3], (wmplayerState_t*)VMA(4), args[5]);
		return 0;
	case WMG_DROP_CLIENT:
		SV_GameDropClient(args[1], (char*)VMA(2));
		return 0;
	case WMG_SEND_SERVER_COMMAND:
		SV_GameSendServerCommand(args[1], (char*)VMA(2));
		return 0;
//------
	case WMG_ENTITY_CONTACT:
		return SV_EntityContact((float*)VMA(1), (float*)VMA(2), (wmsharedEntity_t*)VMA(3), /* int capsule */ qfalse);
	case WMG_ENTITY_CONTACTCAPSULE:
		return SV_EntityContact((float*)VMA(1), (float*)VMA(2), (wmsharedEntity_t*)VMA(3), /* int capsule */ qtrue);
//------
	case WMG_SET_BRUSH_MODEL:
		SV_SetBrushModel((wmsharedEntity_t*)VMA(1), (char*)VMA(2));
		return 0;
//------
	case WMG_SET_CONFIGSTRING:
		SV_SetConfigstring(args[1], (char*)VMA(2));
		return 0;
	case WMG_GET_CONFIGSTRING:
		SV_GetConfigstring(args[1], (char*)VMA(2), args[3]);
		return 0;
	case WMG_SET_USERINFO:
		SV_SetUserinfo(args[1], (char*)VMA(2));
		return 0;
	case WMG_GET_USERINFO:
		SV_GetUserinfo(args[1], (char*)VMA(2), args[3]);
		return 0;
	case WMG_GET_SERVERINFO:
		SV_GetServerinfo((char*)VMA(1), args[2]);
		return 0;
	case WMG_ADJUST_AREA_PORTAL_STATE:
		SV_AdjustAreaPortalState((wmsharedEntity_t*)VMA(1), args[2]);
		return 0;
//------
	case WMG_BOT_ALLOCATE_CLIENT:
		return SV_BotAllocateClient();
	case WMG_BOT_FREE_CLIENT:
		SV_BotFreeClient(args[1]);
		return 0;

	case WMG_GET_USERCMD:
		SV_GetUsercmd(args[1], (wmusercmd_t*)VMA(2));
		return 0;
	case WMG_GET_ENTITY_TOKEN:
	{
		const char* s;

		s = String::Parse3(&sv.q3_entityParsePoint);
		String::NCpyZ((char*)VMA(1), s, args[2]);
		if (!sv.q3_entityParsePoint && !s[0])
		{
			return qfalse;
		}
		else
		{
			return qtrue;
		}
	}
//------
	case WMG_REAL_TIME:
		return Com_RealTime((qtime_t*)VMA(1));
	case WMG_SNAPVECTOR:
		Sys_SnapVector((float*)VMA(1));
		return 0;
	case WMG_GETTAG:
		return SV_GetTag(args[1], (char*)VMA(2), (orientation_t*)VMA(3));

	//====================================

	case WMBOTLIB_SETUP:
		return SV_BotLibSetup();
	case WMBOTLIB_SHUTDOWN:
		return SV_BotLibShutdown();
//------
	case WMBOTLIB_GET_SNAPSHOT_ENTITY:
		return SV_BotGetSnapshotEntity(args[1], args[2]);
	case WMBOTLIB_GET_CONSOLE_MESSAGE:
		return SV_BotGetConsoleMessage(args[1], (char*)VMA(2), args[3]);
	case WMBOTLIB_USER_COMMAND:
		SV_ClientThink(&svs.clients[args[1]], (wmusercmd_t*)VMA(2));
		return 0;
//------

	default:
		return SVWM_GameSystemCalls(args);
	}
	return -1;
}

/*
===============
SV_ShutdownGameProgs

Called every time a map changes
===============
*/
void SV_ShutdownGameProgs(void)
{
	if (!gvm)
	{
		return;
	}
	VM_Call(gvm, WMGAME_SHUTDOWN, qfalse);
	VM_Free(gvm);
	gvm = NULL;
}

/*
==================
SV_InitGameVM

Called for both a full init and a restart
==================
*/
static void SV_InitGameVM(qboolean restart)
{
	int i;

	// start the entity parsing at the beginning
	sv.q3_entityParsePoint = CM_EntityString();

	// clear all gentity pointers that might still be set from
	// a previous level
	for (i = 0; i < sv_maxclients->integer; i++)
	{
		svs.clients[i].wm_gentity = NULL;
	}

	// use the current msec count for a random seed
	// init for this gamestate
	VM_Call(gvm, WMGAME_INIT, svs.q3_time, Com_Milliseconds(), restart);
}



/*
===================
SV_RestartGameProgs

Called on a map_restart, but not on a normal map change
===================
*/
void SV_RestartGameProgs(void)
{
	if (!gvm)
	{
		return;
	}
	VM_Call(gvm, WMGAME_SHUTDOWN, qtrue);

	// do a restart instead of a free
	gvm = VM_Restart(gvm);
	if (!gvm)		// bk001212 - as done below
	{
		Com_Error(ERR_FATAL, "VM_Restart on game failed");
	}

	SV_InitGameVM(qtrue);
}


/*
===============
SV_InitGameProgs

Called on a normal map change, not on a map_restart
===============
*/
void SV_InitGameProgs(void)
{

	// load the dll
	gvm = VM_Create("qagame", SV_GameSystemCalls, VMI_NATIVE);
	if (!gvm)
	{
		Com_Error(ERR_FATAL, "VM_Create on game failed");
	}

	SV_InitGameVM(qfalse);
}


/*
====================
SV_GameCommand

See if the current console command is claimed by the game
====================
*/
qboolean SV_GameCommand(void)
{
	if (sv.state != SS_GAME)
	{
		return qfalse;
	}

	return VM_Call(gvm, WMGAME_CONSOLE_COMMAND);
}


/*
====================
SV_SendMoveSpeedsToGame
====================
*/
void SV_SendMoveSpeedsToGame(int entnum, char* text)
{
	if (!gvm)
	{
		return;
	}
	VM_Call(gvm, WMGAME_RETRIEVE_MOVESPEEDS_FROM_CLIENT, entnum, text);
}

/*
====================
SV_GetTag

  return qfalse if unable to retrieve tag information for this client
====================
*/
extern qboolean CL_GetTag(int clientNum, char* tagname, orientation_t* _or);

qboolean SV_GetTag(int clientNum, char* tagname, orientation_t* _or)
{
#ifndef DEDICATED	// TTimo: dedicated only binary defines DEDICATED
	if (com_dedicated->integer)
	{
		return qfalse;
	}

	return CL_GetTag(clientNum, tagname, _or);
#else
	return qfalse;
#endif
}
