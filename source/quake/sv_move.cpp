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
// sv_move.c -- monster movement

#include "quakedef.h"

/*
=============
SV_CheckBottom

Returns false if any part of the bottom of the entity is off an edge that
is not a staircase.

=============
*/
int c_yes, c_no;

qboolean SV_CheckBottom(qhedict_t* ent)
{
	vec3_t mins, maxs, start, stop;
	q1trace_t trace;
	int x, y;
	float mid, bottom;

	VectorAdd(ent->GetOrigin(), ent->GetMins(), mins);
	VectorAdd(ent->GetOrigin(), ent->GetMaxs(), maxs);

// if all of the points under the corners are solid world, don't bother
// with the tougher checks
// the corners must be within 16 of the midpoint
	start[2] = mins[2] - 1;
	for (x = 0; x <= 1; x++)
		for (y = 0; y <= 1; y++)
		{
			start[0] = x ? maxs[0] : mins[0];
			start[1] = y ? maxs[1] : mins[1];
			if (SV_PointContents(start) != BSP29CONTENTS_SOLID)
			{
				goto realcheck;
			}
		}

	c_yes++;
	return true;		// we got out easy

realcheck:
	c_no++;
//
// check it for real...
//
	start[2] = mins[2];

// the midpoint must be within 16 of the bottom
	start[0] = stop[0] = (mins[0] + maxs[0]) * 0.5;
	start[1] = stop[1] = (mins[1] + maxs[1]) * 0.5;
	stop[2] = start[2] - 2 * STEPSIZE;
	trace = SV_Move(start, vec3_origin, vec3_origin, stop, true, ent);

	if (trace.fraction == 1.0)
	{
		return false;
	}
	mid = bottom = trace.endpos[2];

// the corners must be within 16 of the midpoint
	for (x = 0; x <= 1; x++)
		for (y = 0; y <= 1; y++)
		{
			start[0] = stop[0] = x ? maxs[0] : mins[0];
			start[1] = stop[1] = y ? maxs[1] : mins[1];

			trace = SV_Move(start, vec3_origin, vec3_origin, stop, true, ent);

			if (trace.fraction != 1.0 && trace.endpos[2] > bottom)
			{
				bottom = trace.endpos[2];
			}
			if (trace.fraction == 1.0 || mid - trace.endpos[2] > STEPSIZE)
			{
				return false;
			}
		}

	c_yes++;
	return true;
}


/*
=============
SV_movestep

Called by monster program code.
The move will be adjusted for slopes and stairs, but if the move isn't
possible, no move is done, false is returned, and
pr_global_struct->trace_normal is set to the normal of the blocking wall
=============
*/
qboolean SV_movestep(qhedict_t* ent, vec3_t move, qboolean relink)
{
	float dz;
	vec3_t oldorg, neworg, end;
	q1trace_t trace;
	int i;
	qhedict_t* enemy;

// try the move
	VectorCopy(ent->GetOrigin(), oldorg);
	VectorAdd(ent->GetOrigin(), move, neworg);

// flying monsters don't step up
	if ((int)ent->GetFlags() & (FL_SWIM | FL_FLY))
	{
		// try one move with vertical motion, then one without
		for (i = 0; i < 2; i++)
		{
			VectorAdd(ent->GetOrigin(), move, neworg);
			enemy = PROG_TO_EDICT(ent->GetEnemy());
			if (i == 0 && enemy != sv.edicts)
			{
				dz = ent->GetOrigin()[2] - PROG_TO_EDICT(ent->GetEnemy())->GetOrigin()[2];
				if (dz > 40)
				{
					neworg[2] -= 8;
				}
				if (dz < 30)
				{
					neworg[2] += 8;
				}
			}
			trace = SV_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), neworg, false, ent);

			if (trace.fraction == 1)
			{
				if (((int)ent->GetFlags() & FL_SWIM) && SV_PointContents(trace.endpos) == BSP29CONTENTS_EMPTY)
				{
					return false;	// swim monster left water

				}
				VectorCopy(trace.endpos, ent->GetOrigin());
				if (relink)
				{
					SV_LinkEdict(ent, true);
				}
				return true;
			}

			if (enemy == sv.edicts)
			{
				break;
			}
		}

		return false;
	}

// push down from a step height above the wished position
	neworg[2] += STEPSIZE;
	VectorCopy(neworg, end);
	end[2] -= STEPSIZE * 2;

	trace = SV_Move(neworg, ent->GetMins(), ent->GetMaxs(), end, false, ent);

	if (trace.allsolid)
	{
		return false;
	}

	if (trace.startsolid)
	{
		neworg[2] -= STEPSIZE;
		trace = SV_Move(neworg, ent->GetMins(), ent->GetMaxs(), end, false, ent);
		if (trace.allsolid || trace.startsolid)
		{
			return false;
		}
	}
	if (trace.fraction == 1)
	{
		// if monster had the ground pulled out, go ahead and fall
		if ((int)ent->GetFlags() & FL_PARTIALGROUND)
		{
			VectorAdd(ent->GetOrigin(), move, ent->GetOrigin());
			if (relink)
			{
				SV_LinkEdict(ent, true);
			}
			ent->SetFlags((int)ent->GetFlags() & ~FL_ONGROUND);
//	Con_Printf ("fall down\n");
			return true;
		}

		return false;		// walked off an edge
	}

// check point traces down for dangling corners
	VectorCopy(trace.endpos, ent->GetOrigin());

	if (!SV_CheckBottom(ent))
	{
		if ((int)ent->GetFlags() & FL_PARTIALGROUND)
		{	// entity had floor mostly pulled out from underneath it
			// and is trying to correct
			if (relink)
			{
				SV_LinkEdict(ent, true);
			}
			return true;
		}
		VectorCopy(oldorg, ent->GetOrigin());
		return false;
	}

	if ((int)ent->GetFlags() & FL_PARTIALGROUND)
	{
//		Con_Printf ("back on ground\n");
		ent->SetFlags((int)ent->GetFlags() & ~FL_PARTIALGROUND);
	}
	ent->SetGroundEntity(EDICT_TO_PROG(EDICT_NUM(trace.entityNum)));

// the move is ok
	if (relink)
	{
		SV_LinkEdict(ent, true);
	}
	return true;
}


//============================================================================

/*
======================
SV_StepDirection

Turns to the movement direction, and walks the current distance if
facing it.

======================
*/
void PF_changeyaw(void);
qboolean SV_StepDirection(qhedict_t* ent, float yaw, float dist)
{
	vec3_t move, oldorigin;
	float delta;

	ent->SetIdealYaw(yaw);
	PF_changeyaw();

	yaw = yaw * M_PI * 2 / 360;
	move[0] = cos(yaw) * dist;
	move[1] = sin(yaw) * dist;
	move[2] = 0;

	VectorCopy(ent->GetOrigin(), oldorigin);
	if (SV_movestep(ent, move, false))
	{
		delta = ent->GetAngles()[YAW] - ent->GetIdealYaw();
		if (delta > 45 && delta < 315)
		{		// not turned far enough, so don't take the step
			VectorCopy(oldorigin, ent->GetOrigin());
		}
		SV_LinkEdict(ent, true);
		return true;
	}
	SV_LinkEdict(ent, true);

	return false;
}

/*
======================
SV_FixCheckBottom

======================
*/
void SV_FixCheckBottom(qhedict_t* ent)
{
//	Con_Printf ("SV_FixCheckBottom\n");

	ent->SetFlags((int)ent->GetFlags() | FL_PARTIALGROUND);
}



/*
================
SV_NewChaseDir

================
*/
#define DI_NODIR    -1
void SV_NewChaseDir(qhedict_t* actor, qhedict_t* enemy, float dist)
{
	float deltax,deltay;
	float d[3];
	float tdir, olddir, turnaround;

	olddir = AngleMod((int)(actor->GetIdealYaw() / 45) * 45);
	turnaround = AngleMod(olddir - 180);

	deltax = enemy->GetOrigin()[0] - actor->GetOrigin()[0];
	deltay = enemy->GetOrigin()[1] - actor->GetOrigin()[1];
	if (deltax > 10)
	{
		d[1] = 0;
	}
	else if (deltax < -10)
	{
		d[1] = 180;
	}
	else
	{
		d[1] = DI_NODIR;
	}
	if (deltay < -10)
	{
		d[2] = 270;
	}
	else if (deltay > 10)
	{
		d[2] = 90;
	}
	else
	{
		d[2] = DI_NODIR;
	}

// try direct route
	if (d[1] != DI_NODIR && d[2] != DI_NODIR)
	{
		if (d[1] == 0)
		{
			tdir = d[2] == 90 ? 45 : 315;
		}
		else
		{
			tdir = d[2] == 90 ? 135 : 215;
		}

		if (tdir != turnaround && SV_StepDirection(actor, tdir, dist))
		{
			return;
		}
	}

// try other directions
	if (((rand() & 3) & 1) ||  abs(deltay) > abs(deltax))
	{
		tdir = d[1];
		d[1] = d[2];
		d[2] = tdir;
	}

	if (d[1] != DI_NODIR && d[1] != turnaround &&
		SV_StepDirection(actor, d[1], dist))
	{
		return;
	}

	if (d[2] != DI_NODIR && d[2] != turnaround &&
		SV_StepDirection(actor, d[2], dist))
	{
		return;
	}

/* there is no direct path to the player, so pick another direction */

	if (olddir != DI_NODIR && SV_StepDirection(actor, olddir, dist))
	{
		return;
	}

	if (rand() & 1)		/*randomly determine direction of search*/
	{
		for (tdir = 0; tdir <= 315; tdir += 45)
			if (tdir != turnaround && SV_StepDirection(actor, tdir, dist))
			{
				return;
			}
	}
	else
	{
		for (tdir = 315; tdir >= 0; tdir -= 45)
			if (tdir != turnaround && SV_StepDirection(actor, tdir, dist))
			{
				return;
			}
	}

	if (turnaround != DI_NODIR && SV_StepDirection(actor, turnaround, dist))
	{
		return;
	}

	actor->SetIdealYaw(olddir);		// can't move

// if a bridge was pulled out from underneath a monster, it may not have
// a valid standing position at all

	if (!SV_CheckBottom(actor))
	{
		SV_FixCheckBottom(actor);
	}

}

/*
======================
SV_CloseEnough

======================
*/
qboolean SV_CloseEnough(qhedict_t* ent, qhedict_t* goal, float dist)
{
	int i;

	for (i = 0; i < 3; i++)
	{
		if (goal->v.absmin[i] > ent->v.absmax[i] + dist)
		{
			return false;
		}
		if (goal->v.absmax[i] < ent->v.absmin[i] - dist)
		{
			return false;
		}
	}
	return true;
}

/*
======================
SV_MoveToGoal

======================
*/
void SV_MoveToGoal(void)
{
	qhedict_t* ent, * goal;
	float dist;

	ent = PROG_TO_EDICT(pr_global_struct->self);
	goal = PROG_TO_EDICT(ent->GetGoalEntity());
	dist = G_FLOAT(OFS_PARM0);

	if (!((int)ent->GetFlags() & (FL_ONGROUND | FL_FLY | FL_SWIM)))
	{
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

// if the next step hits the enemy, return immediately
	if (PROG_TO_EDICT(ent->GetEnemy()) != sv.edicts &&  SV_CloseEnough(ent, goal, dist))
	{
		return;
	}

// bump around...
	if ((rand() & 3) == 1 ||
		!SV_StepDirection(ent, ent->GetIdealYaw(), dist))
	{
		SV_NewChaseDir(ent, goal, dist);
	}
}
