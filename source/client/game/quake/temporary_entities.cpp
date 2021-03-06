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

#include "local.h"
#include "../../client_main.h"
#include "../particles.h"
#include "../dynamic_lights.h"
#include "../../../common/Common.h"
#include "../../../common/common_defs.h"

enum { MAX_BEAMS_Q1 = 24 };

struct q1beam_t {
	int entity;
	qhandle_t model;
	int endtime;
	vec3_t start;
	vec3_t end;
};

enum { MAX_EXPLOSIONS_Q1 = 8 };

struct q1explosion_t {
	vec3_t origin;
	int start;
	qhandle_t model;
};

static q1beam_t clq1_beams[ MAX_BEAMS_Q1 ];
static q1explosion_t clq1_explosions[ MAX_EXPLOSIONS_Q1 ];

static sfxHandle_t clq1_sfx_wizhit;
static sfxHandle_t clq1_sfx_knighthit;
static sfxHandle_t clq1_sfx_tink1;
static sfxHandle_t clq1_sfx_ric1;
static sfxHandle_t clq1_sfx_ric2;
static sfxHandle_t clq1_sfx_ric3;
static sfxHandle_t clq1_sfx_r_exp3;

void CLQ1_InitTEnts() {
	clq1_sfx_wizhit = S_RegisterSound( "wizard/hit.wav" );
	clq1_sfx_knighthit = S_RegisterSound( "hknight/hit.wav" );
	clq1_sfx_tink1 = S_RegisterSound( "weapons/tink1.wav" );
	clq1_sfx_ric1 = S_RegisterSound( "weapons/ric1.wav" );
	clq1_sfx_ric2 = S_RegisterSound( "weapons/ric2.wav" );
	clq1_sfx_ric3 = S_RegisterSound( "weapons/ric3.wav" );
	clq1_sfx_r_exp3 = S_RegisterSound( "weapons/r_exp3.wav" );
}

void CLQ1_ClearTEnts() {
	Com_Memset( clq1_beams, 0, sizeof ( clq1_beams ) );
	Com_Memset( clq1_explosions, 0, sizeof ( clq1_explosions ) );
}

static void CLQ1_ParseBeam( QMsg& message, qhandle_t model ) {
	int entity = message.ReadShort();

	vec3_t start;
	message.ReadPos( start );

	vec3_t end;
	message.ReadPos( end );

	// override any beam with the same entity
	q1beam_t* beam = clq1_beams;
	for ( int i = 0; i < MAX_BEAMS_Q1; i++, beam++ ) {
		if ( beam->entity == entity ) {
			beam->entity = entity;
			beam->model = model;
			beam->endtime = cl.serverTime + 200;
			VectorCopy( start, beam->start );
			VectorCopy( end, beam->end );
			return;
		}
	}

	// find a free beam
	beam = clq1_beams;
	for ( int i = 0; i < MAX_BEAMS_Q1; i++, beam++ ) {
		if ( !beam->model || beam->endtime < cl.serverTime ) {
			beam->entity = entity;
			beam->model = model;
			beam->endtime = cl.serverTime + 200;
			VectorCopy( start, beam->start );
			VectorCopy( end, beam->end );
			return;
		}
	}
	common->Printf( "beam list overflow!\n" );
}

static void CLQ1_UpdateBeams() {
	// update lightning
	q1beam_t* beam = clq1_beams;
	for ( int i = 0; i < MAX_BEAMS_Q1; i++, beam++ ) {
		if ( !beam->model || beam->endtime < cl.serverTime ) {
			continue;
		}

		// if coming from the player, update the start position
		if ( beam->entity == cl.viewentity ) {
			float* org = GGameType & GAME_QuakeWorld ? cl.qh_simorg : clq1_entities[ cl.viewentity ].state.origin;
			VectorCopy( org, beam->start );
		}

		// calculate pitch and yaw
		vec3_t direction;
		VectorSubtract( beam->end, beam->start, direction );

		vec3_t angles;
		VecToAnglesBuggy( direction, angles );

		// add new entities for the lightning
		vec3_t origin;
		VectorCopy( beam->start, origin );
		float distance = VectorNormalize( direction );
		while ( distance > 0 ) {
			refEntity_t entity;
			Com_Memset( &entity, 0, sizeof ( entity ) );
			entity.reType = RT_MODEL;
			VectorCopy( origin, entity.origin );
			entity.hModel = beam->model;
			angles[ 2 ] = rand() % 360;
			CLQ1_SetRefEntAxis( &entity, angles );
			R_AddRefEntityToScene( &entity );

			for ( int j = 0; j < 3; j++ ) {
				origin[ j ] += direction[ j ] * 30;
			}
			distance -= 30;
		}
	}
}

static q1explosion_t* CLQ1_AllocExplosion() {
	for ( int i = 0; i < MAX_EXPLOSIONS_Q1; i++ ) {
		if ( !clq1_explosions[ i ].model ) {
			return &clq1_explosions[ i ];
		}
	}

	// find the oldest explosion
	int time = cl.serverTime;
	int index = 0;
	for ( int i = 0; i < MAX_EXPLOSIONS_Q1; i++ ) {
		if ( clq1_explosions[ i ].start < time ) {
			time = clq1_explosions[ i ].start;
			index = i;
		}
	}
	return &clq1_explosions[ index ];
}

static void CLQ1_ExplosionSprite( vec3_t position ) {
	q1explosion_t* explosion = CLQ1_AllocExplosion();
	VectorCopy( position, explosion->origin );
	explosion->start = cl.serverTime;
	explosion->model = R_RegisterModel( "progs/s_explod.spr" );
}

static void CLQ1_UpdateExplosions() {
	q1explosion_t* explosion = clq1_explosions;
	for ( int i = 0; i < MAX_EXPLOSIONS_Q1; i++, explosion++ ) {
		if ( !explosion->model ) {
			continue;
		}
		int f = 10 * ( cl.serverTime - explosion->start );
		if ( f >= R_ModelNumFrames( explosion->model ) ) {
			explosion->model = 0;
			continue;
		}

		refEntity_t entity;
		Com_Memset( &entity, 0, sizeof ( entity ) );
		entity.reType = RT_MODEL;
		VectorCopy( explosion->origin, entity.origin );
		entity.hModel = explosion->model;
		entity.frame = f;
		CLQ1_SetRefEntAxis( &entity, oldvec3_origin );
		R_AddRefEntityToScene( &entity );
	}
}

static void CLQ1_ParseWizSpike( QMsg& message ) {
	vec3_t position;
	message.ReadPos( position );

	CLQ1_RunParticleEffect( position, oldvec3_origin, 20, 30 );
	S_StartSound( position, -1, 0, clq1_sfx_wizhit, 1, 1 );
}

static void CLQ1_ParseKnightSpike( QMsg& message ) {
	vec3_t position;
	message.ReadPos( position );

	CLQ1_RunParticleEffect( position, oldvec3_origin, 226, 20 );
	S_StartSound( position, -1, 0, clq1_sfx_knighthit, 1, 1 );
}

static void SpikeSound( vec3_t pos ) {
	if ( rand() % 5 ) {
		S_StartSound( pos, -1, 0, clq1_sfx_tink1, 1, 1 );
	} else {
		int rnd = rand() & 3;
		if ( rnd == 1 ) {
			S_StartSound( pos, -1, 0, clq1_sfx_ric1, 1, 1 );
		} else if ( rnd == 2 ) {
			S_StartSound( pos, -1, 0, clq1_sfx_ric2, 1, 1 );
		} else {
			S_StartSound( pos, -1, 0, clq1_sfx_ric3, 1, 1 );
		}
	}
}

static void CLQ1_ParseSpike( QMsg& message ) {
	vec3_t position;
	message.ReadPos( position );

	CLQ1_RunParticleEffect( position, oldvec3_origin, 0, 10 );
	SpikeSound( position );
}

static void CLQ1_ParseSuperSpike( QMsg& message ) {
	vec3_t position;
	message.ReadPos( position );

	CLQ1_RunParticleEffect( position, oldvec3_origin, 0, 20 );
	SpikeSound( position );
}

static void CLQ1_ParseExplosion( QMsg& message ) {
	vec3_t position;
	message.ReadPos( position );

	CLQ1_ParticleExplosion( position );
	CLQ1_ExplosionLight( position );
	S_StartSound( position, -1, 0, clq1_sfx_r_exp3, 1, 1 );
}

static void CLQW_ParseExplosion( QMsg& message ) {
	vec3_t position;
	message.ReadPos( position );

	CLQ1_ParticleExplosion( position );
	CLQ1_ExplosionLight( position );
	S_StartSound( position, -1, 0, clq1_sfx_r_exp3, 1, 1 );
	CLQ1_ExplosionSprite( position );
}

static void CLQ1_ParseTarExplosion( QMsg& message ) {
	vec3_t position;
	message.ReadPos( position );

	CLQ1_BlobExplosion( position );
	S_StartSound( position, -1, 0, clq1_sfx_r_exp3, 1, 1 );
}

static void CLQ1_ParseExplosion2( QMsg& message ) {
	vec3_t position;
	message.ReadPos( position );
	int colourStart = message.ReadByte();
	int colourLength = message.ReadByte();

	CLQ1_ParticleExplosion2( position, colourStart, colourLength );
	CLQ1_ExplosionLight( position );
	S_StartSound( position, -1, 0, clq1_sfx_r_exp3, 1, 1 );
}

static void CLQ1_ParseLavaSplash( QMsg& message ) {
	vec3_t position;
	message.ReadPos( position );

	CLQ1_LavaSplash( position );
}

static void CLQ1_ParseTeleportSplash( QMsg& message ) {
	vec3_t position;
	message.ReadPos( position );

	CLQ1_TeleportSplash( position );
}

static void CLQ1_ParseGunShot( QMsg& message ) {
	vec3_t position;
	message.ReadPos( position );

	CLQ1_RunParticleEffect( position, oldvec3_origin, 0, 20 );
}

static void CLQW_ParseGunShot( QMsg& message ) {
	int count = message.ReadByte();
	vec3_t position;
	message.ReadPos( position );

	CLQ1_RunParticleEffect( position, oldvec3_origin, 0, 20 * count );
}

static void CLQ1_ParseBlood( QMsg& message ) {
	int count = message.ReadByte();
	vec3_t position;
	message.ReadPos( position );

	CLQ1_RunParticleEffect( position, oldvec3_origin, 73, 20 * count );
}

static void CLQ1_ParseLightningBlood( QMsg& message ) {
	vec3_t position;
	message.ReadPos( position );

	CLQ1_RunParticleEffect( position, oldvec3_origin, 225, 50 );
}

static void CLQ1_ParseTEntCommon( QMsg& message, int type ) {
	switch ( type ) {
	case Q1TE_WIZSPIKE:			// spike hitting wall
		CLQ1_ParseWizSpike( message );
		break;

	case Q1TE_KNIGHTSPIKE:		// spike hitting wall
		CLQ1_ParseKnightSpike( message );
		break;

	case Q1TE_SPIKE:			// spike hitting wall
		CLQ1_ParseSpike( message );
		break;

	case Q1TE_SUPERSPIKE:		// super spike hitting wall
		CLQ1_ParseSuperSpike( message );
		break;

	case Q1TE_TAREXPLOSION:		// tarbaby explosion
		CLQ1_ParseTarExplosion( message );
		break;

	case Q1TE_LIGHTNING1:		// lightning bolts
		CLQ1_ParseBeam( message, R_RegisterModel( "progs/bolt.mdl" ) );
		break;

	case Q1TE_LIGHTNING2:		// lightning bolts
		CLQ1_ParseBeam( message, R_RegisterModel( "progs/bolt2.mdl" ) );
		break;

	case Q1TE_LIGHTNING3:		// lightning bolts
		CLQ1_ParseBeam( message, R_RegisterModel( "progs/bolt3.mdl" ) );
		break;

	case Q1TE_LAVASPLASH:
		CLQ1_ParseLavaSplash( message );
		break;

	case Q1TE_TELEPORT:
		CLQ1_ParseTeleportSplash( message );
		break;

	default:
		common->FatalError( "CL_ParseTEnt: bad type" );
	}
}

void CLQ1_ParseTEnt( QMsg& message ) {
	int type = message.ReadByte();
	switch ( type ) {
	case Q1TE_EXPLOSION:		// rocket explosion
		CLQ1_ParseExplosion( message );
		break;

	case Q1TE_EXPLOSION2:		// color mapped explosion
		CLQ1_ParseExplosion2( message );
		break;

	case Q1TE_BEAM:				// grappling hook beam
		CLQ1_ParseBeam( message, R_RegisterModel( "progs/beam.mdl" ) );
		break;

	case Q1TE_GUNSHOT:			// bullet hitting wall
		CLQ1_ParseGunShot( message );
		break;

	default:
		CLQ1_ParseTEntCommon( message, type );
	}
}

void CLQW_ParseTEnt( QMsg& message ) {
	int type = message.ReadByte();
	switch ( type ) {
	case Q1TE_EXPLOSION:		// rocket explosion
		CLQW_ParseExplosion( message );
		break;

	case Q1TE_GUNSHOT:			// bullet hitting wall
		CLQW_ParseGunShot( message );
		break;

	case QWTE_BLOOD:			// bullets hitting body
		CLQ1_ParseBlood( message );
		break;

	case QWTE_LIGHTNINGBLOOD:	// lightning hitting body
		CLQ1_ParseLightningBlood( message );
		break;

	default:
		CLQ1_ParseTEntCommon( message, type );
	}
}

void CLQ1_UpdateTEnts() {
	CLQ1_UpdateBeams();
	CLQ1_UpdateExplosions();
}
