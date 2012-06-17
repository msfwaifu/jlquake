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

#include "qwsvdef.h"
#include "../../common/file_formats/bsp29.h"

#define RETURN_EDICT(e) (((int*)pr_globals)[OFS_RETURN] = EDICT_TO_PROG(e))
#define RETURN_STRING(s) (((int*)pr_globals)[OFS_RETURN] = PR_SetString(s))

/*
===============================================================================

                        BUILT-IN FUNCTIONS

===============================================================================
*/

/*
=================
PF_objerror

Dumps out self, then an error message.  The program is aborted and self is
removed, but the level can continue.

objerror(value)
=================
*/
void PF_objerror(void)
{
	const char* s;
	qhedict_t* ed;

	s = PF_VarString(0);
	Con_Printf("======OBJECT ERROR in %s:\n%s\n",
		PR_GetString(pr_xfunction->s_name),s);
	ed = PROG_TO_EDICT(*pr_globalVars.self);
	ED_Print(ed);
	ED_Free(ed);

	SV_Error("Program error");
}



/*
=================
PF_setorigin

This is the only valid way to move an object without using the physics of the world (setting velocity and waiting).  Directly changing origin will not set internal links correctly, so clipping would be messed up.  This should be called when an object is spawned, and then only if it is teleported.

setorigin (entity, origin)
=================
*/
void PF_setorigin(void)
{
	qhedict_t* e;
	float* org;

	e = G_EDICT(OFS_PARM0);
	org = G_VECTOR(OFS_PARM1);
	VectorCopy(org, e->GetOrigin());
	SVQH_LinkEdict(e, false);
}


/*
=================
PF_setsize

the size box is rotated by the current angle

setsize (entity, minvector, maxvector)
=================
*/
void PF_setsize(void)
{
	qhedict_t* e;
	float* min, * max;

	e = G_EDICT(OFS_PARM0);
	min = G_VECTOR(OFS_PARM1);
	max = G_VECTOR(OFS_PARM2);
	e->SetMins(min);
	e->SetMaxs(max);
	VectorSubtract(max, min, e->GetSize());
	SVQH_LinkEdict(e, false);
}


/*
=================
PF_setmodel

setmodel(entity, model)
Also sets size, mins, and maxs for inline bmodels
=================
*/
void PF_setmodel(void)
{
	qhedict_t* e;
	const char* m;
	const char** check;
	int i;
	clipHandle_t mod;

	e = G_EDICT(OFS_PARM0);
	m = G_STRING(OFS_PARM1);

// check to see if model was properly precached
	for (i = 0, check = sv.qh_model_precache; *check; i++, check++)
		if (!String::Cmp(*check, m))
		{
			break;
		}

	if (!*check)
	{
		PR_RunError("no precache: %s\n", m);
	}

	e->SetModel(PR_SetString(m));
	e->v.modelindex = i;

// if it is an inline model, get the size information for it
	if (m[0] == '*')
	{
		mod = CM_InlineModel(String::Atoi(m + 1));
		CM_ModelBounds(mod, e->GetMins(), e->GetMaxs());
		VectorSubtract(e->GetMaxs(), e->GetMins(), e->GetSize());
		SVQH_LinkEdict(e, false);
	}

}

/*
=================
PF_bprint

broadcast print to everyone on server

bprint(value)
=================
*/
void PF_bprint(void)
{
	const char* s;
	int level;

	level = G_FLOAT(OFS_PARM0);

	s = PF_VarString(1);
	SV_BroadcastPrintf(level, "%s", s);
}

/*
=================
PF_sprint

single print to a specific client

sprint(clientent, value)
=================
*/
void PF_sprint(void)
{
	const char* s;
	client_t* client;
	int entnum;
	int level;

	entnum = G_EDICTNUM(OFS_PARM0);
	level = G_FLOAT(OFS_PARM1);

	s = PF_VarString(2);

	if (entnum < 1 || entnum > MAX_CLIENTS_QW)
	{
		Con_Printf("tried to sprint to a non-client\n");
		return;
	}

	client = &svs.clients[entnum - 1];

	SV_ClientPrintf(client, level, "%s", s);
}


/*
=================
PF_centerprint

single print to a specific client

centerprint(clientent, value)
=================
*/
void PF_centerprint(void)
{
	const char* s;
	int entnum;
	client_t* cl;

	entnum = G_EDICTNUM(OFS_PARM0);
	s = PF_VarString(1);

	if (entnum < 1 || entnum > MAX_CLIENTS_QW)
	{
		Con_Printf("tried to sprint to a non-client\n");
		return;
	}

	cl = &svs.clients[entnum - 1];

	ClientReliableWrite_Begin(cl, q1svc_centerprint, 2 + String::Length(s));
	ClientReliableWrite_String(cl, s);
}


/*
=================
PF_ambientsound

=================
*/
void PF_ambientsound(void)
{
	const char** check;
	const char* samp;
	float* pos;
	float vol, attenuation;
	int i, soundnum;

	pos = G_VECTOR(OFS_PARM0);
	samp = G_STRING(OFS_PARM1);
	vol = G_FLOAT(OFS_PARM2);
	attenuation = G_FLOAT(OFS_PARM3);

// check to see if samp was properly precached
	for (soundnum = 0, check = sv.qh_sound_precache; *check; check++, soundnum++)
		if (!String::Cmp(*check,samp))
		{
			break;
		}

	if (!*check)
	{
		Con_Printf("no precache: %s\n", samp);
		return;
	}

// add an svc_spawnambient command to the level signon packet

	sv.qh_signon.WriteByte(q1svc_spawnstaticsound);
	for (i = 0; i < 3; i++)
		sv.qh_signon.WriteCoord(pos[i]);

	sv.qh_signon.WriteByte(soundnum);

	sv.qh_signon.WriteByte(vol * 255);
	sv.qh_signon.WriteByte(attenuation * 64);

}

/*
=================
PF_sound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
allready running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.

=================
*/
void PF_sound(void)
{
	const char* sample;
	int channel;
	qhedict_t* entity;
	int volume;
	float attenuation;

	entity = G_EDICT(OFS_PARM0);
	channel = G_FLOAT(OFS_PARM1);
	sample = G_STRING(OFS_PARM2);
	volume = G_FLOAT(OFS_PARM3) * 255;
	attenuation = G_FLOAT(OFS_PARM4);

	SV_StartSound(entity, channel, sample, volume, attenuation);
}

/*
=================
PF_traceline

Used for use tracing and shot targeting
Traces are blocked by bbox and exact bsp entityes, and also slide box entities
if the tryents flag is set.

traceline (vector1, vector2, tryents)
=================
*/
void PF_traceline(void)
{
	float* v1, * v2;
	q1trace_t trace;
	int nomonsters;
	qhedict_t* ent;

	v1 = G_VECTOR(OFS_PARM0);
	v2 = G_VECTOR(OFS_PARM1);
	nomonsters = G_FLOAT(OFS_PARM2);
	ent = G_EDICT(OFS_PARM3);

	trace = SVQH_Move(v1, vec3_origin, vec3_origin, v2, nomonsters, ent);

	SVQH_SetMoveTrace(trace);
}

/*
=================
PF_checkpos

Returns true if the given entity can move to the given position from it's
current position by walking or rolling.
FIXME: make work...
scalar checkpos (entity, vector)
=================
*/
void PF_checkpos(void)
{
}

//============================================================================

byte checkpvs[BSP29_MAX_MAP_LEAFS / 8];

int PF_newcheckclient(int check)
{
	int i;
	byte* pvs;
	qhedict_t* ent;
	vec3_t org;

// cycle to the next one

	if (check < 1)
	{
		check = 1;
	}
	if (check > MAX_CLIENTS_QW)
	{
		check = MAX_CLIENTS_QW;
	}

	if (check == MAX_CLIENTS_QW)
	{
		i = 1;
	}
	else
	{
		i = check + 1;
	}

	for (;; i++)
	{
		if (i == MAX_CLIENTS_QW + 1)
		{
			i = 1;
		}

		ent = QH_EDICT_NUM(i);

		if (i == check)
		{
			break;	// didn't find anything else

		}
		if (ent->free)
		{
			continue;
		}
		if (ent->GetHealth() <= 0)
		{
			continue;
		}
		if ((int)ent->GetFlags() & QHFL_NOTARGET)
		{
			continue;
		}

		// anything that is a client, or has a client as an enemy
		break;
	}

	// get the PVS for the entity
	VectorAdd(ent->GetOrigin(), ent->GetViewOfs(), org);
	int leaf = CM_PointLeafnum(org);
	pvs = CM_ClusterPVS(CM_LeafCluster(leaf));
	Com_Memcpy(checkpvs, pvs, (CM_NumClusters() + 7) >> 3);

	return i;
}

/*
=================
PF_checkclient

Returns a client (or object that has a client enemy) that would be a
valid target.

If there are more than one valid options, they are cycled each frame

If (self.origin + self.viewofs) is not in the PVS of the current target,
it is not returned at all.

name checkclient ()
=================
*/
#define MAX_CHECK   16
int c_invis, c_notvis;
void PF_checkclient(void)
{
	qhedict_t* ent, * self;
	vec3_t view;

// find a new check if on a new frame
	if (sv.qh_time - sv.qh_lastchecktime >= 0.1)
	{
		sv.qh_lastcheck = PF_newcheckclient(sv.qh_lastcheck);
		sv.qh_lastchecktime = sv.qh_time;
	}

// return check if it might be visible
	ent = QH_EDICT_NUM(sv.qh_lastcheck);
	if (ent->free || ent->GetHealth() <= 0)
	{
		RETURN_EDICT(sv.qh_edicts);
		return;
	}

// if current entity can't possibly see the check entity, return 0
	self = PROG_TO_EDICT(*pr_globalVars.self);
	VectorAdd(self->GetOrigin(), self->GetViewOfs(), view);
	int leaf = CM_PointLeafnum(view);
	int l = CM_LeafCluster(leaf);
	if ((l < 0) || !(checkpvs[l >> 3] & (1 << (l & 7))))
	{
		c_notvis++;
		RETURN_EDICT(sv.qh_edicts);
		return;
	}

// might be able to see it
	c_invis++;
	RETURN_EDICT(ent);
}

//============================================================================


/*
=================
PF_stuffcmd

Sends text over to the client's execution buffer

stuffcmd (clientent, value)
=================
*/
void PF_stuffcmd(void)
{
	int entnum;
	const char* str;
	client_t* cl;

	entnum = G_EDICTNUM(OFS_PARM0);
	if (entnum < 1 || entnum > MAX_CLIENTS_QW)
	{
		PR_RunError("Parm 0 not a client");
	}
	str = G_STRING(OFS_PARM1);

	cl = &svs.clients[entnum - 1];

	if (String::Cmp(str, "disconnect\n") == 0)
	{
		// so long and thanks for all the fish
		cl->qw_drop = true;
		return;
	}

	ClientReliableWrite_Begin(cl, q1svc_stufftext, 2 + String::Length(str));
	ClientReliableWrite_String(cl, str);
}

/*
=================
PF_findradius

Returns a chain of entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
void PF_findradius(void)
{
	qhedict_t* ent, * chain;
	float rad;
	float* org;
	vec3_t eorg;
	int i, j;

	chain = (qhedict_t*)sv.qh_edicts;

	org = G_VECTOR(OFS_PARM0);
	rad = G_FLOAT(OFS_PARM1);

	ent = NEXT_EDICT(sv.qh_edicts);
	for (i = 1; i < sv.qh_num_edicts; i++, ent = NEXT_EDICT(ent))
	{
		if (ent->free)
		{
			continue;
		}
		if (ent->GetSolid() == QHSOLID_NOT)
		{
			continue;
		}
		for (j = 0; j < 3; j++)
			eorg[j] = org[j] - (ent->GetOrigin()[j] + (ent->GetMins()[j] + ent->GetMaxs()[j]) * 0.5);
		if (VectorLength(eorg) > rad)
		{
			continue;
		}

		ent->SetChain(EDICT_TO_PROG(chain));
		chain = ent;
	}

	RETURN_EDICT(chain);
}


void PF_Spawn(void)
{
	qhedict_t* ed;
	ed = ED_Alloc();
	RETURN_EDICT(ed);
}

void PF_Remove(void)
{
	qhedict_t* ed;

	ed = G_EDICT(OFS_PARM0);
	ED_Free(ed);
}


// entity (entity start, .string field, string match) find = #5;
void PF_Find(void)
{
	int e;
	int f;
	const char* s;
	const char* t;
	qhedict_t* ed;

	e = G_EDICTNUM(OFS_PARM0);
	f = G_INT(OFS_PARM1);
	s = G_STRING(OFS_PARM2);
	if (!s)
	{
		PR_RunError("PF_Find: bad search string");
	}

	for (e++; e < sv.qh_num_edicts; e++)
	{
		ed = QH_EDICT_NUM(e);
		if (ed->free)
		{
			continue;
		}
		t = E_STRING(ed,f);
		if (!t)
		{
			continue;
		}
		if (!String::Cmp(t,s))
		{
			RETURN_EDICT(ed);
			return;
		}
	}

	RETURN_EDICT(sv.qh_edicts);
}

void PR_CheckEmptyString(const char* s)
{
	if (s[0] <= ' ')
	{
		PR_RunError("Bad string");
	}
}

void PF_precache_file(void)
{	// precache_file is only used to copy files with qcc, it does nothing
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
}

void PF_precache_sound(void)
{
	const char* s;
	int i;

	if (sv.state != SS_LOADING)
	{
		PR_RunError("PF_Precache_*: Precache can only be done in spawn functions");
	}

	s = G_STRING(OFS_PARM0);
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
	PR_CheckEmptyString(s);

	for (i = 0; i < MAX_SOUNDS_Q1; i++)
	{
		if (!sv.qh_sound_precache[i])
		{
			sv.qh_sound_precache[i] = s;
			return;
		}
		if (!String::Cmp(sv.qh_sound_precache[i], s))
		{
			return;
		}
	}
	PR_RunError("PF_precache_sound: overflow");
}

void PF_precache_model(void)
{
	const char* s;
	int i;

	if (sv.state != SS_LOADING)
	{
		PR_RunError("PF_Precache_*: Precache can only be done in spawn functions");
	}

	s = G_STRING(OFS_PARM0);
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
	PR_CheckEmptyString(s);

	for (i = 0; i < MAX_MODELS_Q1; i++)
	{
		if (!sv.qh_model_precache[i])
		{
			sv.qh_model_precache[i] = s;
			return;
		}
		if (!String::Cmp(sv.qh_model_precache[i], s))
		{
			return;
		}
	}
	PR_RunError("PF_precache_model: overflow");
}


/*
===============
PF_walkmove

float(float yaw, float dist) walkmove
===============
*/
void PF_walkmove(void)
{
	qhedict_t* ent;
	float yaw, dist;
	vec3_t move;
	dfunction_t* oldf;
	int oldself;

	ent = PROG_TO_EDICT(*pr_globalVars.self);
	yaw = G_FLOAT(OFS_PARM0);
	dist = G_FLOAT(OFS_PARM1);

	if (!((int)ent->GetFlags() & (QHFL_ONGROUND | QHFL_FLY | QHFL_SWIM)))
	{
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

	yaw = yaw * M_PI * 2 / 360;

	move[0] = cos(yaw) * dist;
	move[1] = sin(yaw) * dist;
	move[2] = 0;

// save program state, because SVQH_movestep may call other progs
	oldf = pr_xfunction;
	oldself = *pr_globalVars.self;

	G_FLOAT(OFS_RETURN) = SVQH_movestep(ent, move, true, false, false);


// restore program state
	pr_xfunction = oldf;
	*pr_globalVars.self = oldself;
}

/*
===============
PF_droptofloor

void() droptofloor
===============
*/
void PF_droptofloor(void)
{
	qhedict_t* ent;
	vec3_t end;
	q1trace_t trace;

	ent = PROG_TO_EDICT(*pr_globalVars.self);

	VectorCopy(ent->GetOrigin(), end);
	end[2] -= 256;

	trace = SVQH_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), end, false, ent);

	if (trace.fraction == 1 || trace.allsolid)
	{
		G_FLOAT(OFS_RETURN) = 0;
	}
	else
	{
		VectorCopy(trace.endpos, ent->GetOrigin());
		SVQH_LinkEdict(ent, false);
		ent->SetFlags((int)ent->GetFlags() | QHFL_ONGROUND);
		ent->SetGroundEntity(EDICT_TO_PROG(QH_EDICT_NUM(trace.entityNum)));
		G_FLOAT(OFS_RETURN) = 1;
	}
}

/*
===============
PF_lightstyle

void(float style, string value) lightstyle
===============
*/
void PF_lightstyle(void)
{
	int style;
	const char* val;
	client_t* client;
	int j;

	style = G_FLOAT(OFS_PARM0);
	val = G_STRING(OFS_PARM1);

// change the string in sv
	sv.qh_lightstyles[style] = val;

// send message to all clients on this server
	if (sv.state != SS_GAME)
	{
		return;
	}

	for (j = 0, client = svs.clients; j < MAX_CLIENTS_QW; j++, client++)
		if (client->state == CS_ACTIVE)
		{
			ClientReliableWrite_Begin(client, q1svc_lightstyle, String::Length(val) + 3);
			ClientReliableWrite_Char(client, style);
			ClientReliableWrite_String(client, val);
		}
}

/*
=============
PF_checkbottom
=============
*/
void PF_checkbottom(void)
{
	qhedict_t* ent;

	ent = G_EDICT(OFS_PARM0);

	G_FLOAT(OFS_RETURN) = SVQH_CheckBottom(ent);
}

/*
=============
PF_pointcontents
=============
*/
void PF_pointcontents(void)
{
	float* v;

	v = G_VECTOR(OFS_PARM0);

	G_FLOAT(OFS_RETURN) = SVQH_PointContents(v);
}

/*
=============
PF_nextent

entity nextent(entity)
=============
*/
void PF_nextent(void)
{
	int i;
	qhedict_t* ent;

	i = G_EDICTNUM(OFS_PARM0);
	while (1)
	{
		i++;
		if (i == sv.qh_num_edicts)
		{
			RETURN_EDICT(sv.qh_edicts);
			return;
		}
		ent = QH_EDICT_NUM(i);
		if (!ent->free)
		{
			RETURN_EDICT(ent);
			return;
		}
	}
}

/*
=============
PF_aim

Pick a vector for the player to shoot along
vector aim(entity, missilespeed)
=============
*/
Cvar* sv_aim;
void PF_aim(void)
{
	qhedict_t* ent, * check, * bestent;
	vec3_t start, dir, end, bestdir;
	int i, j;
	q1trace_t tr;
	float dist, bestdist;
	float speed;
	const char* noaim;

	ent = G_EDICT(OFS_PARM0);
	speed = G_FLOAT(OFS_PARM1);

	VectorCopy(ent->GetOrigin(), start);
	start[2] += 20;

// noaim option
	i = QH_NUM_FOR_EDICT(ent);
	if (i > 0 && i < MAX_CLIENTS_QW)
	{
		noaim = Info_ValueForKey(svs.clients[i - 1].userinfo, "noaim");
		if (String::Atoi(noaim) > 0)
		{
			VectorCopy(pr_globalVars.v_forward, G_VECTOR(OFS_RETURN));
			return;
		}
	}

// try sending a trace straight
	VectorCopy(pr_globalVars.v_forward, dir);
	VectorMA(start, 2048, dir, end);
	tr = SVQH_Move(start, vec3_origin, vec3_origin, end, false, ent);
	if (tr.entityNum >= 0 && QH_EDICT_NUM(tr.entityNum)->GetTakeDamage() == DAMAGE_AIM &&
		(!teamplay->value || ent->GetTeam() <= 0 || ent->GetTeam() != QH_EDICT_NUM(tr.entityNum)->GetTeam()))
	{
		VectorCopy(pr_globalVars.v_forward, G_VECTOR(OFS_RETURN));
		return;
	}


// try all possible entities
	VectorCopy(dir, bestdir);
	bestdist = sv_aim->value;
	bestent = NULL;

	check = NEXT_EDICT(sv.qh_edicts);
	for (i = 1; i < sv.qh_num_edicts; i++, check = NEXT_EDICT(check))
	{
		if (check->GetTakeDamage() != DAMAGE_AIM)
		{
			continue;
		}
		if (check == ent)
		{
			continue;
		}
		if (teamplay->value && ent->GetTeam() > 0 && ent->GetTeam() == check->GetTeam())
		{
			continue;	// don't aim at teammate
		}
		for (j = 0; j < 3; j++)
			end[j] = check->GetOrigin()[j]
					 + 0.5 * (check->GetMins()[j] + check->GetMaxs()[j]);
		VectorSubtract(end, start, dir);
		VectorNormalize(dir);
		dist = DotProduct(dir, pr_globalVars.v_forward);
		if (dist < bestdist)
		{
			continue;	// to far to turn
		}
		tr = SVQH_Move(start, vec3_origin, vec3_origin, end, false, ent);
		if (QH_EDICT_NUM(tr.entityNum) == check)
		{	// can shoot at this one
			bestdist = dist;
			bestent = check;
		}
	}

	if (bestent)
	{
		VectorSubtract(bestent->GetOrigin(), ent->GetOrigin(), dir);
		dist = DotProduct(dir, pr_globalVars.v_forward);
		VectorScale(pr_globalVars.v_forward, dist, end);
		end[2] = dir[2];
		VectorNormalize(end);
		VectorCopy(end, G_VECTOR(OFS_RETURN));
	}
	else
	{
		VectorCopy(bestdir, G_VECTOR(OFS_RETURN));
	}
}

/*
===============================================================================

MESSAGE WRITING

===============================================================================
*/

#define MSG_BROADCAST   0		// unreliable to all
#define MSG_ONE         1		// reliable to one (msg_entity)
#define MSG_ALL         2		// reliable to all
#define MSG_INIT        3		// write to the init string
#define MSG_MULTICAST   4		// for multicast()

QMsg* WriteDest(void)
{
	int dest;

	dest = G_FLOAT(OFS_PARM0);
	switch (dest)
	{
	case MSG_BROADCAST:
		return &sv.qh_datagram;

	case MSG_ONE:
		SV_Error("Shouldn't be at MSG_ONE");
#if 0
		ent = PROG_TO_EDICT(pr_global_struct->msg_entity);
		entnum = QH_NUM_FOR_EDICT(ent);
		if (entnum < 1 || entnum > MAX_CLIENTS_QW)
		{
			PR_RunError("WriteDest: not a client");
		}
		return &svs.clients[entnum - 1].netchan.message;
#endif

	case MSG_ALL:
		return &sv.qh_reliable_datagram;

	case MSG_INIT:
		if (sv.state != SS_LOADING)
		{
			PR_RunError("PF_Write_*: MSG_INIT can only be written in spawn functions");
		}
		return &sv.qh_signon;

	case MSG_MULTICAST:
		return &sv.multicast;

	default:
		PR_RunError("WriteDest: bad destination");
		break;
	}

	return NULL;
}

static client_t* Write_GetClient(void)
{
	int entnum;
	qhedict_t* ent;

	ent = PROG_TO_EDICT(*pr_globalVars.msg_entity);
	entnum = QH_NUM_FOR_EDICT(ent);
	if (entnum < 1 || entnum > MAX_CLIENTS_QW)
	{
		PR_RunError("WriteDest: not a client");
	}
	return &svs.clients[entnum - 1];
}


void PF_WriteByte(void)
{
	if (G_FLOAT(OFS_PARM0) == MSG_ONE)
	{
		client_t* cl = Write_GetClient();
		ClientReliableCheckBlock(cl, 1);
		ClientReliableWrite_Byte(cl, G_FLOAT(OFS_PARM1));
	}
	else
	{
		WriteDest()->WriteByte(G_FLOAT(OFS_PARM1));
	}
}

void PF_WriteChar()
{
	if (G_FLOAT(OFS_PARM0) == MSG_ONE)
	{
		client_t* cl = Write_GetClient();
		ClientReliableCheckBlock(cl, 1);
		ClientReliableWrite_Char(cl, G_FLOAT(OFS_PARM1));
	}
	else
	{
		WriteDest()->WriteChar(G_FLOAT(OFS_PARM1));
	}
}

void PF_WriteShort(void)
{
	if (G_FLOAT(OFS_PARM0) == MSG_ONE)
	{
		client_t* cl = Write_GetClient();
		ClientReliableCheckBlock(cl, 2);
		ClientReliableWrite_Short(cl, G_FLOAT(OFS_PARM1));
	}
	else
	{
		WriteDest()->WriteShort(G_FLOAT(OFS_PARM1));
	}
}

void PF_WriteLong(void)
{
	if (G_FLOAT(OFS_PARM0) == MSG_ONE)
	{
		client_t* cl = Write_GetClient();
		ClientReliableCheckBlock(cl, 4);
		ClientReliableWrite_Long(cl, G_FLOAT(OFS_PARM1));
	}
	else
	{
		WriteDest()->WriteLong(G_FLOAT(OFS_PARM1));
	}
}

void PF_WriteAngle(void)
{
	if (G_FLOAT(OFS_PARM0) == MSG_ONE)
	{
		client_t* cl = Write_GetClient();
		ClientReliableCheckBlock(cl, 1);
		ClientReliableWrite_Angle(cl, G_FLOAT(OFS_PARM1));
	}
	else
	{
		WriteDest()->WriteAngle(G_FLOAT(OFS_PARM1));
	}
}

void PF_WriteCoord(void)
{
	if (G_FLOAT(OFS_PARM0) == MSG_ONE)
	{
		client_t* cl = Write_GetClient();
		ClientReliableCheckBlock(cl, 2);
		ClientReliableWrite_Coord(cl, G_FLOAT(OFS_PARM1));
	}
	else
	{
		WriteDest()->WriteCoord(G_FLOAT(OFS_PARM1));
	}
}

void PF_WriteString(void)
{
	if (G_FLOAT(OFS_PARM0) == MSG_ONE)
	{
		client_t* cl = Write_GetClient();
		ClientReliableCheckBlock(cl, 1 + String::Length(G_STRING(OFS_PARM1)));
		ClientReliableWrite_String(cl, G_STRING(OFS_PARM1));
	}
	else
	{
		WriteDest()->WriteString2(G_STRING(OFS_PARM1));
	}
}


void PF_WriteEntity(void)
{
	if (G_FLOAT(OFS_PARM0) == MSG_ONE)
	{
		client_t* cl = Write_GetClient();
		ClientReliableCheckBlock(cl, 2);
		ClientReliableWrite_Short(cl, G_EDICTNUM(OFS_PARM1));
	}
	else
	{
		WriteDest()->WriteShort(G_EDICTNUM(OFS_PARM1));
	}
}

//=============================================================================

void PF_makestatic(void)
{
	qhedict_t* ent;
	int i;

	ent = G_EDICT(OFS_PARM0);

	sv.qh_signon.WriteByte(q1svc_spawnstatic);

	sv.qh_signon.WriteByte(SV_ModelIndex(PR_GetString(ent->GetModel())));

	sv.qh_signon.WriteByte(ent->GetFrame());
	sv.qh_signon.WriteByte(ent->GetColorMap());
	sv.qh_signon.WriteByte(ent->GetSkin());
	for (i = 0; i < 3; i++)
	{
		sv.qh_signon.WriteCoord(ent->GetOrigin()[i]);
		sv.qh_signon.WriteAngle(ent->GetAngles()[i]);
	}

// throw the entity away now
	ED_Free(ent);
}

//=============================================================================

/*
==============
PF_setspawnparms
==============
*/
void PF_setspawnparms(void)
{
	qhedict_t* ent;
	int i;
	client_t* client;

	ent = G_EDICT(OFS_PARM0);
	i = QH_NUM_FOR_EDICT(ent);
	if (i < 1 || i > MAX_CLIENTS_QW)
	{
		PR_RunError("Entity is not a client");
	}

	// copy spawn parms out of the client_t
	client = svs.clients + (i - 1);

	for (i = 0; i < NUM_SPAWN_PARMS; i++)
		pr_globalVars.parm1[i] = client->qh_spawn_parms[i];
}

/*
==============
PF_changelevel
==============
*/
void PF_changelevel(void)
{
	const char* s;
	static int last_spawncount;

// make sure we don't issue two changelevels
	if (svs.spawncount == last_spawncount)
	{
		return;
	}
	last_spawncount = svs.spawncount;

	s = G_STRING(OFS_PARM0);
	Cbuf_AddText(va("map %s\n",s));
}


/*
==============
PF_logfrag

logfrag (killer, killee)
==============
*/
void PF_logfrag(void)
{
	qhedict_t* ent1, * ent2;
	int e1, e2;
	char* s;

	ent1 = G_EDICT(OFS_PARM0);
	ent2 = G_EDICT(OFS_PARM1);

	e1 = QH_NUM_FOR_EDICT(ent1);
	e2 = QH_NUM_FOR_EDICT(ent2);

	if (e1 < 1 || e1 > MAX_CLIENTS_QW ||
		e2 < 1 || e2 > MAX_CLIENTS_QW)
	{
		return;
	}

	s = va("\\%s\\%s\\\n",svs.clients[e1 - 1].name, svs.clients[e2 - 1].name);

	svs.qh_log[svs.qh_logsequence & 1].Print(s);
	if (sv_fraglogfile)
	{
		FS_Printf(sv_fraglogfile, s);
		FS_Flush(sv_fraglogfile);
	}
}


/*
==============
PF_infokey

string(entity e, string key) infokey
==============
*/
void PF_infokey(void)
{
	qhedict_t* e;
	int e1;
	const char* value;
	const char* key;
	static char ov[256];

	e = G_EDICT(OFS_PARM0);
	e1 = QH_NUM_FOR_EDICT(e);
	key = G_STRING(OFS_PARM1);

	if (e1 == 0)
	{
		if ((value = Info_ValueForKey(svs.qh_info, key)) == NULL ||
			!*value)
		{
			value = Info_ValueForKey(localinfo, key);
		}
	}
	else if (e1 <= MAX_CLIENTS_QW)
	{
		if (!String::Cmp(key, "ip"))
		{
			String::Cpy(ov, SOCK_BaseAdrToString(svs.clients[e1 - 1].netchan.remoteAddress));
			value = ov;
		}
		else if (!String::Cmp(key, "ping"))
		{
			int ping = SV_CalcPing(&svs.clients[e1 - 1]);
			sprintf(ov, "%d", ping);
			value = ov;
		}
		else
		{
			value = Info_ValueForKey(svs.clients[e1 - 1].userinfo, key);
		}
	}
	else
	{
		value = "";
	}

	RETURN_STRING(value);
}

/*
==============
PF_multicast

void(vector where, float set) multicast
==============
*/
void PF_multicast(void)
{
	float* o;
	int to;

	o = G_VECTOR(OFS_PARM0);
	to = G_FLOAT(OFS_PARM1);

	SV_Multicast(o, to);
}

builtin_t pr_builtin[] =
{
	PF_Fixme,
	PF_makevectors,	// void(entity e)	makevectors         = #1;
	PF_setorigin,	// void(entity e, vector o) setorigin	= #2;
	PF_setmodel,// void(entity e, string m) setmodel	= #3;
	PF_setsize,	// void(entity e, vector min, vector max) setsize = #4;
	PF_Fixme,	// void(entity e, vector min, vector max) setabssize = #5;
	PF_break,	// void() break						= #6;
	PF_random,	// float() random						= #7;
	PF_sound,	// void(entity e, float chan, string samp) sound = #8;
	PF_normalize,	// vector(vector v) normalize			= #9;
	PF_error,	// void(string e) error				= #10;
	PF_objerror,// void(string e) objerror				= #11;
	PF_vlen,// float(vector v) vlen				= #12;
	PF_vectoyaw,// float(vector v) vectoyaw		= #13;
	PF_Spawn,	// entity() spawn						= #14;
	PF_Remove,	// void(entity e) remove				= #15;
	PF_traceline,	// float(vector v1, vector v2, float tryents) traceline = #16;
	PF_checkclient,	// entity() clientlist					= #17;
	PF_Find,// entity(entity start, .string fld, string match) find = #18;
	PF_precache_sound,	// void(string s) precache_sound		= #19;
	PF_precache_model,	// void(string s) precache_model		= #20;
	PF_stuffcmd,// void(entity client, string s)stuffcmd = #21;
	PF_findradius,	// entity(vector org, float rad) findradius = #22;
	PF_bprint,	// void(string s) bprint				= #23;
	PF_sprint,	// void(entity client, string s) sprint = #24;
	PF_dprint,	// void(string s) dprint				= #25;
	PF_ftos,// void(string s) ftos				= #26;
	PF_vtos,// void(string s) vtos				= #27;
	PF_coredump,
	PF_traceon,
	PF_traceoff,
	PF_eprint,	// void(entity e) debug print an entire entity
	PF_walkmove,// float(float yaw, float dist) walkmove
	PF_Fixme,	// float(float yaw, float dist) walkmove
	PF_droptofloor,
	PF_lightstyle,
	PF_rint,
	PF_floor,
	PF_ceil,
	PF_Fixme,
	PF_checkbottom,
	PF_pointcontents,
	PF_Fixme,
	PF_fabs,
	PF_aim,
	PF_cvar,
	PF_localcmd,
	PF_nextent,
	PF_Fixme,
	PF_changeyaw,
	PF_Fixme,
	PF_vectoangles,

	PF_WriteByte,
	PF_WriteChar,
	PF_WriteShort,
	PF_WriteLong,
	PF_WriteCoord,
	PF_WriteAngle,
	PF_WriteString,
	PF_WriteEntity,

	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,
	PF_Fixme,

	SVQH_MoveToGoal,
	PF_precache_file,
	PF_makestatic,

	PF_changelevel,
	PF_Fixme,

	PF_cvar_set,
	PF_centerprint,

	PF_ambientsound,

	PF_precache_model,
	PF_precache_sound,	// precache_sound2 is different only for qcc
	PF_precache_file,

	PF_setspawnparms,

	PF_logfrag,

	PF_infokey,
	PF_stof,
	PF_multicast
};

void PR_InitBuiltins()
{
	pr_builtins = pr_builtin;
	pr_numbuiltins = sizeof(pr_builtin) / sizeof(pr_builtin[0]);
}
