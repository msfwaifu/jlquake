/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// sv_bot.c

#include "server.h"
#include "../game/botlib.h"

extern botlib_export_t* botlib_export;
int bot_enable;

/*
==================
SV_BotAllocateClient
==================
*/
int SV_BotAllocateClient(int clientNum)
{
	int i;
	client_t* cl;

	// Arnout: added possibility to request a clientnum
	if (clientNum > 0)
	{
		if (clientNum >= sv_maxclients->integer)
		{
			return -1;
		}

		cl = &svs.clients[clientNum];
		if (cl->state != CS_FREE)
		{
			return -1;
		}
		else
		{
			i = clientNum;
		}
	}
	else
	{
		// find a client slot
		for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++)
		{
			// Wolfenstein, never use the first slot, otherwise if a bot connects before the first client on a listen server, game won't start
			if (i < 1)
			{
				continue;
			}
			// done.
			if (cl->state == CS_FREE)
			{
				break;
			}
		}
	}

	if (i == sv_maxclients->integer)
	{
		return -1;
	}

	cl->et_gentity = SVET_GentityNum(i);
	cl->et_gentity->s.number = i;
	cl->state = CS_ACTIVE;
	cl->q3_lastPacketTime = svs.q3_time;
	cl->netchan.remoteAddress.type = NA_BOT;
	cl->rate = 16384;

	return i;
}

/*
==================
SV_BotFreeClient
==================
*/
void SV_BotFreeClient(int clientNum)
{
	client_t* cl;

	if (clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "SV_BotFreeClient: bad clientNum: %i", clientNum);
	}
	cl = &svs.clients[clientNum];
	cl->state = CS_FREE;
	cl->name[0] = 0;
	if (cl->et_gentity)
	{
		cl->et_gentity->r.svFlags &= ~Q3SVF_BOT;
	}
}

/*
==================
BotDrawDebugPolygons
==================
*/
void BotDrawDebugPolygons(void (* drawPoly)(int color, int numPoints, float* points), int value)
{
	static Cvar* bot_debug, * bot_groundonly, * bot_reachability, * bot_highlightarea;
	static Cvar* bot_testhidepos;
	bot_debugpoly_t* poly;
	int i;
	static Cvar* debugSurface;

#ifdef PRE_RELEASE_DEMO
	return;
#endif

	if (!bot_enable)
	{
		return;
	}
	//bot debugging
	if (!bot_debug)
	{
		bot_debug = Cvar_Get("bot_debug", "0", 0);
	}
	//show reachabilities
	if (!bot_reachability)
	{
		bot_reachability = Cvar_Get("bot_reachability", "0", 0);
	}
	//show ground faces only
	if (!bot_groundonly)
	{
		bot_groundonly = Cvar_Get("bot_groundonly", "1", 0);
	}
	//get the hightlight area
	if (!bot_highlightarea)
	{
		bot_highlightarea = Cvar_Get("bot_highlightarea", "0", 0);
	}
	//
	if (!bot_testhidepos)
	{
		bot_testhidepos = Cvar_Get("bot_testhidepos", "0", 0);
	}
	//
	if (!debugSurface)
	{
		debugSurface = Cvar_Get("r_debugSurface", "0", 0);
	}
	//
	if (bot_debug->integer == 1 || bot_debug->integer == 9)
	{
		BotLibVarSet("bot_highlightarea", bot_highlightarea->string);
		BotLibVarSet("bot_testhidepos", bot_testhidepos->string);
		BotLibVarSet("bot_debug", bot_debug->string);
	}	//end if
	for (i = 0; i < bot_maxdebugpolys; i++)
	{
		poly = &debugpolygons[i];
		if (!poly->inuse)
		{
			continue;
		}
		drawPoly(poly->color, poly->numPoints, (float*)poly->points);
		//Com_Printf("poly %i, numpoints = %d\n", i, poly->numPoints);
	}
}

/*
==================
BotImport_inPVS
==================
*/
int BotImport_inPVS(vec3_t p1, vec3_t p2)
{
	return SV_inPVS(p1, p2);
}

/*
==================
BotImport_BotVisibleFromPos
==================
*/
qboolean BotImport_BotVisibleFromPos(vec3_t srcorigin, int srcnum, vec3_t destorigin, int destent, qboolean dummy)
{
	return VM_Call(gvm, BOT_VISIBLEFROMPOS, srcorigin, srcnum, destorigin, destent, dummy);
}

/*
==================
BotImport_BotCheckAttackAtPos
==================
*/
qboolean BotImport_BotCheckAttackAtPos(int entnum, int enemy, vec3_t pos, qboolean ducking, qboolean allowHitWorld)
{
	return VM_Call(gvm, BOT_CHECKATTACKATPOS, entnum, enemy, pos, ducking, allowHitWorld);
}

/*
==================
SV_BotClientCommand
==================
*/
void BotClientCommand(int client, const char* command)
{
	SV_ExecuteClientCommand(&svs.clients[client], command, qtrue, qfalse);
}

/*
==================
SV_BotFrame
==================
*/
void SV_BotFrame(int time)
{

#ifdef PRE_RELEASE_DEMO
	return;
#endif

	if (!bot_enable)
	{
		return;
	}
	//NOTE: maybe the game is already shutdown
	if (!gvm)
	{
		return;
	}
	VM_Call(gvm, BOTAI_START_FRAME, time);
}

/*
===============
SV_BotLibSetup
===============
*/
int SV_BotLibSetup(void)
{
	static Cvar* bot_norcd;
	static Cvar* bot_frameroutingupdates;

#ifdef PRE_RELEASE_DEMO
	return 0;
#endif

	if (!bot_enable)
	{
		return 0;
	}

	if (!botlib_export)
	{
		Com_Printf(S_COLOR_RED "Error: SV_BotLibSetup without SV_BotInitBotLib\n");
		return -1;
	}

	// RF, set RCD calculation status
	bot_norcd = Cvar_Get("bot_norcd", "0", 0);
	BotLibVarSet("bot_norcd", bot_norcd->string);

	// RF, set AAS routing max per frame
	if (SV_GameIsSinglePlayer())
	{
		bot_frameroutingupdates = Cvar_Get("bot_frameroutingupdates", "9999999", 0);
	}
	else		// more restrictive in multiplayer
	{
		bot_frameroutingupdates = Cvar_Get("bot_frameroutingupdates", "1000", 0);
	}
	BotLibVarSet("bot_frameroutingupdates", bot_frameroutingupdates->string);

// START	Arnout changes, 28-08-2002.
// added single player
	return BotLibSetup((SV_GameIsSinglePlayer() || SV_GameIsCoop()));
// END	Arnout changes, 28-08-2002.
}

/*
===============
SV_ShutdownBotLib

Called when either the entire server is being killed, or
it is changing to a different game directory.
===============
*/
int SV_BotLibShutdown(void)
{

	if (!botlib_export)
	{
		return -1;
	}

	return BotLibShutdown();
}

/*
==================
SV_BotInitCvars
==================
*/
void SV_BotInitCvars(void)
{

	Cvar_Get("bot_enable", "0", 0);					//enable the bot
	Cvar_Get("bot_developer", "0", 0);				//bot developer mode
	Cvar_Get("bot_debug", "0", 0);					//enable bot debugging
	Cvar_Get("bot_groundonly", "1", 0);				//only show ground faces of areas
	Cvar_Get("bot_reachability", "0", 0);		//show all reachabilities to other areas
	Cvar_Get("bot_thinktime", "50", 0);			//msec the bots thinks
	Cvar_Get("bot_reloadcharacters", "0", 0);	//reload the bot characters each time
	Cvar_Get("bot_testichat", "0", 0);				//test ichats
	Cvar_Get("bot_testrchat", "0", 0);				//test rchats
	Cvar_Get("bot_fastchat", "0", 0);			//fast chatting bots
	Cvar_Get("bot_nochat", "1", 0);					//disable chats
	Cvar_Get("bot_grapple", "0", 0);			//enable grapple
	Cvar_Get("bot_rocketjump", "0", 0);				//enable rocket jumping
	Cvar_Get("bot_norcd", "0", 0);					//enable creation of RCD file

	bot_enable = Cvar_VariableIntegerValue("bot_enable");
}

/*
==================
SV_BotInitBotLib
==================
*/
void SV_BotInitBotLib(void)
{
	botlib_import_t botlib_import;

	if (debugpolygons)
	{
		Z_Free(debugpolygons);
	}
	bot_maxdebugpolys = 128;
	debugpolygons = (bot_debugpoly_t*)Z_Malloc(sizeof(bot_debugpoly_t) * bot_maxdebugpolys);

	botlib_import.inPVS = BotImport_inPVS;
	botlib_import.BotClientCommand = BotClientCommand;

	//bot routines
	botlib_import.BotVisibleFromPos =   BotImport_BotVisibleFromPos;
	botlib_import.BotCheckAttackAtPos = BotImport_BotCheckAttackAtPos;

	// singleplayer check
	// Arnout: no need for this
	//botlib_import.BotGameIsSinglePlayer = SV_GameIsSinglePlayer;

	botlib_export = (botlib_export_t*)GetBotLibAPI(BOTLIB_API_VERSION, &botlib_import);
}


//
//  * * * BOT AI CODE IS BELOW THIS POINT * * *
//

/*
==================
SV_BotGetConsoleMessage
==================
*/
int SV_BotGetConsoleMessage(int client, char* buf, int size)
{
	client_t* cl;
	int index;

	cl = &svs.clients[client];
	cl->q3_lastPacketTime = svs.q3_time;

	if (cl->q3_reliableAcknowledge == cl->q3_reliableSequence)
	{
		return qfalse;
	}

	cl->q3_reliableAcknowledge++;
	index = cl->q3_reliableAcknowledge & (MAX_RELIABLE_COMMANDS_ET - 1);

	if (!cl->q3_reliableCommands[index][0])
	{
		return qfalse;
	}

	//String::NCpyZ( buf, cl->reliableCommands[index], size );
	return qtrue;
}

#if 0
/*
==================
EntityInPVS
==================
*/
int EntityInPVS(int client, int entityNum)
{
	client_t* cl;
	q3clientSnapshot_t* frame;
	int i;

	cl = &svs.clients[client];
	frame = &cl->frames[cl->netchan.outgoingSequence & PACKET_MASK_Q3];
	for (i = 0; i < frame->num_entities; i++)
	{
		if (svs.snapshotEntities[(frame->first_entity + i) % svs.q3_numSnapshotEntities].number == entityNum)
		{
			return qtrue;
		}
	}
	return qfalse;
}
#endif

/*
==================
SV_BotGetSnapshotEntity
==================
*/
int SV_BotGetSnapshotEntity(int client, int sequence)
{
	client_t* cl;
	q3clientSnapshot_t* frame;

	cl = &svs.clients[client];
	frame = &cl->q3_frames[cl->netchan.outgoingSequence & PACKET_MASK_Q3];
	if (sequence < 0 || sequence >= frame->num_entities)
	{
		return -1;
	}
	return svs.et_snapshotEntities[(frame->first_entity + sequence) % svs.q3_numSnapshotEntities].number;
}
