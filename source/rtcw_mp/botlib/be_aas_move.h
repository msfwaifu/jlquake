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


/*****************************************************************************
 * name:		be_aas_move.h
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

//movement prediction
int AAS_PredictClientMovement(struct aas_clientmove_s* move,
	int entnum, vec3_t origin,
	int presencetype, int onground,
	vec3_t velocity, vec3_t cmdmove,
	int cmdframes,
	int maxframes, float frametime,
	int stopevent, int stopareanum, int visualize);
//returns true if on the ground at the given origin
int AAS_OnGround(vec3_t origin, int presencetype, int passent);
//returns true if swimming at the given origin
int AAS_Swimming(vec3_t origin);
//returns the jump reachability run start point
void AAS_JumpReachRunStart(aas_reachability_t* reach, vec3_t runstart);
//rocket jump Z velocity when rocket-jumping at origin
float AAS_RocketJumpZVelocity(vec3_t origin);
//bfg jump Z velocity when bfg-jumping at origin
float AAS_BFGJumpZVelocity(vec3_t origin);
//
int AAS_DropToFloor(vec3_t origin, vec3_t mins, vec3_t maxs);
