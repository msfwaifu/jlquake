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

#ifndef _ET_LOCAL_H
#define _ET_LOCAL_H

#include "ETEntity.h"

//
//	Game
//
int SVET_NumForGentity(const etsharedEntity_t* ent);
etsharedEntity_t* SVET_GentityNum(int num);
etplayerState_t* SVET_GameClientNum(int num);
q3svEntity_t* SVET_SvEntityForGentity(const etsharedEntity_t* gEnt);
etsharedEntity_t* SVET_GEntityForSvEntity(const q3svEntity_t* svEnt);
idEntity3* SVET_EntityForGentity(const etsharedEntity_t* gEnt);

bool SVET_BotVisibleFromPos(vec3_t srcorigin, int srcnum, vec3_t destorigin, int destent, bool dummy);
bool SVET_BotCheckAttackAtPos(int entnum, int enemy, vec3_t pos, bool ducking, bool allowHitWorld);

//
//	World
//
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself
void SVET_UnlinkEntity(etsharedEntity_t* ent);
// Needs to be called any time an entity changes origin, mins, maxs,
// or solid.  Automatically unlinks if needed.
// sets ent->v.absmin and ent->v.absmax
// sets ent->leafnums[] for pvs determination even if the entity
// is not solid
void SVET_LinkEntity(etsharedEntity_t* ent);
clipHandle_t SVET_ClipHandleForEntity(const etsharedEntity_t* ent);

#endif