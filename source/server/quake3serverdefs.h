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

enum
{
	Q3GT_FFA,				// free for all
	Q3GT_SINGLE_PLAYER = 2,	// single player ffa
	//-- team games go after this --
	Q3GT_TEAM,				// team deathmatch
};

enum
{
	Q3ET_PLAYER = 1,
	Q3ET_ITEM = 2,
	Q3ET_MOVER = 4,

	WSET_PROP = 32,
};

#define MAX_DOWNLOAD_WINDOW         8		// max of eight download frames
#define MAX_DOWNLOAD_BLKSIZE        2048	// 2048 byte block chunks

struct q3clientSnapshot_t
{
	int areabytes;
	byte areabits[MAX_MAP_AREA_BYTES];					// portalarea visibility bits
	q3playerState_t q3_ps;
	wsplayerState_t ws_ps;
	wmplayerState_t wm_ps;
	etplayerState_t et_ps;
	int num_entities;
	int first_entity;					// into the circular sv_packet_entities[]
										// the entities MUST be in increasing state number
										// order, otherwise the delta compression will fail
	int messageSent;					// time the message was transmitted
	int messageAcked;					// time the message was acked
	int messageSize;					// used to rate drop packets
};

struct netchan_buffer_t
{
	QMsg msg;
	byte msgBuffer[MAX_MSGLEN];
	char lastClientCommandString[MAX_STRING_CHARS];
	netchan_buffer_t* next;
};

struct q3entityShared_t
{
	q3entityState_t s;				// communicated by server to clients

	qboolean linked;				// qfalse if not in any good cluster
	int linkcount;

	int svFlags;					// SVF_NOCLIENT, SVF_BROADCAST, etc

	// only send to this client when SVF_SINGLECLIENT is set
	// if SVF_CLIENTMASK is set, use bitmask for clients to send to (maxclients must be <= 32, up to the mod to enforce this)
	int singleClient;

	qboolean bmodel;				// if false, assume an explicit mins / maxs bounding box
									// only set by trap_SetBrushModel
	vec3_t mins, maxs;
	int contents;					// CONTENTS_TRIGGER, BSP46CONTENTS_SOLID, BSP46CONTENTS_BODY, etc
									// a non-solid entity should set to 0

	vec3_t absmin, absmax;			// derived from mins/maxs and origin + rotation

	// currentOrigin will be used for all collision detection and world linking.
	// it will not necessarily be the same as the trajectory evaluation for the current
	// time, because each entity must be moved one at a time after time is advanced
	// to avoid simultanious collision issues
	vec3_t currentOrigin;
	vec3_t currentAngles;

	// when a trace call is made and passEntityNum != Q3ENTITYNUM_NONE,
	// an ent will be excluded from testing if:
	// ent->s.number == passEntityNum	(don't interact with self)
	// ent->s.ownerNum = passEntityNum	(don't interact with your own missiles)
	// entity[ent->s.ownerNum].ownerNum = passEntityNum	(don't interact with other missiles from owner)
	int ownerNum;
};

// the server looks at a sharedEntity, which is the start of the game's gentity_t structure
struct q3sharedEntity_t
{
	q3entityState_t s;				// communicated by server to clients
	q3entityShared_t r;				// shared by both the server system and game
};
