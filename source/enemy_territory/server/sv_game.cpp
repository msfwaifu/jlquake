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

// sv_game.c -- interface to the game dll

#include "server.h"

#include "../game/botlib.h"

botlib_export_t* botlib_export;

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
void SV_GameDropClient(int clientNum, const char* reason, int length)
{
	if (clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		return;
	}
	SV_DropClient(svs.clients + clientNum, reason);
	if (length)
	{
		SV_TempBanNetAddress(svs.clients[clientNum].netchan.remoteAddress, length);
	}
}


/*
=================
SV_SetBrushModel

sets mins and maxs for inline bmodels
=================
*/
void SV_SetBrushModel(etsharedEntity_t* ent, const char* name)
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

	SVET_LinkEntity(ent);			// FIXME: remove
}



/*
========================
SV_AdjustAreaPortalState
========================
*/
void SV_AdjustAreaPortalState(etsharedEntity_t* ent, qboolean open)
{
	q3svEntity_t* svEnt;

	svEnt = SVET_SvEntityForGentity(ent);
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
qboolean    SV_EntityContact(const vec3_t mins, const vec3_t maxs, const etsharedEntity_t* gEnt, const int capsule)
{
	const float* origin, * angles;
	clipHandle_t ch;
	q3trace_t trace;

	// check for exact collision
	origin = gEnt->r.currentOrigin;
	angles = gEnt->r.currentAngles;

	ch = SVET_ClipHandleForEntity(gEnt);
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
	String::NCpyZ(buffer, Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE, MAX_INFO_STRING_Q3), bufferSize);
}

/*
===============
SV_LocateGameData

===============
*/
void SV_LocateGameData(etsharedEntity_t* gEnts, int numGEntities, int sizeofGEntity_t,
	etplayerState_t* clients, int sizeofGameClient)
{
	sv.et_gentities = gEnts;
	sv.q3_gentitySize = sizeofGEntity_t;
	sv.q3_num_entities = numGEntities;

	sv.et_gameClients = clients;
	sv.q3_gameClientSize = sizeofGameClient;
}


/*
===============
SV_GetUsercmd

===============
*/
void SV_GetUsercmd(int clientNum, etusercmd_t* cmd)
{
	if (clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "SV_GetUsercmd: bad clientNum:%i", clientNum);
	}
	*cmd = svs.clients[clientNum].et_lastUsercmd;
}

/*
====================
SV_SendBinaryMessage
====================
*/
static void SV_SendBinaryMessage(int cno, char* buf, int buflen)
{
	if (cno < 0 || cno >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "SV_SendBinaryMessage: bad client %i", cno);
		return;
	}

	if (buflen < 0 || buflen > MAX_BINARY_MESSAGE_ET)
	{
		Com_Error(ERR_DROP, "SV_SendBinaryMessage: bad length %i", buflen);
		svs.clients[cno].et_binaryMessageLength = 0;
		return;
	}

	svs.clients[cno].et_binaryMessageLength = buflen;
	memcpy(svs.clients[cno].et_binaryMessage, buf, buflen);
}

/*
====================
SV_BinaryMessageStatus
====================
*/
static int SV_BinaryMessageStatus(int cno)
{
	if (cno < 0 || cno >= sv_maxclients->integer)
	{
		return qfalse;
	}

	if (svs.clients[cno].et_binaryMessageLength == 0)
	{
		return MESSAGE_EMPTY;
	}

	if (svs.clients[cno].et_binaryMessageOverflowed)
	{
		return MESSAGE_WAITING_OVERFLOW;
	}

	return MESSAGE_WAITING;
}

/*
====================
SV_GameBinaryMessageReceived
====================
*/
void SV_GameBinaryMessageReceived(int cno, const char* buf, int buflen, int commandTime)
{
	VM_Call(gvm, GAME_MESSAGERECEIVED, cno, buf, buflen, commandTime);
}

//==============================================

static int  FloatAsInt(float f)
{
	int temp;

	*(float*)&temp = f;

	return temp;
}

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
	case G_PRINT:
		Com_Printf("%s", (char*)VMA(1));
		return 0;
	case G_ERROR:
		Com_Error(ERR_DROP, "%s", (char*)VMA(1));
		return 0;
	case G_MILLISECONDS:
		return Sys_Milliseconds();
	case G_CVAR_REGISTER:
		Cvar_Register((vmCvar_t*)VMA(1), (char*)VMA(2), (char*)VMA(3), args[4]);
		return 0;
	case G_CVAR_UPDATE:
		Cvar_Update((vmCvar_t*)VMA(1));
		return 0;
	case G_CVAR_SET:
		Cvar_Set((const char*)VMA(1), (const char*)VMA(2));
		return 0;
	case G_CVAR_VARIABLE_INTEGER_VALUE:
		return Cvar_VariableIntegerValue((const char*)VMA(1));
	case G_CVAR_VARIABLE_STRING_BUFFER:
		Cvar_VariableStringBuffer((char*)VMA(1), (char*)VMA(2), args[3]);
		return 0;
	case G_CVAR_LATCHEDVARIABLESTRINGBUFFER:
		Cvar_LatchedVariableStringBuffer((char*)VMA(1), (char*)VMA(2), args[3]);
		return 0;
	case G_ARGC:
		return Cmd_Argc();
	case G_ARGV:
		Cmd_ArgvBuffer(args[1], (char*)VMA(2), args[3]);
		return 0;
	case G_SEND_CONSOLE_COMMAND:
		Cbuf_ExecuteText(args[1], (char*)VMA(2));
		return 0;

	case G_FS_FOPEN_FILE:
		return FS_FOpenFileByMode((char*)VMA(1), (fileHandle_t*)VMA(2), (fsMode_t)args[3]);
	case G_FS_READ:
		FS_Read(VMA(1), args[2], args[3]);
		return 0;
	case G_FS_WRITE:
		return FS_Write(VMA(1), args[2], args[3]);
	case G_FS_RENAME:
		FS_Rename((char*)VMA(1), (char*)VMA(2));
		return 0;
	case G_FS_FCLOSE_FILE:
		FS_FCloseFile(args[1]);
		return 0;
	case G_FS_GETFILELIST:
		return FS_GetFileList((char*)VMA(1), (char*)VMA(2), (char*)VMA(3), args[4]);

	case G_LOCATE_GAME_DATA:
		SV_LocateGameData((etsharedEntity_t*)VMA(1), args[2], args[3], (etplayerState_t*)VMA(4), args[5]);
		return 0;
	case G_DROP_CLIENT:
		SV_GameDropClient(args[1], (char*)VMA(2), args[3]);
		return 0;
	case G_SEND_SERVER_COMMAND:
		SV_GameSendServerCommand(args[1], (char*)VMA(2));
		return 0;
	case G_LINKENTITY:
		SVET_LinkEntity((etsharedEntity_t*)VMA(1));
		return 0;
	case G_UNLINKENTITY:
		SVET_UnlinkEntity((etsharedEntity_t*)VMA(1));
		return 0;
	case G_ENTITIES_IN_BOX:
		return SVT3_AreaEntities((float*)VMA(1), (float*)VMA(2), (int*)VMA(3), args[4]);
	case G_ENTITY_CONTACT:
		return SV_EntityContact((float*)VMA(1), (float*)VMA(2), (etsharedEntity_t*)VMA(3), /* int capsule */ qfalse);
	case G_ENTITY_CONTACTCAPSULE:
		return SV_EntityContact((float*)VMA(1), (float*)VMA(2), (etsharedEntity_t*)VMA(3), /* int capsule */ qtrue);
	case G_TRACE:
		SVT3_Trace((q3trace_t*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4), (float*)VMA(5), args[6], args[7], /* int capsule */ qfalse);
		return 0;
	case G_TRACECAPSULE:
		SVT3_Trace((q3trace_t*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4), (float*)VMA(5), args[6], args[7], /* int capsule */ qtrue);
		return 0;
	case G_POINT_CONTENTS:
		return SVT3_PointContents((float*)VMA(1), args[2]);
	case G_SET_BRUSH_MODEL:
		SV_SetBrushModel((etsharedEntity_t*)VMA(1), (char*)VMA(2));
		return 0;
	case G_IN_PVS:
		return SVT3_inPVS((float*)VMA(1), (float*)VMA(2));
	case G_IN_PVS_IGNORE_PORTALS:
		return SVT3_inPVSIgnorePortals((float*)VMA(1), (float*)VMA(2));

	case G_SET_CONFIGSTRING:
		SV_SetConfigstring(args[1], (char*)VMA(2));
		return 0;
	case G_GET_CONFIGSTRING:
		SV_GetConfigstring(args[1], (char*)VMA(2), args[3]);
		return 0;
	case G_SET_USERINFO:
		SV_SetUserinfo(args[1], (char*)VMA(2));
		return 0;
	case G_GET_USERINFO:
		SV_GetUserinfo(args[1], (char*)VMA(2), args[3]);
		return 0;
	case G_GET_SERVERINFO:
		SV_GetServerinfo((char*)VMA(1), args[2]);
		return 0;
	case G_ADJUST_AREA_PORTAL_STATE:
		SV_AdjustAreaPortalState((etsharedEntity_t*)VMA(1), args[2]);
		return 0;
	case G_AREAS_CONNECTED:
		return CM_AreasConnected(args[1], args[2]);

	case G_BOT_ALLOCATE_CLIENT:
		return SV_BotAllocateClient(args[1]);
	case G_BOT_FREE_CLIENT:
		SV_BotFreeClient(args[1]);
		return 0;

	case G_GET_USERCMD:
		SV_GetUsercmd(args[1], (etusercmd_t*)VMA(2));
		return 0;
	case G_GET_ENTITY_TOKEN:
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

	case G_DEBUG_POLYGON_CREATE:
		return BotImport_DebugPolygonCreate(args[1], args[2], (vec3_t*)VMA(3));
	case G_DEBUG_POLYGON_DELETE:
		BotImport_DebugPolygonDelete(args[1]);
		return 0;
	case G_REAL_TIME:
		return Com_RealTime((qtime_t*)VMA(1));
	case G_SNAPVECTOR:
		Sys_SnapVector((float*)VMA(1));
		return 0;
	case G_GETTAG:
		return SV_GetTag(args[1], args[2], (char*)VMA(3), (orientation_t*)VMA(4));

	case G_REGISTERTAG:
		return SV_LoadTag((char*)VMA(1));

	//	Not used by actual game code.
	case G_REGISTERSOUND:
		common->Error("RegisterSound on server");
	case G_GET_SOUND_LENGTH:
		common->Error("GetSoundLength on server");

	//====================================

	case BOTLIB_SETUP:
		return SV_BotLibSetup();
	case BOTLIB_SHUTDOWN:
		return SV_BotLibShutdown();
	case BOTLIB_LIBVAR_SET:
		return BotLibVarSet((char*)VMA(1), (char*)VMA(2));
	case BOTLIB_LIBVAR_GET:
		return BotLibVarGet((char*)VMA(1), (char*)VMA(2), args[3]);

	case BOTLIB_PC_ADD_GLOBAL_DEFINE:
		return PC_AddGlobalDefine((char*)VMA(1));
	case BOTLIB_PC_LOAD_SOURCE:
		return PC_LoadSourceHandle((char*)VMA(1));
	case BOTLIB_PC_FREE_SOURCE:
		return PC_FreeSourceHandle(args[1]);
	case BOTLIB_PC_READ_TOKEN:
		return PC_ReadTokenHandleET(args[1], (etpc_token_t*)VMA(2));
	case BOTLIB_PC_SOURCE_FILE_AND_LINE:
		return PC_SourceFileAndLine(args[1], (char*)VMA(2), (int*)VMA(3));
	case BOTLIB_PC_UNREAD_TOKEN:
		PC_UnreadLastTokenHandle(args[1]);
		return 0;

	case BOTLIB_START_FRAME:
		return botlib_export->BotLibStartFrame(VMF(1));
	case BOTLIB_LOAD_MAP:
		return botlib_export->BotLibLoadMap((char*)VMA(1));
	case BOTLIB_UPDATENTITY:
		return BotLibUpdateEntity(args[1], (bot_entitystate_t*)VMA(2));
	case BOTLIB_TEST:
		return 0;

	case BOTLIB_GET_SNAPSHOT_ENTITY:
		return SV_BotGetSnapshotEntity(args[1], args[2]);
	case BOTLIB_GET_CONSOLE_MESSAGE:
		return SV_BotGetConsoleMessage(args[1], (char*)VMA(2), args[3]);
	case BOTLIB_USER_COMMAND:
		SV_ClientThink(&svs.clients[args[1]], (etusercmd_t*)VMA(2));
		return 0;

	case BOTLIB_AAS_ENTITY_INFO:
		AAS_EntityInfo(args[1], (aas_entityinfo_t*)VMA(2));
		return 0;

	case BOTLIB_AAS_INITIALIZED:
		return AAS_Initialized();
	case BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX:
		AAS_PresenceTypeBoundingBox(args[1], (float*)VMA(2), (float*)VMA(3));
		return 0;
	case BOTLIB_AAS_TIME:
		return FloatAsInt(AAS_Time());

	// Ridah
	case BOTLIB_AAS_SETCURRENTWORLD:
		AAS_SetCurrentWorld(args[1]);
		return 0;
	// done.

	case BOTLIB_AAS_POINT_AREA_NUM:
		return AAS_PointAreaNum((float*)VMA(1));
	case BOTLIB_AAS_TRACE_AREAS:
		return AAS_TraceAreas((float*)VMA(1), (float*)VMA(2), (int*)VMA(3), (vec3_t*)VMA(4), args[5]);
	case BOTLIB_AAS_BBOX_AREAS:
		return AAS_BBoxAreas((float*)VMA(1), (float*)VMA(2), (int*)VMA(3), args[4]);
	case BOTLIB_AAS_AREA_CENTER:
		AAS_AreaCenter(args[1], (float*)VMA(2));
		return 0;
	case BOTLIB_AAS_AREA_WAYPOINT:
		return AAS_AreaWaypoint(args[1], (float*)VMA(2));

	case BOTLIB_AAS_POINT_CONTENTS:
		return AAS_PointContents((float*)VMA(1));
	case BOTLIB_AAS_NEXT_BSP_ENTITY:
		return AAS_NextBSPEntity(args[1]);
	case BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY:
		return AAS_ValueForBSPEpairKey(args[1], (char*)VMA(2), (char*)VMA(3), args[4]);
	case BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY:
		return AAS_VectorForBSPEpairKey(args[1], (char*)VMA(2), (float*)VMA(3));
	case BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY:
		return AAS_FloatForBSPEpairKey(args[1], (char*)VMA(2), (float*)VMA(3));
	case BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY:
		return AAS_IntForBSPEpairKey(args[1], (char*)VMA(2), (int*)VMA(3));

	case BOTLIB_AAS_AREA_REACHABILITY:
		return AAS_AreaReachability(args[1]);
	case BOTLIB_AAS_AREA_LADDER:
		return AAS_AreaLadder(args[1]);

	case BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA:
		return AAS_AreaTravelTimeToGoalArea(args[1], (float*)VMA(2), args[3], args[4]);

	case BOTLIB_AAS_SWIMMING:
		return AAS_Swimming((float*)VMA(1));
	case BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT:
		return AAS_PredictClientMovementET((aas_clientmove_et_t*)VMA(1), args[2], (float*)VMA(3), args[4], args[5],
			(float*)VMA(6), (float*)VMA(7), args[8], args[9], VMF(10), args[11], args[12], args[13]);

	// Ridah, route-tables
	case BOTLIB_AAS_RT_SHOWROUTE:
		return 0;

	//case BOTLIB_AAS_RT_GETHIDEPOS:
	//	return botlib_export->aas.AAS_RT_GetHidePos( VMA(1), args[2], args[3], VMA(4), args[5], args[6], VMA(7) );

	//case BOTLIB_AAS_FINDATTACKSPOTWITHINRANGE:
	//	return botlib_export->aas.AAS_FindAttackSpotWithinRange( args[1], args[2], args[3], VMF(4), args[5], VMA(6) );

	case BOTLIB_AAS_NEARESTHIDEAREA:
		return botlib_export->aas.AAS_NearestHideArea(args[1], (float*)VMA(2), args[3], args[4], (float*)VMA(5), args[6], args[7], VMF(8), (float*)VMA(9));

	case BOTLIB_AAS_LISTAREASINRANGE:
		return AAS_ListAreasInRange((float*)VMA(1), args[2], VMF(3), args[4], (vec3_t*)VMA(5), args[6]);

	case BOTLIB_AAS_AVOIDDANGERAREA:
		return AAS_AvoidDangerArea((float*)VMA(1), args[2], (float*)VMA(3), args[4], VMF(5), args[6]);

	case BOTLIB_AAS_RETREAT:
		return AAS_Retreat((int*)VMA(1), args[2], (float*)VMA(3), args[4], (float*)VMA(5), args[6], VMF(7), VMF(8), args[9]);

	case BOTLIB_AAS_ALTROUTEGOALS:
		return AAS_AlternativeRouteGoalsET((float*)VMA(1), (float*)VMA(2), args[3], (aas_altroutegoal_t*)VMA(4), args[5], args[6]);

	case BOTLIB_AAS_SETAASBLOCKINGENTITY:
		AAS_SetAASBlockingEntity((float*)VMA(1), (float*)VMA(2), args[3]);
		return 0;

	case BOTLIB_AAS_RECORDTEAMDEATHAREA:
		return 0;

	case BOTLIB_EA_SAY:
		botlib_export->ea.EA_Say(args[1], (char*)VMA(2));
		return 0;
	case BOTLIB_EA_SAY_TEAM:
		botlib_export->ea.EA_SayTeam(args[1], (char*)VMA(2));
		return 0;
	case BOTLIB_EA_USE_ITEM:
		botlib_export->ea.EA_UseItem(args[1], (char*)VMA(2));
		return 0;
	case BOTLIB_EA_DROP_ITEM:
		botlib_export->ea.EA_DropItem(args[1], (char*)VMA(2));
		return 0;
	case BOTLIB_EA_USE_INV:
		botlib_export->ea.EA_UseInv(args[1], (char*)VMA(2));
		return 0;
	case BOTLIB_EA_DROP_INV:
		botlib_export->ea.EA_DropInv(args[1], (char*)VMA(2));
		return 0;
	case BOTLIB_EA_GESTURE:
		EA_Gesture(args[1]);
		return 0;
	case BOTLIB_EA_COMMAND:
		botlib_export->ea.EA_Command(args[1], (char*)VMA(2));
		return 0;

	case BOTLIB_EA_SELECT_WEAPON:
		EA_SelectWeapon(args[1], args[2]);
		return 0;
	case BOTLIB_EA_TALK:
		EA_Talk(args[1]);
		return 0;
	case BOTLIB_EA_ATTACK:
		EA_Attack(args[1]);
		return 0;
	case BOTLIB_EA_RELOAD:
		EA_Reload(args[1]);
		return 0;
	case BOTLIB_EA_USE:
		EA_Use(args[1]);
		return 0;
	case BOTLIB_EA_RESPAWN:
		EA_Respawn(args[1]);
		return 0;
	case BOTLIB_EA_JUMP:
		EA_Jump(args[1]);
		return 0;
	case BOTLIB_EA_DELAYED_JUMP:
		EA_DelayedJump(args[1]);
		return 0;
	case BOTLIB_EA_CROUCH:
		EA_Crouch(args[1]);
		return 0;
	case BOTLIB_EA_WALK:
		EA_Walk(args[1]);
		return 0;
	case BOTLIB_EA_MOVE_UP:
		EA_MoveUp(args[1]);
		return 0;
	case BOTLIB_EA_MOVE_DOWN:
		EA_MoveDown(args[1]);
		return 0;
	case BOTLIB_EA_MOVE_FORWARD:
		EA_MoveForward(args[1]);
		return 0;
	case BOTLIB_EA_MOVE_BACK:
		EA_MoveBack(args[1]);
		return 0;
	case BOTLIB_EA_MOVE_LEFT:
		EA_MoveLeft(args[1]);
		return 0;
	case BOTLIB_EA_MOVE_RIGHT:
		EA_MoveRight(args[1]);
		return 0;
	case BOTLIB_EA_MOVE:
		EA_Move(args[1], (float*)VMA(2), VMF(3));
		return 0;
	case BOTLIB_EA_VIEW:
		EA_View(args[1], (float*)VMA(2));
		return 0;
	case BOTLIB_EA_PRONE:
		EA_Prone(args[1]);
		return 0;

	case BOTLIB_EA_END_REGULAR:
		return 0;
	case BOTLIB_EA_GET_INPUT:
		EA_GetInput(args[1], VMF(2), (bot_input_t*)VMA(3));
		return 0;
	case BOTLIB_EA_RESET_INPUT:
		EA_ResetInputWolf(args[1], (bot_input_t*)VMA(2));
		return 0;

	case BOTLIB_AI_LOAD_CHARACTER:
		return BotLoadCharacter((char*)VMA(1), args[2]);
	case BOTLIB_AI_FREE_CHARACTER:
		BotFreeCharacter(args[1]);
		return 0;
	case BOTLIB_AI_CHARACTERISTIC_FLOAT:
		return FloatAsInt(Characteristic_Float(args[1], args[2]));
	case BOTLIB_AI_CHARACTERISTIC_BFLOAT:
		return FloatAsInt(Characteristic_BFloat(args[1], args[2], VMF(3), VMF(4)));
	case BOTLIB_AI_CHARACTERISTIC_INTEGER:
		return Characteristic_Integer(args[1], args[2]);
	case BOTLIB_AI_CHARACTERISTIC_BINTEGER:
		return Characteristic_BInteger(args[1], args[2], args[3], args[4]);
	case BOTLIB_AI_CHARACTERISTIC_STRING:
		Characteristic_String(args[1], args[2], (char*)VMA(3), args[4]);
		return 0;

	case BOTLIB_AI_ALLOC_CHAT_STATE:
		return BotAllocChatState();
	case BOTLIB_AI_FREE_CHAT_STATE:
		BotFreeChatState(args[1]);
		return 0;
	case BOTLIB_AI_QUEUE_CONSOLE_MESSAGE:
		BotQueueConsoleMessage(args[1], args[2], (char*)VMA(3));
		return 0;
	case BOTLIB_AI_REMOVE_CONSOLE_MESSAGE:
		BotRemoveConsoleMessage(args[1], args[2]);
		return 0;
	case BOTLIB_AI_NEXT_CONSOLE_MESSAGE:
		return BotNextConsoleMessageWolf(args[1], (struct bot_consolemessage_wolf_t*)VMA(2));
	case BOTLIB_AI_NUM_CONSOLE_MESSAGE:
		return BotNumConsoleMessages(args[1]);
	case BOTLIB_AI_INITIAL_CHAT:
		BotInitialChat(args[1], (char*)VMA(2), args[3], (char*)VMA(4), (char*)VMA(5), (char*)VMA(6), (char*)VMA(7), (char*)VMA(8), (char*)VMA(9), (char*)VMA(10), (char*)VMA(11));
		return 0;
	case BOTLIB_AI_NUM_INITIAL_CHATS:
		return BotNumInitialChats(args[1], (char*)VMA(2));
	case BOTLIB_AI_REPLY_CHAT:
		return BotReplyChat(args[1], (char*)VMA(2), args[3], args[4], (char*)VMA(5), (char*)VMA(6), (char*)VMA(7), (char*)VMA(8), (char*)VMA(9), (char*)VMA(10), (char*)VMA(11), (char*)VMA(12));
	case BOTLIB_AI_CHAT_LENGTH:
		return BotChatLength(args[1]);
	case BOTLIB_AI_ENTER_CHAT:
		botlib_export->ai.BotEnterChat(args[1], args[2], args[3]);
		return 0;
	case BOTLIB_AI_GET_CHAT_MESSAGE:
		BotGetChatMessage(args[1], (char*)VMA(2), args[3]);
		return 0;
	case BOTLIB_AI_STRING_CONTAINS:
		return StringContains((char*)VMA(1), (char*)VMA(2), args[3]);
	case BOTLIB_AI_FIND_MATCH:
		return BotFindMatchWolf((char*)VMA(1), (bot_match_wolf_t*)VMA(2), args[3]);
	case BOTLIB_AI_MATCH_VARIABLE:
		BotMatchVariableWolf((bot_match_wolf_t*)VMA(1), args[2], (char*)VMA(3), args[4]);
		return 0;
	case BOTLIB_AI_UNIFY_WHITE_SPACES:
		UnifyWhiteSpaces((char*)VMA(1));
		return 0;
	case BOTLIB_AI_REPLACE_SYNONYMS:
		BotReplaceSynonyms((char*)VMA(1), args[2]);
		return 0;
	case BOTLIB_AI_LOAD_CHAT_FILE:
		return BotLoadChatFile(args[1], (char*)VMA(2), (char*)VMA(3));
	case BOTLIB_AI_SET_CHAT_GENDER:
		BotSetChatGender(args[1], args[2]);
		return 0;
	case BOTLIB_AI_SET_CHAT_NAME:
		BotSetChatName(args[1], (char*)VMA(2), 0);
		return 0;

	case BOTLIB_AI_RESET_GOAL_STATE:
		BotResetGoalState(args[1]);
		return 0;
	case BOTLIB_AI_RESET_AVOID_GOALS:
		BotResetAvoidGoals(args[1]);
		return 0;
	case BOTLIB_AI_REMOVE_FROM_AVOID_GOALS:
		BotRemoveFromAvoidGoals(args[1], args[2]);
		return 0;
	case BOTLIB_AI_PUSH_GOAL:
		BotPushGoalET(args[1], (struct bot_goal_et_t*)VMA(2));
		return 0;
	case BOTLIB_AI_POP_GOAL:
		BotPopGoal(args[1]);
		return 0;
	case BOTLIB_AI_EMPTY_GOAL_STACK:
		BotEmptyGoalStack(args[1]);
		return 0;
	case BOTLIB_AI_DUMP_AVOID_GOALS:
		BotDumpAvoidGoals(args[1]);
		return 0;
	case BOTLIB_AI_DUMP_GOAL_STACK:
		BotDumpGoalStack(args[1]);
		return 0;
	case BOTLIB_AI_GOAL_NAME:
		BotGoalName(args[1], (char*)VMA(2), args[3]);
		return 0;
	case BOTLIB_AI_GET_TOP_GOAL:
		return BotGetTopGoalET(args[1], (struct bot_goal_et_t*)VMA(2));
	case BOTLIB_AI_GET_SECOND_GOAL:
		return BotGetSecondGoalET(args[1], (struct bot_goal_et_t*)VMA(2));
	case BOTLIB_AI_CHOOSE_LTG_ITEM:
		return BotChooseLTGItem(args[1], (float*)VMA(2), (int*)VMA(3), args[4]);
	case BOTLIB_AI_CHOOSE_NBG_ITEM:
		return BotChooseNBGItemET(args[1], (float*)VMA(2), (int*)VMA(3), args[4], (struct bot_goal_et_t*)VMA(5), VMF(6));
	case BOTLIB_AI_TOUCHING_GOAL:
		return BotTouchingGoalET((float*)VMA(1), (struct bot_goal_et_t*)VMA(2));
	case BOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE:
		return BotItemGoalInVisButNotVisibleET(args[1], (float*)VMA(2), (float*)VMA(3), (struct bot_goal_et_t*)VMA(4));
	case BOTLIB_AI_GET_LEVEL_ITEM_GOAL:
		return BotGetLevelItemGoalET(args[1], (char*)VMA(2), (struct bot_goal_et_t*)VMA(3));
	case BOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL:
		return BotGetNextCampSpotGoalET(args[1], (struct bot_goal_et_t*)VMA(2));
	case BOTLIB_AI_GET_MAP_LOCATION_GOAL:
		return BotGetMapLocationGoalET((char*)VMA(1), (struct bot_goal_et_t*)VMA(2));
	case BOTLIB_AI_AVOID_GOAL_TIME:
		return FloatAsInt(BotAvoidGoalTime(args[1], args[2]));
	case BOTLIB_AI_INIT_LEVEL_ITEMS:
		BotInitLevelItems();
		return 0;
	case BOTLIB_AI_UPDATE_ENTITY_ITEMS:
		BotUpdateEntityItems();
		return 0;
	case BOTLIB_AI_LOAD_ITEM_WEIGHTS:
		return BotLoadItemWeights(args[1], (char*)VMA(2));
	case BOTLIB_AI_FREE_ITEM_WEIGHTS:
		BotFreeItemWeights(args[1]);
		return 0;
	case BOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC:
		BotInterbreedGoalFuzzyLogic(args[1], args[2], args[3]);
		return 0;
	case BOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC:
		return 0;
	case BOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC:
		BotMutateGoalFuzzyLogic(args[1], VMF(2));
		return 0;
	case BOTLIB_AI_ALLOC_GOAL_STATE:
		return BotAllocGoalState(args[1]);
	case BOTLIB_AI_FREE_GOAL_STATE:
		BotFreeGoalState(args[1]);
		return 0;

	case BOTLIB_AI_RESET_MOVE_STATE:
		BotResetMoveState(args[1]);
		return 0;
	case BOTLIB_AI_MOVE_TO_GOAL:
		botlib_export->ai.BotMoveToGoal((struct bot_moveresult_t*)VMA(1), args[2], (struct bot_goal_et_t*)VMA(3), args[4]);
		return 0;
	case BOTLIB_AI_MOVE_IN_DIRECTION:
		return BotMoveInDirection(args[1], (float*)VMA(2), VMF(3), args[4]);
	case BOTLIB_AI_RESET_AVOID_REACH:
		BotResetAvoidReachAndMove(args[1]);
		return 0;
	case BOTLIB_AI_RESET_LAST_AVOID_REACH:
		BotResetLastAvoidReach(args[1]);
		return 0;
	case BOTLIB_AI_REACHABILITY_AREA:
		return BotReachabilityArea((float*)VMA(1), args[2]);
	case BOTLIB_AI_MOVEMENT_VIEW_TARGET:
		return BotMovementViewTargetET(args[1], (struct bot_goal_et_t*)VMA(2), args[3], VMF(4), (float*)VMA(5));
	case BOTLIB_AI_PREDICT_VISIBLE_POSITION:
		return BotPredictVisiblePositionET((float*)VMA(1), args[2], (struct bot_goal_et_t*)VMA(3), args[4], (float*)VMA(5));
	case BOTLIB_AI_ALLOC_MOVE_STATE:
		return BotAllocMoveState();
	case BOTLIB_AI_FREE_MOVE_STATE:
		BotFreeMoveState(args[1]);
		return 0;
	case BOTLIB_AI_INIT_MOVE_STATE:
		BotInitMoveStateET(args[1], (struct bot_initmove_et_t*)VMA(2));
		return 0;
	// Ridah
	case BOTLIB_AI_INIT_AVOID_REACH:
		BotResetAvoidReach(args[1]);
		return 0;
	// done.

	case BOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON:
		return BotChooseBestFightWeapon(args[1], (int*)VMA(2));
	case BOTLIB_AI_GET_WEAPON_INFO:
		BotGetWeaponInfo(args[1], args[2], (weaponinfo_t*)VMA(3));
		return 0;
	case BOTLIB_AI_LOAD_WEAPON_WEIGHTS:
		return BotLoadWeaponWeights(args[1], (char*)VMA(2));
	case BOTLIB_AI_ALLOC_WEAPON_STATE:
		return BotAllocWeaponState();
	case BOTLIB_AI_FREE_WEAPON_STATE:
		BotFreeWeaponState(args[1]);
		return 0;
	case BOTLIB_AI_RESET_WEAPON_STATE:
		BotResetWeaponState(args[1]);
		return 0;

	case BOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION:
		return GeneticParentsAndChildSelection(args[1], (float*)VMA(2), (int*)VMA(3), (int*)VMA(4), (int*)VMA(5));

	case TRAP_MEMSET:
		memset(VMA(1), args[2], args[3]);
		return 0;

	case TRAP_MEMCPY:
		memcpy(VMA(1), VMA(2), args[3]);
		return 0;

	case TRAP_STRNCPY:
		String::NCpy((char*)VMA(1), (char*)VMA(2), args[3]);
		return args[1];

	case TRAP_SIN:
		return FloatAsInt(sin(VMF(1)));

	case TRAP_COS:
		return FloatAsInt(cos(VMF(1)));

	case TRAP_ATAN2:
		return FloatAsInt(atan2(VMF(1), VMF(2)));

	case TRAP_SQRT:
		return FloatAsInt(sqrt(VMF(1)));

	case TRAP_MATRIXMULTIPLY:
		MatrixMultiply((vec3_t*)VMA(1), (vec3_t*)VMA(2), (vec3_t*)VMA(3));
		return 0;

	case TRAP_ANGLEVECTORS:
		AngleVectors((float*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4));
		return 0;

	case TRAP_PERPENDICULARVECTOR:
		PerpendicularVector((float*)VMA(1), (float*)VMA(2));
		return 0;

	case TRAP_FLOOR:
		return FloatAsInt(floor(VMF(1)));

	case TRAP_CEIL:
		return FloatAsInt(ceil(VMF(1)));

	case PB_STAT_REPORT:
		return 0;

	case G_SENDMESSAGE:
		SV_SendBinaryMessage(args[1], (char*)VMA(2), args[3]);
		return 0;
	case G_MESSAGESTATUS:
		return SV_BinaryMessageStatus(args[1]);

	default:
		Com_Error(ERR_DROP, "Bad game system trap: %i", (int)args[0]);
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
	VM_Call(gvm, GAME_SHUTDOWN, qfalse);
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
		svs.clients[i].et_gentity = NULL;
	}

	// use the current msec count for a random seed
	// init for this gamestate
	VM_Call(gvm, GAME_INIT, svs.q3_time, Com_Milliseconds(), restart);
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
	VM_Call(gvm, GAME_SHUTDOWN, qtrue);

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
	sv.et_num_tagheaders = 0;
	sv.et_num_tags = 0;

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

	return VM_Call(gvm, GAME_CONSOLE_COMMAND);
}

/*
====================
SV_GameIsSinglePlayer
====================
*/
qboolean SV_GameIsSinglePlayer(void)
{
	return(com_gameInfo.spGameTypes & (1 << g_gameType->integer));
}

/*
====================
SV_GameIsCoop

    This is a modified SinglePlayer, no savegame capability for example
====================
*/
qboolean SV_GameIsCoop(void)
{
	return(com_gameInfo.coopGameTypes & (1 << g_gameType->integer));
}

/*
====================
SV_GetTag

  return qfalse if unable to retrieve tag information for this client
====================
*/
extern qboolean CL_GetTag(int clientNum, char* tagname, orientation_t* _or);

qboolean SV_GetTag(int clientNum, int tagFileNumber, char* tagname, orientation_t* _or)
{
	int i;

	if (tagFileNumber > 0 && tagFileNumber <= sv.et_num_tagheaders)
	{
		for (i = sv.et_tagHeadersExt[tagFileNumber - 1].start; i < sv.et_tagHeadersExt[tagFileNumber - 1].start + sv.et_tagHeadersExt[tagFileNumber - 1].count; i++)
		{
			if (!String::ICmp(sv.et_tags[i].name, tagname))
			{
				VectorCopy(sv.et_tags[i].origin, _or->origin);
				VectorCopy(sv.et_tags[i].axis[0], _or->axis[0]);
				VectorCopy(sv.et_tags[i].axis[1], _or->axis[1]);
				VectorCopy(sv.et_tags[i].axis[2], _or->axis[2]);
				return qtrue;
			}
		}
	}

	// Gordon: lets try and remove the inconsitancy between ded/non-ded servers...
	// Gordon: bleh, some code in clientthink_real really relies on this working on player models...
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
