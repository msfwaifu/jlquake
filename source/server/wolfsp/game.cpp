//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../server.h"
#include "../tech3/local.h"
#include "local.h"
#include "g_public.h"

// these functions must be used instead of pointer arithmetic, because
// the game allocates gentities with private information after the server shared part
int SVWS_NumForGentity(const wssharedEntity_t* ent)
{
	return ((byte*)ent - (byte*)sv.ws_gentities) / sv.q3_gentitySize;
}

wssharedEntity_t* SVWS_GentityNum(int num)
{
	return (wssharedEntity_t*)((byte*)sv.ws_gentities + sv.q3_gentitySize * num);
}

wsplayerState_t* SVWS_GameClientNum(int num)
{
	return (wsplayerState_t*)((byte*)sv.ws_gameClients + sv.q3_gameClientSize * (num));
}

q3svEntity_t* SVWS_SvEntityForGentity(const wssharedEntity_t* gEnt)
{
	if (!gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES_Q3)
	{
		common->Error("SVWS_SvEntityForGentity: bad gEnt");
	}
	return &sv.q3_svEntities[gEnt->s.number];
}

idEntity3* SVWS_EntityForGentity(const wssharedEntity_t* gEnt)
{
	if (!gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES_Q3)
	{
		common->Error("SVWS_SvEntityForGentity: bad gEnt");
	}
	return sv.q3_entities[gEnt->s.number];
}

wssharedEntity_t* SVWS_GEntityForSvEntity(const q3svEntity_t* svEnt)
{
	int num = svEnt - sv.q3_svEntities;
	return SVWS_GentityNum(num);
}

void SVWS_UnlinkEntity(wssharedEntity_t* gEnt)
{
	SVT3_UnlinkEntity(SVWS_EntityForGentity(gEnt), SVWS_SvEntityForGentity(gEnt));
}

void SVWS_LinkEntity(wssharedEntity_t* gEnt)
{
	SVT3_LinkEntity(SVWS_EntityForGentity(gEnt), SVWS_SvEntityForGentity(gEnt));
}

//	Returns a headnode that can be used for testing or clipping to a
// given entity.  If the entity is a bsp model, the headnode will
// be returned, otherwise a custom box tree will be constructed.
clipHandle_t SVWS_ClipHandleForEntity(const wssharedEntity_t* gent)
{
	return SVT3_ClipHandleForEntity(SVWS_EntityForGentity(gent));
}

bool SVWS_BotVisibleFromPos(vec3_t srcpos, int srcnum, vec3_t destpos, int destnum, bool updateVisPos)
{
	return VM_Call(gvm, WSAICAST_VISIBLEFROMPOS, (qintptr)srcpos, srcnum, (qintptr)destpos, destnum, updateVisPos);
}

bool SVWS_BotCheckAttackAtPos(int entnum, int enemy, vec3_t pos, bool ducking, bool allowHitWorld)
{
	return VM_Call(gvm, WSAICAST_CHECKATTACKATPOS, entnum, enemy, (qintptr)pos, ducking, allowHitWorld);
}