// sv_phys.c

/*
 * $Header: /H2 Mission Pack/SV_PHYS.C 10    3/17/98 6:14p Jmonroe $
 */

#include "quakedef.h"

/*


pushmove objects do not obey gravity, and do not interact with each other or trigger fields, but block normal movement and push normal objects when they move.

onground is set for toss objects when they come to a complete rest.  it is set for steping or walking objects

doors, plats, etc are SOLID_BSP, and QHMOVETYPE_PUSH
bonus items are SOLID_TRIGGER touch, and QHMOVETYPE_TOSS
corpses are SOLID_NOT and QHMOVETYPE_TOSS
crates are SOLID_BBOX and QHMOVETYPE_TOSS
walking monsters are SOLID_SLIDEBOX and QHMOVETYPE_STEP
flying/floating monsters are SOLID_SLIDEBOX and QHMOVETYPE_FLY

solid_edge items only clip against bsp models.

*/

Cvar* sv_friction;
Cvar* sv_stopspeed;
Cvar* sv_gravity;
Cvar* sv_maxvelocity;
Cvar* sv_nostep;
Cvar* sv_flypitch;
Cvar* sv_walkpitch;

#define MOVE_EPSILON    0.01

void SV_Physics_Toss(qhedict_t* ent);

/*
================
SV_CheckAllEnts
================
*/
void SV_CheckAllEnts(void)
{
	int e;
	qhedict_t* check;

// see if any solid entities are inside the final position
	check = NEXT_EDICT(sv.edicts);
	for (e = 1; e < sv.num_edicts; e++, check = NEXT_EDICT(check))
	{
		if (check->free)
		{
			continue;
		}
		if (check->GetMoveType() == QHMOVETYPE_PUSH ||
			check->GetMoveType() == QHMOVETYPE_NONE ||
			check->GetMoveType() == QHMOVETYPE_NOCLIP)
		{
			continue;
		}

		if (SV_TestEntityPosition(check))
		{
			Con_Printf("entity in invalid position\n");
		}
	}
}

/*
================
SV_CheckVelocity
================
*/
void SV_CheckVelocity(qhedict_t* ent)
{
	int i;

//
// bound velocity
//
	for (i = 0; i < 3; i++)
	{
		if (IS_NAN(ent->GetVelocity()[i]))
		{
			Con_DPrintf("Got a NaN velocity on %s\n", PR_GetString(ent->GetClassName()));
			ent->GetVelocity()[i] = 0;
		}
		if (IS_NAN(ent->GetOrigin()[i]))
		{
			Con_DPrintf("Got a NaN origin on %s\n", PR_GetString(ent->GetClassName()));
			ent->GetOrigin()[i] = 0;
		}
		if (ent->GetVelocity()[i] > sv_maxvelocity->value)
		{
			ent->GetVelocity()[i] = sv_maxvelocity->value;
		}
		else if (ent->GetVelocity()[i] < -sv_maxvelocity->value)
		{
			ent->GetVelocity()[i] = -sv_maxvelocity->value;
		}
	}
}

/*
=============
SV_RunThink

Runs thinking code if time.  There is some play in the exact time the think
function will be called, because it is called before any movement is done
in a frame.  Not used for pushmove objects, because they must be exact.
Returns false if the entity removed itself.
=============
*/
qboolean SV_RunThink(qhedict_t* ent)
{
	float thinktime;

	thinktime = ent->GetNextThink();
	if (thinktime <= 0 || thinktime > sv.time + host_frametime)
	{
		return true;
	}

	if (thinktime < sv.time)
	{
		thinktime = sv.time;	// don't let things stay in the past.
	}
	// it is possible to start that way
	// by a trigger with a local time.
	ent->SetNextThink(0);
	pr_global_struct->time = thinktime;
	pr_global_struct->self = EDICT_TO_PROG(ent);
	pr_global_struct->other = EDICT_TO_PROG(sv.edicts);
	PR_ExecuteProgram(ent->GetThink());
	return !ent->free;
}

/*
==================
SV_Impact

Two entities have touched, so run their touch functions
==================
*/
void SV_Impact(qhedict_t* e1, qhedict_t* e2)
{
	int old_self, old_other;

	old_self = pr_global_struct->self;
	old_other = pr_global_struct->other;

	pr_global_struct->time = sv.time;
	if (e1->GetTouch() && e1->GetSolid() != SOLID_NOT)
	{
		pr_global_struct->self = EDICT_TO_PROG(e1);
		pr_global_struct->other = EDICT_TO_PROG(e2);
		PR_ExecuteProgram(e1->GetTouch());
	}

	if (e2->GetTouch() && e2->GetSolid() != SOLID_NOT)
	{
		pr_global_struct->self = EDICT_TO_PROG(e2);
		pr_global_struct->other = EDICT_TO_PROG(e1);
		PR_ExecuteProgram(e2->GetTouch());
	}


	pr_global_struct->self = old_self;
	pr_global_struct->other = old_other;
}

/*
============
SV_FlyMove

The basic solid body movement clip that slides along multiple planes
Returns the clipflags if the velocity was modified (hit something solid)
1 = floor
2 = wall / step
4 = dead stop
If steptrace is not NULL, the trace of any vertical wall hit will be stored
============
*/
int SV_FlyMove(qhedict_t* ent, float time, q1trace_t* steptrace)
{
	int bumpcount, numbumps;
	vec3_t dir;
	float d;
	int numplanes;
	vec3_t planes[MAX_FLY_MOVE_CLIP_PLANES];
	vec3_t primal_velocity, original_velocity, new_velocity;
//rjr	vec3_t		before_velocity;
	int i, j;
	q1trace_t trace;
	vec3_t end;
	float time_left;
	int blocked;

	numbumps = 4;

	blocked = 0;
	VectorCopy(ent->GetVelocity(), original_velocity);
	VectorCopy(ent->GetVelocity(), primal_velocity);
	numplanes = 0;

	time_left = time;

	for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		if (!ent->GetVelocity()[0] && !ent->GetVelocity()[1] && !ent->GetVelocity()[2])
		{
			break;
		}

		for (i = 0; i < 3; i++)
			end[i] = ent->GetOrigin()[i] + time_left* ent->GetVelocity()[i];

		trace = SV_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), end, false, ent);

		if (trace.allsolid)
		{	// entity is trapped in another solid
			VectorCopy(vec3_origin, ent->GetVelocity());
			return 3;
		}

		if (trace.fraction > 0)
		{	// actually covered some distance
			VectorCopy(trace.endpos, ent->GetOrigin());
			VectorCopy(ent->GetVelocity(), original_velocity);
			numplanes = 0;
		}

		if (trace.fraction == 1)
		{
			break;		// moved the entire distance

		}
		if (trace.entityNum < 0)
		{
			Sys_Error("SV_FlyMove: !trace.ent");
		}

		if (trace.plane.normal[2] > 0.7)
		{
			blocked |= 1;		// floor
			if (EDICT_NUM(trace.entityNum)->GetSolid() == SOLID_BSP)
			{
				ent->SetFlags((int)ent->GetFlags() | FL_ONGROUND);
				ent->SetGroundEntity(EDICT_TO_PROG(EDICT_NUM(trace.entityNum)));
			}
		}
		if (!trace.plane.normal[2])
		{
			blocked |= 2;		// step
			if (steptrace)
			{
				*steptrace = trace;	// save for player extrafriction
			}
		}

//rjr		VectorCopy (ent->v.velocity, before_velocity);

//
// run the impact function
//
		SV_Impact(ent, EDICT_NUM(trace.entityNum));
		if (ent->free)
		{
			break;		// removed by the impact function

		}
/*rjr		if (!VectorCompare(ent->v.velocity, before_velocity))
        {
            break;
        }
*/

		time_left -= time_left * trace.fraction;

		// cliped to another plane
		if (numplanes >= MAX_FLY_MOVE_CLIP_PLANES)
		{	// this shouldn't really happen
			VectorCopy(vec3_origin, ent->GetVelocity());
			return 3;
		}

		VectorCopy(trace.plane.normal, planes[numplanes]);
		numplanes++;

//
// modify original_velocity so it parallels all of the clip planes
//
		for (i = 0; i < numplanes; i++)
		{
			PM_ClipVelocity(original_velocity, planes[i], new_velocity, 1);
			for (j = 0; j < numplanes; j++)
				if (j != i)
				{
					if (DotProduct(new_velocity, planes[j]) < 0)
					{
						break;	// not ok
					}
				}
			if (j == numplanes)
			{
				break;
			}
		}

		if (i != numplanes)
		{	// go along this plane
			VectorCopy(new_velocity, ent->GetVelocity());
		}
		else
		{	// go along the crease
			if (numplanes != 2)
			{
//				Con_Printf ("clip velocity, numplanes == %i\n",numplanes);
				VectorCopy(vec3_origin, ent->GetVelocity());
				return 7;
			}
			CrossProduct(planes[0], planes[1], dir);
			d = DotProduct(dir, ent->GetVelocity());
			VectorScale(dir, d, ent->GetVelocity());
		}

//
// if original velocity is against the original velocity, stop dead
// to avoid tiny occilations in sloping corners
//
		if (DotProduct(ent->GetVelocity(), primal_velocity) <= 0)
		{
			VectorCopy(vec3_origin, ent->GetVelocity());
			return blocked;
		}
	}

	return blocked;
}


/*
============
SV_FlyExtras

============
*/
void SV_FlyExtras(qhedict_t* ent, float time, q1trace_t* steptrace)
{
	const float hoverinc = 0.4;

	ent->SetFlags((int)ent->GetFlags() | FL_ONGROUND);	// Jumping makes you loose this flag so reset it

	if ((ent->GetVelocity()[2] <= 6) && (ent->GetVelocity()[2] >= -6))
	{
		ent->GetVelocity()[2] += ent->GetHoverZ();

		if (ent->GetVelocity()[2] >= 6)
		{
			ent->SetHoverZ(-hoverinc);
			ent->GetVelocity()[2] += ent->GetHoverZ();
		}
		else if (ent->GetVelocity()[2] <= -6)
		{
			ent->SetHoverZ(hoverinc);
			ent->GetVelocity()[2] += ent->GetHoverZ();
		}
	}
	else	// friction for upward or downward progress once key is released
	{
		ent->GetVelocity()[2] -= sv_player->GetVelocity()[2] * .1;
	}

}

/*
============
SV_AddGravity

============
*/
void SV_AddGravity(qhedict_t* ent)
{
	float ent_gravity;

	eval_t* val;

	val = GetEdictFieldValue(ent, "gravity");
	if (val && val->_float)
	{
		ent_gravity = val->_float;
	}
	else
	{
		ent_gravity = 1.0;
	}

	ent->GetVelocity()[2] -= ent_gravity * sv_gravity->value * host_frametime;
}


/*
===============================================================================

PUSHMOVE

===============================================================================
*/

/*
============
SV_PushEntity

Does not change the entities velocity at all
============
*/
q1trace_t SV_PushEntity(qhedict_t* ent, vec3_t push)
{
	q1trace_t trace;
	vec3_t start,end, impact;
	qhedict_t* impact_e;

	VectorCopy(ent->GetOrigin(), start);
	VectorAdd(ent->GetOrigin(), push, end);

	if (ent->GetMoveType() == QHMOVETYPE_FLYMISSILE  || ent->GetMoveType() == H2MOVETYPE_BOUNCEMISSILE)
	{
		trace = SV_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), end, MOVE_MISSILE, ent);
	}
	else if (ent->GetSolid() == SOLID_TRIGGER || ent->GetSolid() == SOLID_NOT)
	{
// only clip against bmodels
		trace = SV_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), end, MOVE_NOMONSTERS, ent);
	}
	else if (ent->GetMoveType() == H2MOVETYPE_SWIM)
	{
		trace = SV_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), end, MOVE_WATER, ent);
	}
	else
	{
		trace = SV_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), end, MOVE_NORMAL, ent);
	}

	if (ent->GetSolid() != SOLID_PHASE)
	{
		if (ent->GetMoveType() != QHMOVETYPE_BOUNCE || (trace.allsolid == 0 && trace.startsolid == 0))
		{
			VectorCopy(trace.endpos, ent->GetOrigin());		// Macro - watchout
		}
		else
		{
			trace.fraction = 0;

			return trace;
		}
	}
	else	// Entity is PHASED so bounce off walls and other entities, go through monsters and players
	{
		if (trace.entityNum >= 0)
		{	// Go through MONSTERS and PLAYERS, can't use FL_CLIENT cause rotating brushes do
			if (((int)EDICT_NUM(trace.entityNum)->GetFlags() & FL_MONSTER) ||
				(EDICT_NUM(trace.entityNum)->GetMoveType() == QHMOVETYPE_WALK))
			{
				VectorCopy(trace.endpos, impact);
				impact_e = EDICT_NUM(trace.entityNum);

				trace = SV_Move(ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), end, MOVE_PHASE, ent);

				VectorCopy(impact, ent->GetOrigin());
				SV_Impact(ent, impact_e);

				VectorCopy(trace.endpos, ent->GetOrigin());
			}
			else
			{
				VectorCopy(trace.endpos, ent->GetOrigin());
			}
		}
		else
		{
			VectorCopy(trace.endpos, ent->GetOrigin());
		}
	}

	SV_LinkEdict(ent, true);

	if (trace.entityNum >= 0)
	{
		SV_Impact(ent, EDICT_NUM(trace.entityNum));
	}

	return trace;
}


/*
============
SV_PushMove

============
*/
void SV_PushMove(qhedict_t* pusher, float movetime, qboolean update_time)
{
	int i, e;
	qhedict_t* check, * block;
	vec3_t mins, maxs, move;
	vec3_t entorig, pushorig;
	int num_moved;
	qhedict_t* moved_edict[MAX_EDICTS_H2];
	vec3_t moved_from[MAX_EDICTS_H2];

	if (!pusher->GetVelocity()[0] && !pusher->GetVelocity()[1] && !pusher->GetVelocity()[2])
	{
		if (update_time)
		{
			pusher->v.ltime += movetime;
		}
		return;
	}

	for (i = 0; i < 3; i++)
	{
		move[i] = pusher->GetVelocity()[i] * movetime;
		mins[i] = pusher->v.absmin[i] + move[i];
		maxs[i] = pusher->v.absmax[i] + move[i];
	}

	VectorCopy(pusher->GetOrigin(), pushorig);

// move the pusher to it's final position

	VectorAdd(pusher->GetOrigin(), move, pusher->GetOrigin());
	if (update_time)
	{
		pusher->v.ltime += movetime;
	}
	SV_LinkEdict(pusher, false);


// see if any solid entities are inside the final position
	num_moved = 0;
	check = NEXT_EDICT(sv.edicts);
	for (e = 1; e < sv.num_edicts; e++, check = NEXT_EDICT(check))
	{
		if (check->free)
		{
			continue;
		}
		if (check->GetMoveType() == QHMOVETYPE_PUSH ||
			check->GetMoveType() == QHMOVETYPE_NONE ||
			check->GetMoveType() == QHMOVETYPE_NOCLIP)
		{
			continue;
		}

		// if the entity is standing on the pusher, it will definately be moved
		if (!(((int)check->GetFlags() & FL_ONGROUND) &&
			  PROG_TO_EDICT(check->GetGroundEntity()) == pusher))
		{
			if (check->v.absmin[0] >= maxs[0] ||
				check->v.absmin[1] >= maxs[1] ||
				check->v.absmin[2] >= maxs[2] ||
				check->v.absmax[0] <= mins[0] ||
				check->v.absmax[1] <= mins[1] ||
				check->v.absmax[2] <= mins[2])
			{
				continue;
			}

			// see if the ent's bbox is inside the pusher's final position
			if (!SV_TestEntityPosition(check))
			{
				continue;
			}
		}

		// remove the onground flag for non-players
		if (check->GetMoveType() != QHMOVETYPE_WALK)
		{
			check->SetFlags((int)check->GetFlags() & ~FL_ONGROUND);
		}

		VectorCopy(check->GetOrigin(), entorig);
		VectorCopy(check->GetOrigin(), moved_from[num_moved]);
		moved_edict[num_moved] = check;
		num_moved++;

		// try moving the contacted entity
		pusher->SetSolid(SOLID_NOT);
		SV_PushEntity(check, move);
		pusher->SetSolid(SOLID_BSP);

		// if it is still inside the pusher, block
		block = SV_TestEntityPosition(check);
		if (block)
		{	// fail the move
			if (check->GetMins()[0] == check->GetMaxs()[0])
			{
				continue;
			}
			if (check->GetSolid() == SOLID_NOT || check->GetSolid() == SOLID_TRIGGER)
			{	// corpse
				check->GetMins()[0] = check->GetMins()[1] = 0;
				check->SetMaxs(check->GetMins());
				continue;
			}

			VectorCopy(entorig, check->GetOrigin());
			SV_LinkEdict(check, true);

			VectorCopy(pushorig, pusher->GetOrigin());
			SV_LinkEdict(pusher, false);
			if (update_time)
			{
				pusher->v.ltime -= movetime;
			}

			// if the pusher has a "blocked" function, call it
			// otherwise, just stay in place until the obstacle is gone
			if (pusher->GetBlocked())
			{
				pr_global_struct->self = EDICT_TO_PROG(pusher);
				pr_global_struct->other = EDICT_TO_PROG(check);
				PR_ExecuteProgram(pusher->GetBlocked());
			}

			// move back any entities we already moved
			for (i = 0; i < num_moved; i++)
			{
				VectorCopy(moved_from[i], moved_edict[i]->GetOrigin());
				SV_LinkEdict(moved_edict[i], false);
			}
			return;
		}
	}


}

/*
============
SV_PushRotate

============
*/
/*
void SV_PushRotate (qhedict_t *pusher, float movetime)
{
    int			i, e;
    qhedict_t		*check, *block;
    vec3_t		move, a, amove;
    vec3_t		entorig, pushorig;
    int			num_moved;
    qhedict_t		*moved_edict[MAX_EDICTS_H2];
    vec3_t		moved_from[MAX_EDICTS_H2];
    vec3_t		org, org2;
    vec3_t		forward, right, up;

    if (!pusher->v.avelocity[0] && !pusher->v.avelocity[1] && !pusher->v.avelocity[2])
    {
        pusher->v.ltime += movetime;
        return;
    }

    for (i=0 ; i<3 ; i++)
        amove[i] = pusher->v.avelocity[i] * movetime;

    VectorSubtract (vec3_origin, amove, a);
    AngleVectors (a, forward, right, up);

    VectorCopy (pusher->v.angles, pushorig);

// move the pusher to it's final position

    VectorAdd (pusher->v.angles, amove, pusher->v.angles);
    pusher->v.ltime += movetime;
    SV_LinkEdict (pusher, false);


// see if any solid entities are inside the final position
    num_moved = 0;
    check = NEXT_EDICT(sv.edicts);
    for (e=1 ; e<sv.num_edicts ; e++, check = NEXT_EDICT(check))
    {
        if (check->free)
            continue;
        if (check->v.movetype == QHMOVETYPE_PUSH
        || check->v.movetype == QHMOVETYPE_NONE
        || check->v.movetype == H2MOVETYPE_FOLLOW
        || check->v.movetype == QHMOVETYPE_NOCLIP)
            continue;

    // if the entity is standing on the pusher, it will definately be moved
        if ( ! ( ((int)check->v.flags & FL_ONGROUND)
        && PROG_TO_EDICT(check->v.groundentity) == pusher) )
        {
            if ( check->v.absmin[0] >= pusher->v.absmax[0]
            || check->v.absmin[1] >= pusher->v.absmax[1]
            || check->v.absmin[2] >= pusher->v.absmax[2]
            || check->v.absmax[0] <= pusher->v.absmin[0]
            || check->v.absmax[1] <= pusher->v.absmin[1]
            || check->v.absmax[2] <= pusher->v.absmin[2] )
                continue;

        // see if the ent's bbox is inside the pusher's final position
            if (!SV_TestEntityPosition (check))
                continue;
        }

    // remove the onground flag for non-players
        if (check->v.movetype != QHMOVETYPE_WALK)
            check->v.flags = (int)check->v.flags & ~FL_ONGROUND;

        VectorCopy (check->v.origin, entorig);
        VectorCopy (check->v.origin, moved_from[num_moved]);
        moved_edict[num_moved] = check;
        num_moved++;

        // calculate destination position
        VectorSubtract (check->v.origin, pusher->v.origin, org);
        org2[0] = DotProduct (org, forward);
        org2[1] = -DotProduct (org, right);
        org2[2] = DotProduct (org, up);
        VectorSubtract (org2, org, move);

        // try moving the contacted entity
        pusher->v.solid = SOLID_NOT;
        SV_PushEntity (check, move);
        pusher->v.solid = SOLID_BSP;

    // if it is still inside the pusher, block
        block = SV_TestEntityPosition (check);
        if (block)
        {	// fail the move
            if (check->v.mins[0] == check->v.maxs[0])
                continue;
            if (check->v.solid == SOLID_NOT || check->v.solid == SOLID_TRIGGER)
            {	// corpse
                check->v.mins[0] = check->v.mins[1] = 0;
                VectorCopy (check->v.mins, check->v.maxs);
                continue;
            }

            VectorCopy (entorig, check->v.origin);
            SV_LinkEdict (check, true);

            VectorCopy (pushorig, pusher->v.angles);
            SV_LinkEdict (pusher, false);
            pusher->v.ltime -= movetime;

            // if the pusher has a "blocked" function, call it
            // otherwise, just stay in place until the obstacle is gone
            if (pusher->v.blocked)
            {
                pr_global_struct->self = EDICT_TO_PROG(pusher);
                pr_global_struct->other = EDICT_TO_PROG(check);
                PR_ExecuteProgram (pusher->v.blocked);
            }

        // move back any entities we already moved
            for (i=0 ; i<num_moved ; i++)
            {
                VectorCopy (moved_from[i], moved_edict[i]->v.origin);
                VectorSubtract (moved_edict[i]->v.angles, amove, moved_edict[i]->v.angles);
                SV_LinkEdict (moved_edict[i], false);
            }
            return;
        }
        else
        {
            VectorAdd (check->v.angles, amove, check->v.angles);
        }
    }


}
*/


/*
============
SV_PushRotate
NEW
============
*/
void SV_PushRotate(qhedict_t* pusher, float movetime)
{
	int i, e, t;
	qhedict_t* check, * block;
	vec3_t move, a, amove,mins,maxs,move2,move3,testmove /*,amove_norm*/;
	vec3_t entorig, pushorig,pushorigangles;
	int num_moved;
	qhedict_t* moved_edict[MAX_EDICTS_H2];
	vec3_t moved_from[MAX_EDICTS_H2];
	vec3_t org, org2, check_center;
	vec3_t forward, right, up;
	//vec3_t		dir2push,push_vel;
	qhedict_t* ground;
	qhedict_t* master;
	qhedict_t* slave;
	int slaves_moved;
	qboolean moveit	/*, null_z*/;
	//float		amove_mag,turn_away;

#if 0
	Con_DPrintf("SV_PushRotate entity %i (time=%f)\n", NUM_FOR_EDICT(pusher), movetime);
	Con_DPrintf("%f %f %f (avelocity)\n", pusher->v.avelocity[0], pusher->v.avelocity[1], pusher->v.avelocity[2]);
	Con_DPrintf("%f %f %f\n", pusher->v.angles[0], pusher->v.angles[1], pusher->v.angles[2]);
#endif

	for (i = 0; i < 3; i++)
	{
		amove[i] = pusher->GetAVelocity()[i] * movetime;
		move[i] = pusher->GetVelocity()[i] * movetime;
		mins[i] = pusher->v.absmin[i] + move[i];
		maxs[i] = pusher->v.absmax[i] + move[i];
	}

	VectorSubtract(vec3_origin, amove, a);
	AngleVectors(a, forward, right, up);

	VectorCopy(pusher->GetOrigin(), pushorig);
	VectorCopy(pusher->GetAngles(), pushorigangles);

// move the pusher to it's final position

	VectorAdd(pusher->GetOrigin(), move, pusher->GetOrigin());
	VectorAdd(pusher->GetAngles(), amove, pusher->GetAngles());

	pusher->v.ltime += movetime;
	SV_LinkEdict(pusher, false);

	master = pusher;
	slaves_moved = 0;
/*	while (master->v.aiment)
    {
        slave = PROG_TO_EDICT(master->v.aiment);
#if 0
        Con_DPrintf("%f %f %f   slave entity %i\n", slave->v.angles[0], slave->v.angles[1], slave->v.angles[2], NUM_FOR_EDICT(slave));
#endif

        slaves_moved++;
        VectorCopy (slave->v.angles, moved_from[MAX_EDICTS_H2 - slaves_moved]);
        moved_edict[MAX_EDICTS_H2 - slaves_moved] = slave;

        if (slave->v.movedir[PITCH])
            slave->v.angles[PITCH] = master->v.angles[PITCH];
        else
            slave->v.angles[PITCH] += slave->v.avelocity[PITCH] * movetime;

        if (slave->v.movedir[YAW])
            slave->v.angles[YAW] = master->v.angles[YAW];
        else
            slave->v.angles[YAW] += slave->v.avelocity[YAW] * movetime;

        if (slave->v.movedir[ROLL])
            slave->v.angles[ROLL] = master->v.angles[ROLL];
        else
            slave->v.angles[ROLL] += slave->v.avelocity[ROLL] * movetime;

        slave->v.ltime = master->v.ltime;
        SV_LinkEdict (slave, false);

        master = slave;
    }
*/

// see if any solid entities are inside the final position
	num_moved = 0;
	check = NEXT_EDICT(sv.edicts);
	for (e = 1; e < sv.num_edicts; e++, check = NEXT_EDICT(check))
	{
		if (check->free)
		{
			continue;
		}
		if (check->GetMoveType() == QHMOVETYPE_PUSH ||
			check->GetMoveType() == QHMOVETYPE_NONE ||
			check->GetMoveType() == H2MOVETYPE_FOLLOW ||
			check->GetMoveType() == QHMOVETYPE_NOCLIP)
		{
			continue;
		}

		// if the entity is standing on the pusher, it will definitely be moved
		moveit = false;
		ground = PROG_TO_EDICT(check->GetGroundEntity());
		if ((int)check->GetFlags() & FL_ONGROUND)
		{
			if (ground == pusher)
			{
				moveit = true;
			}
			else
			{
				for (i = 0; i < slaves_moved; i++)
				{
					if (ground == moved_edict[MAX_EDICTS_H2 - i - 1])
					{
						moveit = true;
						break;
					}
				}
			}
		}

		if (!moveit)
		{
			if (check->v.absmin[0] >= maxs[0] ||
				check->v.absmin[1] >= maxs[1] ||
				check->v.absmin[2] >= maxs[2] ||
				check->v.absmax[0] <= mins[0] ||
				check->v.absmax[1] <= mins[1] ||
				check->v.absmax[2] <= mins[2])
			{
				for (i = 0; i < slaves_moved; i++)
				{
					slave = moved_edict[MAX_EDICTS_H2 - i - 1];
					if (check->v.absmin[0] >= slave->v.absmax[0] ||
						check->v.absmin[1] >= slave->v.absmax[1] ||
						check->v.absmin[2] >= slave->v.absmax[2] ||
						check->v.absmax[0] <= slave->v.absmin[0] ||
						check->v.absmax[1] <= slave->v.absmin[1] ||
						check->v.absmax[2] <= slave->v.absmin[2])
					{
						continue;
					}
				}
				if (i == slaves_moved)
				{
					continue;
				}
			}

			// see if the ent's bbox is inside the pusher's final position
			if (!SV_TestEntityPosition(check))
			{
				continue;
			}
		}

		// remove the onground flag for non-players
		if (check->GetMoveType() != QHMOVETYPE_WALK)
		{
			check->SetFlags((int)check->GetFlags() & ~FL_ONGROUND);
		}

		VectorCopy(check->GetOrigin(), entorig);
		VectorCopy(check->GetOrigin(), moved_from[num_moved]);
		moved_edict[num_moved] = check;
		num_moved++;

//put check in first move spot
		VectorAdd(check->GetOrigin(), move, check->GetOrigin());
//Use center of model, like in QUAKE!!!!  Our origins are on the bottom!!!
		for (i = 0; i < 3; i++)
			check_center[i] = (check->v.absmin[i] + check->v.absmax[i]) / 2;
// calculate destination position
		VectorSubtract(check_center, pusher->GetOrigin(), org);
//put check back
		VectorSubtract(check->GetOrigin(), move, check->GetOrigin());
		org2[0] = DotProduct(org, forward);
		org2[1] = -DotProduct(org, right);
		org2[2] = DotProduct(org, up);
		VectorSubtract(org2, org, move2);

//		Con_DPrintf("%f %f %f (move2)\n", move2[0], move2[1], move2[2]);

//		VectorAdd (check->v.origin, move2, check->v.origin);

		//Add all moves together
		VectorAdd(move,move2,move3);

		//Find the angle of rotation as compared to vector from pusher origin to check center
//		turn_away = DotProduct(org,a);

		// try moving the contacted entity
		for (t = 0; t < 13; t++)
		{
			switch (t)
			{
			case 0:
				//try x, y and z
				VectorCopy(move3,testmove);
				break;
			case 1:
				//Try xy only
				VectorSubtract(check->GetOrigin(),testmove,check->GetOrigin());
				testmove[0] = move3[0];
				testmove[1] = move3[1];
				testmove[2] = 0;
				break;
			case 2:
				//Try z only
				VectorSubtract(check->GetOrigin(),testmove,check->GetOrigin());
				testmove[0] = 0;
				testmove[1] = 0;
				testmove[2] = move3[2];
				break;
			case 3:
				//Try none
				VectorSubtract(check->GetOrigin(),testmove,check->GetOrigin());
				testmove[0] = 0;
				testmove[1] = 0;
				testmove[2] = 0;
				break;
			case 4:
				//Try xy in opposite dir
				testmove[0] = move3[0] * -1;
				testmove[1] = move3[1] * -1;
				testmove[2] = move3[2];
				break;
			case 5:
				//Try z in opposite dir
				VectorSubtract(check->GetOrigin(),testmove,check->GetOrigin());
				testmove[0] = move3[0];
				testmove[1] = move3[1];
				testmove[2] = move3[2] * -1;
				break;
			case 6:
				//Try xyz in opposite dir
				VectorSubtract(check->GetOrigin(),testmove,check->GetOrigin());
				testmove[0] = move3[0] * -1;
				testmove[1] = move3[1] * -1;
				testmove[2] = move3[2] * -1;
				break;
			case 7:
				//Try move3 times 2
				VectorSubtract(check->GetOrigin(),testmove,check->GetOrigin());
				VectorScale(move3,2,testmove);
				break;
			case 8:
				//Try normalized org
				VectorSubtract(check->GetOrigin(),testmove,check->GetOrigin());

//					VectorCopy(amove,amove_norm);
//					amove_mag=VectorNormalize(amove_norm);
//					//VectorNormalize(org);
//					VectorScale(org,amove_mag,org);

//					VectorNormalize(org);
				VectorScale(org,movetime,org);		//movetime*20?
				VectorCopy(org,testmove);
				break;
			case 9:
				//Try normalized org z * 3 only
				VectorSubtract(check->GetOrigin(),testmove,check->GetOrigin());
				testmove[0] = 0;
				testmove[1] = 0;
				testmove[2] = org[2] * 3;	//was: +org[2]*(Q_fabs(org[1])+Q_fabs(org[2]));
				break;
			case 10:
				//Try normalized org xy * 2 only
				VectorSubtract(check->GetOrigin(),testmove,check->GetOrigin());
				testmove[0] = org[0] * 2;	//was: +org[0]*Q_fabs(org[2]);
				testmove[1] = org[1] * 2;	//was: +org[1]*Q_fabs(org[2]);
				testmove[2] = 0;
				break;
			case 11:
				//Try xy in opposite org dir
				VectorSubtract(check->GetOrigin(),testmove,check->GetOrigin());
				testmove[0] = org[0] * -2;
				testmove[1] = org[1] * -2;
				testmove[2] = org[2];
				break;
			case 12:
				//Try z in opposite dir
				VectorSubtract(check->GetOrigin(),testmove,check->GetOrigin());
				testmove[0] = org[0];
				testmove[1] = org[1];
				testmove[2] = org[2] * -3;
				break;
			}

			if (t != 3)
			{
				//THIS IS VERY BAD BAD HACK...
				pusher->SetSolid(SOLID_NOT);
				SV_PushEntity(check, move3);
				//@@TODO: do we ever want to do anybody's angles?  maybe just yaw???
				//		if (!((int)check->v.flags & (FL_CLIENT | FL_MONSTER)))
				//			VectorAdd (check->v.angles, amove, check->v.angles);
				check->GetAngles()[YAW] += amove[YAW];
				pusher->SetSolid(SOLID_BSP);
			}
			// if it is still inside the pusher, block
			block = SV_TestEntityPosition(check);
			if (!block)
			{
				break;
			}
		}

//		Con_DPrintf("t: %i\n",t);

//		if(turn_away>0)
//		{
		if (block)
		{		// fail the move
				//			Con_DPrintf("Check blocked\n");
			if (check->GetMins()[0] == check->GetMaxs()[0])
			{
				continue;
			}
			if (check->GetSolid() == SOLID_NOT || check->GetSolid() == SOLID_TRIGGER)
			{		// corpse
				check->GetMins()[0] = check->GetMins()[1] = 0;
				check->SetMaxs(check->GetMins());
				continue;
			}

			VectorCopy(entorig, check->GetOrigin());
			SV_LinkEdict(check, true);

			VectorCopy(pushorig, pusher->GetOrigin());
			pusher->SetAngles(pushorigangles);
			SV_LinkEdict(pusher, false);
			pusher->v.ltime -= movetime;

			for (i = 0; i < slaves_moved; i++)
			{
				slave = moved_edict[MAX_EDICTS_H2 - i - 1];
				slave->SetAngles(moved_from[MAX_EDICTS_H2 - i - 1]);
				SV_LinkEdict(slave, false);
				slave->v.ltime -= movetime;
			}

			// if the pusher has a "blocked" function, call it
			// otherwise, just stay in place until the obstacle is gone
			if (pusher->GetBlocked())
			{
				pr_global_struct->self = EDICT_TO_PROG(pusher);
				pr_global_struct->other = EDICT_TO_PROG(check);
				PR_ExecuteProgram(pusher->GetBlocked());
			}

			// move back any entities we already moved
			for (i = 0; i < num_moved; i++)
			{
				VectorCopy(moved_from[i], moved_edict[i]->GetOrigin());
				//@@TODO:: see above
				//				if (!((int)moved_edict[i]->v.flags & (FL_CLIENT | FL_MONSTER)))
				//					VectorSubtract (moved_edict[i]->v.angles, amove, moved_edict[i]->v.angles);
				moved_edict[i]->GetAngles()[YAW] -= amove[YAW];

				SV_LinkEdict(moved_edict[i], false);
			}
			return;
		}
//		}
//		else if(block)//undo last move
//			VectorCopy (entorig, check->v.origin);
	}

#if 0
	Con_DPrintf("result:\n");
	Con_DPrintf("%f %f %f\n", pusher->v.angles[0], pusher->v.angles[1], pusher->v.angles[2]);
	for (i = 0; i < slaves_moved; i++)
	{
		slave = moved_edict[MAX_EDICTS_H2 - i - 1];
		Con_DPrintf("%f %f %f   slave entity %i\n", slave->v.angles[0], slave->v.angles[1], slave->v.angles[2], NUM_FOR_EDICT(slave));
	}
	Con_DPrintf("\n");
#endif
}

/*
================
SV_Physics_Pusher

================
*/
void SV_Physics_Pusher(qhedict_t* ent)
{
	float thinktime;
	float oldltime;
	float movetime;

	oldltime = ent->v.ltime;

	thinktime = ent->GetNextThink();
	if (thinktime < ent->v.ltime + host_frametime)
	{
		movetime = thinktime - ent->v.ltime;
		if (movetime < 0)
		{
			movetime = 0;
		}
	}
	else
	{
		movetime = host_frametime;
	}

	if (movetime)
	{
		if (ent->GetAVelocity()[0] || ent->GetAVelocity()[1] || ent->GetAVelocity()[2])
		{
			//SV_PushMove (ent, movetime, false);
			SV_PushRotate(ent, movetime);
		}
		else
		{
			SV_PushMove(ent, movetime, true);	// advances ent->v.ltime if not blocked
		}
	}

	if (thinktime > oldltime && thinktime <= ent->v.ltime)
	{
		ent->SetNextThink(0);
		pr_global_struct->time = sv.time;
		pr_global_struct->self = EDICT_TO_PROG(ent);
		pr_global_struct->other = EDICT_TO_PROG(sv.edicts);
		PR_ExecuteProgram(ent->GetThink());
		if (ent->free)
		{
			return;
		}
	}

}


/*
===============================================================================

CLIENT MOVEMENT

===============================================================================
*/

/*
=============
SV_CheckStuck

This is a big hack to try and fix the rare case of getting stuck in the world
clipping hull.
=============
*/
void SV_CheckStuck(qhedict_t* ent)
{
	int i, j;
	int z;
	vec3_t org;

	if (!SV_TestEntityPosition(ent))
	{
		VectorCopy(ent->GetOrigin(), ent->GetOldOrigin());
		return;
	}

	VectorCopy(ent->GetOrigin(), org);
	VectorCopy(ent->GetOldOrigin(), ent->GetOrigin());
	if (!SV_TestEntityPosition(ent))
	{
		Con_DPrintf("moved back Unstuck.\n");
		SV_LinkEdict(ent, true);
		return;
	}

	for (z = 0; z < 18; z++)
		for (i = -1; i <= 1; i++)
			for (j = -1; j <= 1; j++)
			{
				ent->GetOrigin()[0] = org[0] + i;
				ent->GetOrigin()[1] = org[1] + j;
				ent->GetOrigin()[2] = org[2] + z;
				if (!SV_TestEntityPosition(ent))
				{
					Con_DPrintf("scooted Unstuck.\n");
					SV_LinkEdict(ent, true);
					return;
				}
			}

	VectorCopy(org, ent->GetOrigin());
	if (ent->GetOldOrigin() != ent->GetOrigin())
	{
		Con_DPrintf("player is stuck.\n");
	}
}


/*
=============
SV_CheckWater
=============
*/
qboolean SV_CheckWater(qhedict_t* ent)
{
	vec3_t point;
	int cont;

	point[0] = ent->GetOrigin()[0];
	point[1] = ent->GetOrigin()[1];
	point[2] = ent->GetOrigin()[2] + ent->GetMins()[2] + 1;

	ent->SetWaterLevel(0);
	ent->SetWaterType(BSP29CONTENTS_EMPTY);
	cont = SV_PointContents(point);
	if (cont <= BSP29CONTENTS_WATER)
	{
		ent->SetWaterType(cont);
		ent->SetWaterLevel(1);
		point[2] = ent->GetOrigin()[2] + (ent->GetMins()[2] + ent->GetMaxs()[2]) * 0.5;
		cont = SV_PointContents(point);
		if (cont <= BSP29CONTENTS_WATER)
		{
			ent->SetWaterLevel(2);
			point[2] = ent->GetOrigin()[2] + ent->GetViewOfs()[2];
			cont = SV_PointContents(point);
			if (cont <= BSP29CONTENTS_WATER)
			{
				ent->SetWaterLevel(3);
			}
		}
	}

	return ent->GetWaterLevel() > 1;
}

/*
============
SV_WallFriction

============
*/
void SV_WallFriction(qhedict_t* ent, q1trace_t* trace)
{
	vec3_t forward, right, up;
	float d, i;
	vec3_t into, side;

	AngleVectors(ent->GetVAngle(), forward, right, up);
	d = DotProduct(trace->plane.normal, forward);

	d += 0.5;
	if (d >= 0)
	{
		return;
	}

// cut the tangential velocity
	i = DotProduct(trace->plane.normal, ent->GetVelocity());
	VectorScale(trace->plane.normal, i, into);
	VectorSubtract(ent->GetVelocity(), into, side);

	ent->GetVelocity()[0] = side[0] * (1 + d);
	ent->GetVelocity()[1] = side[1] * (1 + d);
}

/*
=====================
SV_TryUnstick

Player has come to a dead stop, possibly due to the problem with limited
float precision at some angle joins in the BSP hull.

Try fixing by pushing one pixel in each direction.

This is a hack, but in the interest of good gameplay...
======================
*/
int SV_TryUnstick(qhedict_t* ent, vec3_t oldvel)
{
	int i;
	vec3_t oldorg;
	vec3_t dir;
	int clip;
	q1trace_t steptrace;

	VectorCopy(ent->GetOrigin(), oldorg);
	VectorCopy(vec3_origin, dir);

	for (i = 0; i < 8; i++)
	{
// try pushing a little in an axial direction
		switch (i)
		{
		case 0: dir[0] = 2; dir[1] = 0; break;
		case 1: dir[0] = 0; dir[1] = 2; break;
		case 2: dir[0] = -2; dir[1] = 0; break;
		case 3: dir[0] = 0; dir[1] = -2; break;
		case 4: dir[0] = 2; dir[1] = 2; break;
		case 5: dir[0] = -2; dir[1] = 2; break;
		case 6: dir[0] = 2; dir[1] = -2; break;
		case 7: dir[0] = -2; dir[1] = -2; break;
		}

		SV_PushEntity(ent, dir);

// retry the original move
		ent->GetVelocity()[0] = oldvel[0];
		ent->GetVelocity()[1] = oldvel[1];
		ent->GetVelocity()[2] = 0;
		clip = SV_FlyMove(ent, 0.1, &steptrace);

		if (Q_fabs(oldorg[1] - ent->GetOrigin()[1]) > 4 ||
			Q_fabs(oldorg[0] - ent->GetOrigin()[0]) > 4)
		{
//			Con_DPrintf ("unstuck!\n");
			return clip;
		}

// go back to the original pos and try again
		VectorCopy(oldorg, ent->GetOrigin());
	}

	VectorCopy(vec3_origin, ent->GetVelocity());
	return 7;		// still not moving
}

/*
=====================
SV_WalkMove

Only used by players
======================
*/
void SV_WalkMove(qhedict_t* ent)
{
	vec3_t upmove, downmove;
	vec3_t oldorg, oldvel;
	vec3_t nosteporg, nostepvel;
	int clip;
	int oldonground;
	q1trace_t steptrace, downtrace;

//
// do a regular slide move unless it looks like you ran into a step
//
	oldonground = (int)ent->GetFlags() & FL_ONGROUND;
	ent->SetFlags((int)ent->GetFlags() & ~FL_ONGROUND);

	VectorCopy(ent->GetOrigin(), oldorg);
	VectorCopy(ent->GetVelocity(), oldvel);

	clip = SV_FlyMove(ent, host_frametime, &steptrace);

	if (!(clip & 2))
	{
		return;		// move didn't block on a step

	}
	if (!oldonground && ent->GetWaterLevel() == 0)
	{
		return;		// don't stair up while jumping

	}
	if (ent->GetMoveType() != QHMOVETYPE_WALK)
	{
		return;		// gibbed by a trigger

	}
	if (sv_nostep->value)
	{
		return;
	}

	if ((int)sv_player->GetFlags() & FL_WATERJUMP)
	{
		return;
	}

	VectorCopy(ent->GetOrigin(), nosteporg);
	VectorCopy(ent->GetVelocity(), nostepvel);

//
// try moving up and forward to go up a step
//
	VectorCopy(oldorg, ent->GetOrigin());	// back to start pos

	VectorCopy(vec3_origin, upmove);
	VectorCopy(vec3_origin, downmove);
	upmove[2] = STEPSIZE;
	downmove[2] = -STEPSIZE + oldvel[2] * host_frametime;

// move up
	SV_PushEntity(ent, upmove);		// FIXME: don't link?

// move forward
	ent->GetVelocity()[0] = oldvel[0];
	ent->GetVelocity()[1] = oldvel[1];
	ent->GetVelocity()[2] = 0;
	clip = SV_FlyMove(ent, host_frametime, &steptrace);

// check for stuckness, possibly due to the limited precision of floats
// in the clipping hulls
	if (clip)
	{
		if (Q_fabs(oldorg[1] - ent->GetOrigin()[1]) < 0.03125 &&
			Q_fabs(oldorg[0] - ent->GetOrigin()[0]) < 0.03125)
		{	// stepping up didn't make any progress
			clip = SV_TryUnstick(ent, oldvel);
		}
	}

// extra friction based on view angle
	if (clip & 2)
	{
		SV_WallFriction(ent, &steptrace);
	}

// move down
	downtrace = SV_PushEntity(ent, downmove);	// FIXME: don't link?

	if (downtrace.plane.normal[2] > 0.7)
	{
		if (ent->GetSolid() == SOLID_BSP)
		{
			ent->SetFlags((int)ent->GetFlags() | FL_ONGROUND);
			ent->SetGroundEntity(EDICT_TO_PROG(EDICT_NUM(downtrace.entityNum)));
		}
	}
	else
	{
// if the push down didn't end up on good ground, use the move without
// the step up.  This happens near wall / slope combinations, and can
// cause the player to hop up higher on a slope too steep to climb
		VectorCopy(nosteporg, ent->GetOrigin());
		VectorCopy(nostepvel, ent->GetVelocity());
	}
}


/*
================
SV_Physics_Client

Player character actions
================
*/
void SV_Physics_Client(qhedict_t* ent, int num)
{
	if (svs.clients[num - 1].state < CS_CONNECTED)
	{
		return;		// unconnected slot

	}
//
// call standard client pre-think
//
	pr_global_struct->time = sv.time;
	pr_global_struct->self = EDICT_TO_PROG(ent);
	PR_ExecuteProgram(pr_global_struct->PlayerPreThink);

//
// do a move
//
	SV_CheckVelocity(ent);

//
// decide which move function to call
//
	switch ((int)ent->GetMoveType())
	{
	case QHMOVETYPE_NONE:
		if (!SV_RunThink(ent))
		{
			return;
		}
		break;

	case QHMOVETYPE_WALK:
		if (!SV_RunThink(ent))
		{
			return;
		}
		if (!SV_CheckWater(ent) && !((int)ent->GetFlags() & FL_WATERJUMP))
		{
			SV_AddGravity(ent);
		}
		SV_CheckStuck(ent);
		SV_WalkMove(ent);
		break;

	case QHMOVETYPE_TOSS:
	case QHMOVETYPE_BOUNCE:
		SV_Physics_Toss(ent);
		break;

	case QHMOVETYPE_FLY:
	case H2MOVETYPE_SWIM:
		if (!SV_RunThink(ent))
		{
			return;
		}
		SV_CheckWater(ent);
		SV_FlyMove(ent, host_frametime, NULL);
		SV_FlyExtras(ent, host_frametime, NULL);	// Hover & friction
		break;

	case QHMOVETYPE_NOCLIP:
		if (!SV_RunThink(ent))
		{
			return;
		}
		VectorMA(ent->GetOrigin(), host_frametime, ent->GetVelocity(), ent->GetOrigin());
		break;

	default:
		Sys_Error("SV_Physics_client: bad movetype %i", (int)ent->GetMoveType());
	}

//
// call standard player post-think
//
	SV_LinkEdict(ent, true);

	pr_global_struct->time = sv.time;
	pr_global_struct->self = EDICT_TO_PROG(ent);
	PR_ExecuteProgram(pr_global_struct->PlayerPostThink);
}

//============================================================================

/*
=============
SV_Physics_None

Non moving objects can only think
=============
*/
void SV_Physics_None(qhedict_t* ent)
{
// regular thinking
	SV_RunThink(ent);
}

/*
=============
SV_Physics_Noclip

A moving object that doesn't obey physics
=============
*/
void SV_Physics_Noclip(qhedict_t* ent)
{
// regular thinking
	if (!SV_RunThink(ent))
	{
		return;
	}

	VectorMA(ent->GetAngles(), host_frametime, ent->GetAVelocity(), ent->GetAngles());
	VectorMA(ent->GetOrigin(), host_frametime, ent->GetVelocity(), ent->GetOrigin());

	SV_LinkEdict(ent, false);
}

/*
==============================================================================

TOSS / BOUNCE

==============================================================================
*/

/*
=============
SV_CheckWaterTransition

=============
*/
void SV_CheckWaterTransition(qhedict_t* ent)
{
	int cont;
	cont = SV_PointContents(ent->GetOrigin());
	if (!ent->GetWaterType())
	{	// just spawned here
		ent->SetWaterType(cont);
		ent->SetWaterLevel(1);
		return;
	}

	if (cont <= BSP29CONTENTS_WATER)
	{
		if (ent->GetWaterType() == BSP29CONTENTS_EMPTY)
		{	// just crossed into water
			SV_StartSound(ent, 0, "misc/hith2o.wav", 255, 1);
		}
		ent->SetWaterType(cont);
		ent->SetWaterLevel(1);
	}
	else
	{
		if (ent->GetWaterType() != BSP29CONTENTS_EMPTY)
		{	// just crossed into water
			SV_StartSound(ent, 0, "misc/hith2o.wav", 255, 1);
		}
		ent->SetWaterType(BSP29CONTENTS_EMPTY);
		ent->SetWaterLevel(cont);
	}
}

/*
=============
SV_Physics_Toss

Toss, bounce, and fly movement.  When onground, do nothing.
=============
*/

void SV_Physics_Toss(qhedict_t* ent)
{
	q1trace_t trace;
	vec3_t move;
	float backoff;
	// regular thinking
	if (!SV_RunThink(ent))
	{
		return;
	}

// if onground, return without moving
	if (((int)ent->GetFlags() & FL_ONGROUND))
	{
		return;
	}

	SV_CheckVelocity(ent);

// add gravity
	if (ent->GetMoveType() != QHMOVETYPE_FLY &&
		ent->GetMoveType() != H2MOVETYPE_BOUNCEMISSILE &&
		ent->GetMoveType() != QHMOVETYPE_FLYMISSILE &&
		ent->GetMoveType() != H2MOVETYPE_SWIM)
	{
		SV_AddGravity(ent);
	}

// move angles
	VectorMA(ent->GetAngles(), host_frametime, ent->GetAVelocity(), ent->GetAngles());

// move origin
	VectorScale(ent->GetVelocity(), host_frametime, move);
//	VectorCopy(vec_origin,move);
	trace = SV_PushEntity(ent, move);
	if (trace.fraction == 1)
	{
		return;
	}
	if (ent->free)
	{
		return;
	}

	if (ent->GetMoveType() == QHMOVETYPE_BOUNCE)
	{
		backoff = 1.5;
	}
	else if (ent->GetMoveType() == H2MOVETYPE_BOUNCEMISSILE)
	{	// Solid phased missiles don't bounce on monsters or players
		if ((ent->GetSolid() == SOLID_PHASE) && (((int)EDICT_NUM(trace.entityNum)->GetFlags() & FL_MONSTER) || ((int)EDICT_NUM(trace.entityNum)->GetMoveType() == QHMOVETYPE_WALK)))
		{
			return;
		}
		backoff = 2.0;
	}
	else
	{
		backoff = 1;
	}

	PM_ClipVelocity(ent->GetVelocity(), trace.plane.normal, ent->GetVelocity(), backoff);

// stop if on ground
	if ((trace.plane.normal[2] > 0.7) && (ent->GetMoveType() != H2MOVETYPE_BOUNCEMISSILE))
	{
		if (ent->GetVelocity()[2] < 60 || ent->GetMoveType() != QHMOVETYPE_BOUNCE)
		{
			ent->SetFlags((int)ent->GetFlags() | FL_ONGROUND);
			ent->SetGroundEntity(EDICT_TO_PROG(EDICT_NUM(trace.entityNum)));
			VectorCopy(vec3_origin, ent->GetVelocity());
			ent->SetAVelocity(vec3_origin);
		}
	}

// check for in water
	SV_CheckWaterTransition(ent);
}

/*
===============================================================================

STEPPING MOVEMENT

===============================================================================
*/

/*
=============
SV_Physics_Step

Monsters freefall when they don't have a ground entity, otherwise
all movement is done with discrete steps.

This is also used for objects that have become still on the ground, but
will fall if the floor is pulled out from under them.
=============
*/
void SV_Physics_Step(qhedict_t* ent)
{
	qboolean hitsound;

// freefall if not onground
	if (!((int)ent->GetFlags() & (FL_ONGROUND | FL_FLY | FL_SWIM)))
	{
		if (ent->GetVelocity()[2] < sv_gravity->value * -0.1)
		{
			hitsound = true;
		}
		else
		{
			hitsound = false;
		}

		SV_AddGravity(ent);
		SV_CheckVelocity(ent);
		SV_FlyMove(ent, host_frametime, NULL);
		SV_LinkEdict(ent, true);

//		if ( (int)ent->v.flags & FL_ONGROUND )	// just hit ground
		if (((int)ent->GetFlags() & FL_ONGROUND) && (!ent->GetFlags() & FL_MONSTER))
		{
			if (hitsound)
			{
				SV_StartSound(ent, 0, "fx/thngland.wav", 255, 1);
			}
		}
	}

// regular thinking
	SV_RunThink(ent);

	SV_CheckWaterTransition(ent);
}

//============================================================================

/*
================
SV_Physics

================
*/
void SV_Physics(void)
{
	int i,c,originMoved;
	qhedict_t* ent, * ent2;
	vec3_t oldOrigin,oldAngle;

// let the progs know that a new frame has started
	pr_global_struct->self = EDICT_TO_PROG(sv.edicts);
	pr_global_struct->other = EDICT_TO_PROG(sv.edicts);
	pr_global_struct->time = sv.time;
	PR_ExecuteProgram(pr_global_struct->StartFrame);

//SV_CheckAllEnts ();

//
// treat each object in turn
//
	ent = sv.edicts;
	for (i = 0; i < sv.num_edicts; i++, ent = NEXT_EDICT(ent))
	{
		if (ent->free)
		{
			continue;
		}

		ent2 = PROG_TO_EDICT(ent->GetMoveChain());
		if (ent2 != sv.edicts)
		{
			VectorCopy(ent->GetOrigin(),oldOrigin);
			VectorCopy(ent->GetAngles(),oldAngle);
		}

		if (pr_global_struct->force_retouch)
		{
			SV_LinkEdict(ent, true);	// force retouch even for stationary
		}

		if (i > 0 && i <= svs.maxclients)
		{
			SV_Physics_Client(ent, i);
		}
		else if (ent->GetMoveType() == QHMOVETYPE_PUSH)
		{
			SV_Physics_Pusher(ent);
		}
		else if (ent->GetMoveType() == QHMOVETYPE_NONE)
		{
			SV_Physics_None(ent);
		}
		else if (ent->GetMoveType() == QHMOVETYPE_NOCLIP)
		{
			SV_Physics_Noclip(ent);
		}
		else if ((ent->GetMoveType() == QHMOVETYPE_STEP) ||
				 (ent->GetMoveType() == H2MOVETYPE_PUSHPULL))
		{
			SV_Physics_Step(ent);
		}
		else if (ent->GetMoveType() == QHMOVETYPE_TOSS ||
				 ent->GetMoveType() == QHMOVETYPE_BOUNCE ||
				 ent->GetMoveType() == H2MOVETYPE_BOUNCEMISSILE ||
				 ent->GetMoveType() == QHMOVETYPE_FLY ||
				 ent->GetMoveType() == QHMOVETYPE_FLYMISSILE ||
				 ent->GetMoveType() == H2MOVETYPE_SWIM)
		{
			SV_Physics_Toss(ent);
		}
		else
		{
			Sys_Error("SV_Physics: bad movetype %i", (int)ent->GetMoveType());
		}

		if (ent2 != sv.edicts)
		{
			originMoved = !VectorCompare(ent->GetOrigin(),oldOrigin);
			if (originMoved || !VectorCompare(ent->GetAngles(),oldAngle))
			{
				VectorSubtract(ent->GetOrigin(),oldOrigin,oldOrigin);
				VectorSubtract(ent->GetAngles(),oldAngle,oldAngle);

				for (c = 0; c < 10; c++)
				{	// chain a max of 10 objects
					if (ent2->free)
					{
						break;
					}

					VectorAdd(oldOrigin,ent2->GetOrigin(),ent2->GetOrigin());
					if ((int)ent2->GetFlags() & FL_MOVECHAIN_ANGLE)
					{
						VectorAdd(oldAngle,ent2->GetAngles(),ent2->GetAngles());
					}

					if (originMoved && ent2->GetChainMoved())
					{	// callback function
						pr_global_struct->self = EDICT_TO_PROG(ent2);
						pr_global_struct->other = EDICT_TO_PROG(ent);
						PR_ExecuteProgram(ent2->GetChainMoved());
					}

					ent2 = PROG_TO_EDICT(ent2->GetMoveChain());
					if (ent2 == sv.edicts)
					{
						break;
					}

				}
			}
		}
	}

	if (pr_global_struct->force_retouch)
	{
		pr_global_struct->force_retouch--;
	}

	sv.time += host_frametime;
}
