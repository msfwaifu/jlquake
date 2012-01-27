/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cl_ents.c -- entity parsing and management

#include "quakedef.h"

extern	Cvar*	cl_predict_players;
extern	Cvar*	cl_predict_players2;
extern	Cvar*	cl_solid_players;

static struct predicted_player {
	int flags;
	qboolean active;
	vec3_t origin;	// predicted origin
} predicted_players[MAX_CLIENTS_QW];

/*
=========================================================================

PACKET ENTITY PARSING / LINKING

=========================================================================
*/

static void R_HandlePlayerSkin(refEntity_t* Ent, int PlayerNum)
{
	// we can't dynamically colormap textures, so they are cached
	// seperately for the players.  Heads are just uncolored.
	if (!cl.q1_players[PlayerNum].skin)
	{
		CLQW_SkinFind(&cl.q1_players[PlayerNum]);
		CLQ1_TranslatePlayerSkin(PlayerNum);
	}
	Ent->customSkin = R_GetImageHandle(clq1_playertextures[PlayerNum]);
}

/*
===============
CL_LinkPacketEntities

===============
*/
void CL_LinkPacketEntities (void)
{
	qwpacket_entities_t	*pack;
	q1entity_state_t		*s1, *s2;
	float				f;
	qhandle_t			model;
	vec3_t				old_origin;
	float				autorotate;
	int					i;
	int					pnum;

	pack = &cl.qw_frames[clc.netchan.incomingSequence&UPDATE_MASK_QW].packet_entities;
	qwpacket_entities_t* PrevPack = &cl.qw_frames[(clc.netchan.incomingSequence - 1) & UPDATE_MASK_QW].packet_entities;

	autorotate = AngleMod(100*cl.qh_serverTimeFloat);

	f = 0;		// FIXME: no interpolation right now

	for (pnum=0 ; pnum<pack->num_entities ; pnum++)
	{
		s1 = &pack->entities[pnum];
		s2 = s1;	// FIXME: no interpolation right now

		// spawn light flashes, even ones coming from invisible objects
		if ((s1->effects & (QWEF_BLUE | QWEF_RED)) == (QWEF_BLUE | QWEF_RED))
			CLQ1_DimLight (s1->number, s1->origin, 3);
		else if (s1->effects & QWEF_BLUE)
			CLQ1_DimLight (s1->number, s1->origin, 1);
		else if (s1->effects & QWEF_RED)
			CLQ1_DimLight (s1->number, s1->origin, 2);
		else if (s1->effects & Q1EF_BRIGHTLIGHT)
			CLQ1_BrightLight(s1->number, s1->origin);
		else if (s1->effects & Q1EF_DIMLIGHT)
			CLQ1_DimLight (s1->number, s1->origin, 0);

		// if set to invisible, skip
		if (!s1->modelindex)
			continue;

		// create a new entity
		refEntity_t ent;
		Com_Memset(&ent, 0, sizeof(ent));
		ent.reType = RT_MODEL;

		model = cl.model_draw[s1->modelindex];
		ent.hModel = model;
	
		// set colormap
		if (s1->colormap && (s1->colormap < MAX_CLIENTS_QW) && s1->modelindex == clq1_playerindex)
		{
			R_HandlePlayerSkin(&ent, s1->colormap - 1);
		}

		// set skin
		ent.skinNum = s1->skinnum;
		
		// set frame
		ent.frame = s1->frame;

		int ModelFlags = R_ModelFlags(model);
		// rotate binary objects locally
		vec3_t angles;
		if (ModelFlags & Q1MDLEF_ROTATE)
		{
			angles[0] = 0;
			angles[1] = autorotate;
			angles[2] = 0;
		}
		else
		{
			float	a1, a2;

			for (i=0 ; i<3 ; i++)
			{
				a1 = s1->angles[i];
				a2 = s2->angles[i];
				if (a1 - a2 > 180)
					a1 -= 360;
				if (a1 - a2 < -180)
					a1 += 360;
				angles[i] = a2 + f * (a1 - a2);
			}
		}
		CLQ1_SetRefEntAxis(&ent, angles);

		// calculate origin
		for (i=0 ; i<3 ; i++)
			ent.origin[i] = s2->origin[i] + 
			f * (s1->origin[i] - s2->origin[i]);
		R_AddRefEntityToScene(&ent);

		// add automatic particle trails
		if (!ModelFlags)
			continue;

		// scan the old entity display list for a matching
		for (i = 0; i < PrevPack->num_entities; i++)
		{
			if (PrevPack->entities[i].number == s1->number)
			{
				VectorCopy(PrevPack->entities[i].origin, old_origin);
				break;
			}
		}
		if (i == PrevPack->num_entities)
		{
			continue;		// not in last message
		}

		for (i=0 ; i<3 ; i++)
			if ( abs(old_origin[i] - ent.origin[i]) > 128)
			{	// no trail if too far
				VectorCopy (ent.origin, old_origin);
				break;
			}
		if (ModelFlags & Q1MDLEF_ROCKET)
		{
			CLQ1_TrailParticles (old_origin, ent.origin, 0);
			CLQ1_RocketLight(s1->number, ent.origin);
		}
		else if (ModelFlags & Q1MDLEF_GRENADE)
			CLQ1_TrailParticles (old_origin, ent.origin, 1);
		else if (ModelFlags & Q1MDLEF_GIB)
			CLQ1_TrailParticles (old_origin, ent.origin, 2);
		else if (ModelFlags & Q1MDLEF_ZOMGIB)
			CLQ1_TrailParticles (old_origin, ent.origin, 4);
		else if (ModelFlags & Q1MDLEF_TRACER)
			CLQ1_TrailParticles (old_origin, ent.origin, 3);
		else if (ModelFlags & Q1MDLEF_TRACER2)
			CLQ1_TrailParticles (old_origin, ent.origin, 5);
		else if (ModelFlags & Q1MDLEF_TRACER3)
			CLQ1_TrailParticles (old_origin, ent.origin, 6);
	}
}

//========================================


/*
================
CL_AddFlagModels

Called when the CTF flags are set
================
*/
void CL_AddFlagModels (refEntity_t *ent, int team, vec3_t angles)
{
	int		i;
	float	f;
	vec3_t	v_forward, v_right, v_up;

	if (cl_flagindex == -1)
		return;

	f = 14;
	if (ent->frame >= 29 && ent->frame <= 40) {
		if (ent->frame >= 29 && ent->frame <= 34) { //axpain
			if      (ent->frame == 29) f = f + 2; 
			else if (ent->frame == 30) f = f + 8;
			else if (ent->frame == 31) f = f + 12;
			else if (ent->frame == 32) f = f + 11;
			else if (ent->frame == 33) f = f + 10;
			else if (ent->frame == 34) f = f + 4;
		} else if (ent->frame >= 35 && ent->frame <= 40) { // pain
			if      (ent->frame == 35) f = f + 2; 
			else if (ent->frame == 36) f = f + 10;
			else if (ent->frame == 37) f = f + 10;
			else if (ent->frame == 38) f = f + 8;
			else if (ent->frame == 39) f = f + 4;
			else if (ent->frame == 40) f = f + 2;
		}
	} else if (ent->frame >= 103 && ent->frame <= 118) {
		if      (ent->frame >= 103 && ent->frame <= 104) f = f + 6;  //nailattack
		else if (ent->frame >= 105 && ent->frame <= 106) f = f + 6;  //light 
		else if (ent->frame >= 107 && ent->frame <= 112) f = f + 7;  //rocketattack
		else if (ent->frame >= 112 && ent->frame <= 118) f = f + 7;  //shotattack
	}

	refEntity_t newent;
	Com_Memset(&newent, 0, sizeof(newent));

	newent.reType = RT_MODEL;
	newent.hModel = cl.model_draw[cl_flagindex];
	newent.skinNum = team;

	AngleVectors(angles, v_forward, v_right, v_up);
	v_forward[2] = -v_forward[2]; // reverse z component
	for (i=0 ; i<3 ; i++)
		newent.origin[i] = ent->origin[i] - f*v_forward[i] + 22*v_right[i];
	newent.origin[2] -= 16;

	vec3_t flag_angles;
	VectorCopy(angles, flag_angles);
	flag_angles[2] -= 45;
	CLQ1_SetRefEntAxis(&newent, flag_angles);
	R_AddRefEntityToScene(&newent);
}

/*
=============
CL_LinkPlayers

Create visible entities in the correct position
for all current players
=============
*/
void CL_LinkPlayers (void)
{
	int				j;
	q1player_info_t	*info;
	qwplayer_state_t	*state;
	qwplayer_state_t	exact;
	double			playertime;
	int				msec;
	qwframe_t			*frame;
	int				oldphysent;

	playertime = realtime - cls.latency + 0.02;
	if (playertime > realtime)
		playertime = realtime;

	frame = &cl.qw_frames[cl.qh_parsecount&UPDATE_MASK_QW];

	for (j=0, info=cl.q1_players, state=frame->playerstate ; j < MAX_CLIENTS_QW 
		; j++, info++, state++)
	{
		if (state->messagenum != cl.qh_parsecount)
			continue;	// not present this frame

		// spawn light flashes, even ones coming from invisible objects
		if ((state->effects & (QWEF_BLUE | QWEF_RED)) == (QWEF_BLUE | QWEF_RED))
			CLQ1_DimLight (j, state->origin, 3);
		else if (state->effects & QWEF_BLUE)
			CLQ1_DimLight (j, state->origin, 1);
		else if (state->effects & QWEF_RED)
			CLQ1_DimLight (j, state->origin, 2);
		else if (state->effects & Q1EF_BRIGHTLIGHT)
			CLQ1_BrightLight(j, state->origin);
		else if (state->effects & Q1EF_DIMLIGHT)
			CLQ1_DimLight (j, state->origin, 0);

		// the player object never gets added
		if (j == cl.playernum)
			continue;

		if (!state->modelindex)
			continue;

		if (!Cam_DrawPlayer(j))
			continue;

		// grab an entity to fill in
		refEntity_t ent;
		Com_Memset(&ent, 0, sizeof(ent));
		ent.reType = RT_MODEL;

		ent.hModel = cl.model_draw[state->modelindex];
		ent.skinNum = state->skinnum;
		ent.frame = state->frame;
		if (state->modelindex == clq1_playerindex)
		{
			// use custom skin
			R_HandlePlayerSkin(&ent, j);
		}

		//
		// angles
		//
		vec3_t angles;
		angles[PITCH] = -state->viewangles[PITCH]/3;
		angles[YAW] = state->viewangles[YAW];
		angles[ROLL] = 0;
		angles[ROLL] = V_CalcRoll(angles, state->velocity)*4;
		CLQ1_SetRefEntAxis(&ent, angles);

		// only predict half the move to minimize overruns
		msec = 500*(playertime - state->state_time);
		if (msec <= 0 || (!cl_predict_players->value && !cl_predict_players2->value))
		{
			VectorCopy (state->origin, ent.origin);
//Con_DPrintf ("nopredict\n");
		}
		else
		{
			// predict players movement
			if (msec > 255)
				msec = 255;
			state->command.msec = msec;
//Con_DPrintf ("predict: %i\n", msec);

			oldphysent = pmove.numphysent;
			CL_SetSolidPlayers (j);
			CL_PredictUsercmd (state, &exact, &state->command, false);
			pmove.numphysent = oldphysent;
			VectorCopy (exact.origin, ent.origin);
		}
		R_AddRefEntityToScene(&ent);

		if (state->effects & QWEF_FLAG1)
			CL_AddFlagModels(&ent, 0, angles);
		else if (state->effects & QWEF_FLAG2)
			CL_AddFlagModels(&ent, 1, angles);
	}
}

//======================================================================

/*
===============
CL_SetSolid

Builds all the pmove physents for the current frame
===============
*/
void CL_SetSolidEntities (void)
{
	int		i;
	qwframe_t	*frame;
	qwpacket_entities_t	*pak;
	q1entity_state_t		*state;

	pmove.physents[0].model = 0;
	VectorCopy (vec3_origin, pmove.physents[0].origin);
	pmove.physents[0].info = 0;
	pmove.numphysent = 1;

	frame = &cl.qw_frames[cl.qh_parsecount &  UPDATE_MASK_QW];
	pak = &frame->packet_entities;

	for (i=0 ; i<pak->num_entities ; i++)
	{
		state = &pak->entities[i];

		if (state->modelindex < 2)
			continue;
		if (!cl.model_clip[state->modelindex])
			continue;
		pmove.physents[pmove.numphysent].model = cl.model_clip[state->modelindex];
		VectorCopy(state->origin, pmove.physents[pmove.numphysent].origin);
		pmove.numphysent++;
	}

}

/*
===
Calculate the new position of players, without other player clipping

We do this to set up real player prediction.
Players are predicted twice, first without clipping other players,
then with clipping against them.
This sets up the first phase.
===
*/
void CL_SetUpPlayerPrediction(qboolean dopred)
{
	int				j;
	qwplayer_state_t	*state;
	qwplayer_state_t	exact;
	double			playertime;
	int				msec;
	qwframe_t			*frame;
	struct predicted_player *pplayer;

	playertime = realtime - cls.latency + 0.02;
	if (playertime > realtime)
		playertime = realtime;

	frame = &cl.qw_frames[cl.qh_parsecount&UPDATE_MASK_QW];

	for (j=0, pplayer = predicted_players, state=frame->playerstate; 
		j < MAX_CLIENTS_QW;
		j++, pplayer++, state++) {

		pplayer->active = false;

		if (state->messagenum != cl.qh_parsecount)
			continue;	// not present this frame

		if (!state->modelindex)
			continue;

		pplayer->active = true;
		pplayer->flags = state->flags;

		// note that the local player is special, since he moves locally
		// we use his last predicted postition
		if (j == cl.playernum) {
			VectorCopy(cl.qw_frames[clc.netchan.outgoingSequence&UPDATE_MASK_QW].playerstate[cl.playernum].origin,
				pplayer->origin);
		} else {
			// only predict half the move to minimize overruns
			msec = 500*(playertime - state->state_time);
			if (msec <= 0 ||
				(!cl_predict_players->value && !cl_predict_players2->value) ||
				!dopred)
			{
				VectorCopy (state->origin, pplayer->origin);
	//Con_DPrintf ("nopredict\n");
			}
			else
			{
				// predict players movement
				if (msec > 255)
					msec = 255;
				state->command.msec = msec;
	//Con_DPrintf ("predict: %i\n", msec);

				CL_PredictUsercmd (state, &exact, &state->command, false);
				VectorCopy (exact.origin, pplayer->origin);
			}
		}
	}
}

/*
===============
CL_SetSolid

Builds all the pmove physents for the current frame
Note that CL_SetUpPlayerPrediction() must be called first!
pmove must be setup with world and solid entity hulls before calling
(via CL_PredictMove)
===============
*/
void CL_SetSolidPlayers (int playernum)
{
	int		j;
	extern	vec3_t	player_mins;
	extern	vec3_t	player_maxs;
	struct predicted_player *pplayer;
	physent_t *pent;

	if (!cl_solid_players->value)
		return;

	pent = pmove.physents + pmove.numphysent;

	for (j=0, pplayer = predicted_players; j < MAX_CLIENTS_QW;	j++, pplayer++) {

		if (!pplayer->active)
			continue;	// not present this frame

		// the player object never gets added
		if (j == playernum)
			continue;

		if (pplayer->flags & QWPF_DEAD)
			continue; // dead players aren't solid

		pent->model = -1;
		VectorCopy(pplayer->origin, pent->origin);
		VectorCopy(player_mins, pent->mins);
		VectorCopy(player_maxs, pent->maxs);
		pmove.numphysent++;
		pent++;
	}
}

/*
===============
CL_EmitEntities

Builds the visedicts array for cl.time

Made up of: clients, packet_entities, nails, and tents
===============
*/
void CL_EmitEntities (void)
{
	if (cls.state != CA_ACTIVE)
		return;
	if (!cl.qh_validsequence)
		return;

	R_ClearScene();

	CL_LinkPlayers ();
	CL_LinkPacketEntities ();
	CLQ1_LinkProjectiles ();
	CLQ1_UpdateTEnts ();
	CLQ1_LinkStaticEntities();
}

