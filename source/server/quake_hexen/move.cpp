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

#include "../progsvm/progsvm.h"
#include "local.h"
#include "../../common/common_defs.h"
#include "../../common/player_move.h"
#include "../../common/content_types.h"

//	Returns false if any part of the bottom of the entity is off an edge that
// is not a staircase.
bool SVQH_CheckBottom( qhedict_t* ent ) {
	vec3_t mins;
	VectorAdd( ent->GetOrigin(), ent->GetMins(), mins );
	vec3_t maxs;
	VectorAdd( ent->GetOrigin(), ent->GetMaxs(), maxs );

	// if all of the points under the corners are solid world, don't bother
	// with the tougher checks
	// the corners must be within 16 of the midpoint
	vec3_t start;
	start[ 2 ] = mins[ 2 ] - 1;
	for ( int x = 0; x <= 1; x++ ) {
		for ( int y = 0; y <= 1; y++ ) {
			start[ 0 ] = x ? maxs[ 0 ] : mins[ 0 ];
			start[ 1 ] = y ? maxs[ 1 ] : mins[ 1 ];
			if ( SVQH_PointContents( start ) != BSP29CONTENTS_SOLID ) {
				goto realcheck;
			}
		}
	}

	// we got out easy
	return true;

realcheck:
	// check it for real...
	start[ 2 ] = mins[ 2 ];

	// the midpoint must be within 16 of the bottom
	vec3_t stop;
	start[ 0 ] = stop[ 0 ] = ( mins[ 0 ] + maxs[ 0 ] ) * 0.5;
	start[ 1 ] = stop[ 1 ] = ( mins[ 1 ] + maxs[ 1 ] ) * 0.5;
	stop[ 2 ] = start[ 2 ] - 2 * STEPSIZE;
	q1trace_t trace = SVQH_MoveHull0( start, oldvec3_origin, oldvec3_origin, stop, true, ent );

	if ( trace.fraction == 1.0 ) {
		return false;
	}
	float mid = trace.endpos[ 2 ];
	float bottom = mid;

	// the corners must be within 16 of the midpoint
	for ( int x = 0; x <= 1; x++ ) {
		for ( int y = 0; y <= 1; y++ ) {
			start[ 0 ] = stop[ 0 ] = x ? maxs[ 0 ] : mins[ 0 ];
			start[ 1 ] = stop[ 1 ] = y ? maxs[ 1 ] : mins[ 1 ];

			trace = SVQH_MoveHull0( start, oldvec3_origin, oldvec3_origin, stop, true, ent );

			if ( trace.fraction != 1.0 && trace.endpos[ 2 ] > bottom ) {
				bottom = trace.endpos[ 2 ];
			}
			if ( trace.fraction == 1.0 || mid - trace.endpos[ 2 ] > STEPSIZE ) {
				return false;
			}
		}
	}

	return true;
}

void SVQH_SetMoveTrace( const q1trace_t& trace ) {
	*pr_globalVars.trace_allsolid = trace.allsolid;
	*pr_globalVars.trace_startsolid = trace.startsolid;
	*pr_globalVars.trace_fraction = trace.fraction;
	*pr_globalVars.trace_inwater = trace.inwater;
	*pr_globalVars.trace_inopen = trace.inopen;
	VectorCopy( trace.endpos, pr_globalVars.trace_endpos );
	VectorCopy( trace.plane.normal, pr_globalVars.trace_plane_normal );
	*pr_globalVars.trace_plane_dist =  trace.plane.dist;
	if ( trace.entityNum >= 0 ) {
		*pr_globalVars.trace_ent = EDICT_TO_PROG( QH_EDICT_NUM( trace.entityNum ) );
	} else {
		*pr_globalVars.trace_ent = EDICT_TO_PROG( sv.qh_edicts );
	}
}

//	Called by monster program code.
//	The move will be adjusted for slopes and stairs, but if the move isn't
// possible, no move is done, false is returned, and
// pr_global_struct->trace_normal is set to the normal of the blocking wall
bool SVQH_movestep( qhedict_t* ent, const vec3_t move, bool relink, bool noenemy, bool set_trace ) {
	// try the move
	vec3_t oldorg;
	VectorCopy( ent->GetOrigin(), oldorg );
	vec3_t neworg;
	VectorAdd( ent->GetOrigin(), move, neworg );

	// flying monsters don't step up, unless no_z turned on
	if ( ( ( int )ent->GetFlags() & ( QHFL_SWIM | QHFL_FLY ) ) &&
		 ( !( GGameType & GAME_Hexen2 ) || GGameType & GAME_HexenWorld ||
		   ( !( ( int )ent->GetFlags() & H2FL_NOZ ) && !( ( int )ent->GetFlags() & H2FL_HUNTFACE ) ) ) ) {
		// try one move with vertical motion, then one without
		for ( int i = 0; i < 2; i++ ) {
			VectorAdd( ent->GetOrigin(), move, neworg );
			qhedict_t* enemy;
			if ( !noenemy ) {
				enemy = PROG_TO_EDICT( ent->GetEnemy() );
				if ( i == 0 && enemy != sv.qh_edicts ) {
					float dz;
					if ( GGameType & GAME_Hexen2 && !( GGameType & GAME_HexenWorld ) &&
						 ( int )ent->GetFlags() & H2FL_HUNTFACE ) {
						//Go for face
						dz = ent->GetOrigin()[ 2 ] - PROG_TO_EDICT( ent->GetEnemy() )->GetOrigin()[ 2 ] + PROG_TO_EDICT( ent->GetEnemy() )->GetViewOfs()[ 2 ];
					} else {
						dz = ent->GetOrigin()[ 2 ] - PROG_TO_EDICT( ent->GetEnemy() )->GetOrigin()[ 2 ];
					}
					if ( dz > 40 ) {
						neworg[ 2 ] -= 8;
					} else if ( dz < 30 ) {
						neworg[ 2 ] += 8;
					}
				}
			}

			bool swimOutOfWater = GGameType & GAME_Hexen2 && !( GGameType & GAME_HexenWorld ) &&
								  ( ( int )ent->GetFlags() & QHFL_SWIM ) && SVQH_PointContents( neworg ) == BSP29CONTENTS_EMPTY;
			if ( swimOutOfWater ) {
				//Would end up out of water, don't do z move
				neworg[ 2 ] = ent->GetOrigin()[ 2 ];
			}
			q1trace_t trace = SVQH_Move( ent->GetOrigin(), ent->GetMins(), ent->GetMaxs(), neworg, false, ent );
			if ( set_trace ) {
				SVQH_SetMoveTrace( trace );
			}
			if ( swimOutOfWater && ( trace.fraction < 1 || SVQH_PointContents( trace.endpos ) == BSP29CONTENTS_EMPTY ) ) {
				// swim monster left water
				return false;
			}

			if ( trace.fraction == 1 ) {
				if ( ( !( GGameType & GAME_Hexen2 ) || GGameType & GAME_HexenWorld ) &&
					 ( ( int )ent->GetFlags() & QHFL_SWIM ) && SVQH_PointContents( trace.endpos ) == BSP29CONTENTS_EMPTY ) {
					// swim monster left water
					return false;
				}
				VectorCopy( trace.endpos, ent->GetOrigin() );
				if ( relink ) {
					SVQH_LinkEdict( ent, true );
				}
				return true;
			}

			if ( noenemy || enemy == sv.qh_edicts ) {
				break;
			}
		}

		return false;
	}

	// push down from a step height above the wished position
	neworg[ 2 ] += STEPSIZE;
	vec3_t end;
	VectorCopy( neworg, end );
	end[ 2 ] -= STEPSIZE * 2;

	q1trace_t trace = SVQH_Move( neworg, ent->GetMins(), ent->GetMaxs(), end, false, ent );
	if ( set_trace ) {
		SVQH_SetMoveTrace( trace );
	}

	if ( trace.allsolid ) {
		return false;
	}

	if ( trace.startsolid ) {
		neworg[ 2 ] -= STEPSIZE;
		trace = SVQH_Move( neworg, ent->GetMins(), ent->GetMaxs(), end, false, ent );
		if ( set_trace ) {
			SVQH_SetMoveTrace( trace );
		}
		if ( trace.allsolid || trace.startsolid ) {
			return false;
		}
	}
	if ( trace.fraction == 1 ) {
		// if monster had the ground pulled out, go ahead and fall
		if ( ( int )ent->GetFlags() & QHFL_PARTIALGROUND ) {
			VectorAdd( ent->GetOrigin(), move, ent->GetOrigin() );
			if ( relink ) {
				SVQH_LinkEdict( ent, true );
			}
			ent->SetFlags( ( int )ent->GetFlags() & ~QHFL_ONGROUND );
			return true;
		}

		// walked off an edge
		return false;
	}

	// check point traces down for dangling corners
	VectorCopy( trace.endpos, ent->GetOrigin() );

	if ( !SVQH_CheckBottom( ent ) ) {
		if ( ( int )ent->GetFlags() & QHFL_PARTIALGROUND ) {
			// entity had floor mostly pulled out from underneath it
			// and is trying to correct
			if ( relink ) {
				SVQH_LinkEdict( ent, true );
			}
			return true;
		}
		VectorCopy( oldorg, ent->GetOrigin() );
		return false;
	}

	if ( ( int )ent->GetFlags() & QHFL_PARTIALGROUND ) {
		ent->SetFlags( ( int )ent->GetFlags() & ~QHFL_PARTIALGROUND );
	}
	ent->SetGroundEntity( EDICT_TO_PROG( QH_EDICT_NUM( trace.entityNum ) ) );

	// the move is ok
	if ( relink ) {
		SVQH_LinkEdict( ent, true );
	}
	return true;
}

//	Turns to the movement direction, and walks the current distance if
// facing it.
static bool SVQH_StepDirection( qhedict_t* ent, float yaw, float dist ) {
	vec3_t move, oldorigin;
	float delta;

	ent->SetIdealYaw( yaw );
	PF_changeyaw();

	yaw = DEG2RAD( yaw );
	move[ 0 ] = cos( yaw ) * dist;
	move[ 1 ] = sin( yaw ) * dist;
	move[ 2 ] = 0;

	VectorCopy( ent->GetOrigin(), oldorigin );
	bool set_trace_plane = GGameType & GAME_Hexen2 && !( GGameType & GAME_HexenWorld ) &&
						   ( int )ent->GetFlags() & H2FL_SET_TRACE;

	if ( SVQH_movestep( ent, move, false, false, set_trace_plane ) ) {
		delta = ent->GetAngles()[ YAW ] - ent->GetIdealYaw();
		if ( delta > 45 && delta < 315 ) {	// not turned far enough, so don't take the step
			VectorCopy( oldorigin, ent->GetOrigin() );
		}
		SVQH_LinkEdict( ent, true );
		return true;
	}
	SVQH_LinkEdict( ent, true );

	return false;
}

static void SVQH_FixCheckBottom( qhedict_t* ent ) {
	ent->SetFlags( ( int )ent->GetFlags() | QHFL_PARTIALGROUND );
}

static void SVQH_NewChaseDir( qhedict_t* actor, qhedict_t* enemy, float dist ) {
	enum { DI_NODIR = -1 };

	float olddir = AngleMod( ( int )( actor->GetIdealYaw() / 45 ) * 45 );
	float turnaround = AngleMod( olddir - 180 );

	float deltax = enemy->GetOrigin()[ 0 ] - actor->GetOrigin()[ 0 ];
	float deltay = enemy->GetOrigin()[ 1 ] - actor->GetOrigin()[ 1 ];
	float d[ 3 ];
	if ( deltax > 10 ) {
		d[ 1 ] = 0;
	} else if ( deltax < -10 ) {
		d[ 1 ] = 180;
	} else {
		d[ 1 ] = DI_NODIR;
	}
	if ( deltay < -10 ) {
		d[ 2 ] = 270;
	} else if ( deltay > 10 ) {
		d[ 2 ] = 90;
	} else {
		d[ 2 ] = DI_NODIR;
	}

	// try direct route
	if ( d[ 1 ] != DI_NODIR && d[ 2 ] != DI_NODIR ) {
		float tdir;
		if ( d[ 1 ] == 0 ) {
			tdir = d[ 2 ] == 90 ? 45 : 315;
		} else {
			tdir = d[ 2 ] == 90 ? 135 : 215;
		}

		if ( tdir != turnaround && SVQH_StepDirection( actor, tdir, dist ) ) {
			return;
		}
	}

	// try other directions
	if ( ( ( rand() & 3 ) & 1 ) ||  abs( deltay ) > abs( deltax ) ) {
		float tdir = d[ 1 ];
		d[ 1 ] = d[ 2 ];
		d[ 2 ] = tdir;
	}

	if ( d[ 1 ] != DI_NODIR && d[ 1 ] != turnaround &&
		 SVQH_StepDirection( actor, d[ 1 ], dist ) ) {
		return;
	}

	if ( d[ 2 ] != DI_NODIR && d[ 2 ] != turnaround &&
		 SVQH_StepDirection( actor, d[ 2 ], dist ) ) {
		return;
	}

	//	there is no direct path to the player, so pick another direction
	if ( olddir != DI_NODIR && SVQH_StepDirection( actor, olddir, dist ) ) {
		return;
	}

	//	randomly determine direction of search
	if ( rand() & 1 ) {
		for ( float tdir = 0; tdir <= 315; tdir += 45 ) {
			if ( tdir != turnaround && SVQH_StepDirection( actor, tdir, dist ) ) {
				return;
			}
		}
	} else {
		for ( float tdir = 315; tdir >= 0; tdir -= 45 ) {
			if ( tdir != turnaround && SVQH_StepDirection( actor, tdir, dist ) ) {
				return;
			}
		}
	}

	if ( turnaround != DI_NODIR && SVQH_StepDirection( actor, turnaround, dist ) ) {
		return;
	}

	// can't move
	actor->SetIdealYaw( olddir );

	// if a bridge was pulled out from underneath a monster, it may not have
	// a valid standing position at all

	if ( !SVQH_CheckBottom( actor ) ) {
		SVQH_FixCheckBottom( actor );
	}
}

static bool SVQH_CloseEnough( qhedict_t* ent, qhedict_t* goal, float dist ) {
	for ( int i = 0; i < 3; i++ ) {
		if ( goal->v.absmin[ i ] > ent->v.absmax[ i ] + dist ) {
			return false;
		}
		if ( goal->v.absmax[ i ] < ent->v.absmin[ i ] - dist ) {
			return false;
		}
	}
	return true;
}

void SVQH_MoveToGoal() {
	qhedict_t* ent = PROG_TO_EDICT( *pr_globalVars.self );
	qhedict_t* goal = PROG_TO_EDICT( ent->GetGoalEntity() );
	float dist = G_FLOAT( OFS_PARM0 );

	if ( GGameType & GAME_Hexen2 && !( GGameType & GAME_HexenWorld ) ) {
		VectorCopy( oldvec3_origin, pr_globalVars.trace_plane_normal );
	}
	if ( !( ( int )ent->GetFlags() & ( QHFL_ONGROUND | QHFL_FLY | QHFL_SWIM ) ) ) {
		G_FLOAT( OFS_RETURN ) = 0;
		return;
	}

	// if the next step hits the enemy, return immediately
	if ( PROG_TO_EDICT( ent->GetEnemy() ) != sv.qh_edicts &&  SVQH_CloseEnough( ent, goal, dist ) ) {
		if ( GGameType & GAME_Hexen2 && !( GGameType & GAME_HexenWorld ) ) {
			G_FLOAT( OFS_RETURN ) = 0;
		}
		return;
	}

	// bump around...
	if ( !SVQH_StepDirection( ent, ent->GetIdealYaw(), dist ) ) {
		SVQH_NewChaseDir( ent, goal, dist );
		if ( GGameType & GAME_Hexen2 && !( GGameType & GAME_HexenWorld ) ) {
			G_FLOAT( OFS_RETURN ) = 0;
		}
	} else {
		if ( ( rand() & 3 ) == 1 ) {
			SVQH_NewChaseDir( ent, goal, dist );
		}
		if ( GGameType & GAME_Hexen2 && !( GGameType & GAME_HexenWorld ) ) {
			G_FLOAT( OFS_RETURN ) = 1;
		}
	}
}
