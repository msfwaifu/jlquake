/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// tr_map.c

#include "tr_local.h"

/*

Loads and prepares a map file for scene rendering.

A single entry point:

void RE_LoadWorldMap( const char *name );

*/

extern byte        *fileBase;

int c_gridVerts;

//===============================================================================

void R_ColorShiftLightingBytes( byte in[4], byte out[4] );
void R_LoadLightmaps( bsp46_lump_t *l );
void R_LoadVisibility( bsp46_lump_t *l );
void R_LoadSurfaces( bsp46_lump_t *surfs, bsp46_lump_t *verts, bsp46_lump_t *indexLump );

/*
=================
R_LoadSubmodels
=================
*/
static void R_LoadSubmodels( bsp46_lump_t *l ) {
	bsp46_dmodel_t    *in;
	mbrush46_model_t    *out;
	int i, j, count;

	in = ( bsp46_dmodel_t* )( fileBase + l->fileofs );
	if ( l->filelen % sizeof( *in ) ) {
		ri.Error( ERR_DROP, "LoadMap: funny lump size in %s",s_worldData.name );
	}
	count = l->filelen / sizeof( *in );

	s_worldData.bmodels = out = (mbrush46_model_t*)ri.Hunk_Alloc( count * sizeof( *out ), h_low );

	for ( i = 0 ; i < count ; i++, in++, out++ ) {
		model_t *model;

		model = R_AllocModel();

		assert( model != NULL );            // this should never happen

		model->type = MOD_BRUSH46;
		model->q3_bmodel = out;
		String::Sprintf( model->name, sizeof( model->name ), "*%d", i );

		for ( j = 0 ; j < 3 ; j++ ) {
			out->bounds[0][j] = LittleFloat( in->mins[j] );
			out->bounds[1][j] = LittleFloat( in->maxs[j] );
		}

		out->firstSurface = s_worldData.surfaces + LittleLong( in->firstSurface );
		out->numSurfaces = LittleLong( in->numSurfaces );
	}
}

void R_LoadNodesAndLeafs( bsp46_lump_t *nodeLump, bsp46_lump_t *leafLump );
void R_LoadShaders( bsp46_lump_t *l );
void R_LoadMarksurfaces( bsp46_lump_t *l );
void R_LoadPlanes( bsp46_lump_t *l );
void R_LoadFogs( bsp46_lump_t *l, bsp46_lump_t *brushesLump, bsp46_lump_t *sidesLump );
void R_LoadLightGrid( bsp46_lump_t *l );

/*
================
R_LoadEntities
================
*/
void R_LoadEntities( bsp46_lump_t *l ) {
	const char *p;
	char *token, *s;
	char keyname[MAX_TOKEN_CHARS_Q3];
	char value[MAX_TOKEN_CHARS_Q3];
	world_t *w;

	w = &s_worldData;
	w->lightGridSize[0] = 64;
	w->lightGridSize[1] = 64;
	w->lightGridSize[2] = 128;

	p = ( char * )( fileBase + l->fileofs );

	// store for reference by the cgame
	w->entityString = (char*)ri.Hunk_Alloc( l->filelen + 1, h_low );
	String::Cpy( w->entityString, p );
	w->entityParsePoint = w->entityString;

	token = String::ParseExt( &p, qtrue );
	if ( !*token || *token != '{' ) {
		return;
	}

	// only parse the world spawn
	while ( 1 ) {
		// parse key
		token = String::ParseExt( &p, qtrue );

		if ( !*token || *token == '}' ) {
			break;
		}
		String::NCpyZ( keyname, token, sizeof( keyname ) );

		// parse value
		token = String::ParseExt( &p, qtrue );

		if ( !*token || *token == '}' ) {
			break;
		}
		String::NCpyZ( value, token, sizeof( value ) );

		// check for remapping of shaders for vertex lighting
		s = "vertexremapshader";
		if ( !String::NCmp( keyname, s, String::Length( s ) ) ) {
			s = strchr( value, ';' );
			if ( !s ) {
				ri.Printf( PRINT_WARNING, "WARNING: no semi colon in vertexshaderremap '%s'\n", value );
				break;
			}
			*s++ = 0;
			if ( r_vertexLight->integer ) {
				R_RemapShader( value, s, "0" );
			}
			continue;
		}
		// check for remapping of shaders
		s = "remapshader";
		if ( !String::NCmp( keyname, s, String::Length( s ) ) ) {
			s = strchr( value, ';' );
			if ( !s ) {
				ri.Printf( PRINT_WARNING, "WARNING: no semi colon in shaderremap '%s'\n", value );
				break;
			}
			*s++ = 0;
			R_RemapShader( value, s, "0" );
			continue;
		}
		// check for a different grid size
		if ( !String::ICmp( keyname, "gridsize" ) ) {
			sscanf( value, "%f %f %f", &w->lightGridSize[0], &w->lightGridSize[1], &w->lightGridSize[2] );
			continue;
		}
	}
}

/*
=================
R_GetEntityToken
=================
*/
qboolean R_GetEntityToken( char *buffer, int size ) {
	const char  *s;

	s = String::Parse3(const_cast<const char**>(&s_worldData.entityParsePoint));
	String::NCpyZ( buffer, s, size );
	if ( !s_worldData.entityParsePoint || !s[0] ) {
		s_worldData.entityParsePoint = s_worldData.entityString;
		return qfalse;
	} else {
		return qtrue;
	}
}

/*
=================
RE_LoadWorldMap

Called directly from cgame
=================
*/
void RE_LoadWorldMap( const char *name ) {
	int i;
	bsp46_dheader_t   *header;
	byte        *buffer;
	byte        *startMarker;

	skyboxportal = 0;

	if ( tr.worldMapLoaded ) {
		ri.Error( ERR_DROP, "ERROR: attempted to redundantly load world map\n" );
	}

	// set default sun direction to be used if it isn't
	// overridden by a shader
	tr.sunDirection[0] = 0.45;
	tr.sunDirection[1] = 0.3;
	tr.sunDirection[2] = 0.9;

	tr.sunShader = 0;   // clear sunshader so it's not there if the level doesn't specify it

	// invalidate fogs (likely to be re-initialized to new values by the current map)
	// TODO:(SA)this is sort of silly.  I'm going to do a general cleanup on fog stuff
	//			now that I can see how it's been used.  (functionality can narrow since
	//			it's not used as much as it's designed for.)
	R_SetFog( FOG_SKY,       0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_PORTALVIEW,0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_HUD,       0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_MAP,       0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_CURRENT,   0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_TARGET,    0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_WATER,     0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_SERVER,    0, 0, 0, 0, 0, 0 );

	VectorNormalize( tr.sunDirection );

	tr.worldMapLoaded = qtrue;

	// load it
	ri.FS_ReadFile( name, (void **)&buffer );
	if ( !buffer ) {
		ri.Error( ERR_DROP, "RE_LoadWorldMap: %s not found", name );
	}

	// clear tr.world so if the level fails to load, the next
	// try will not look at the partially loaded version
	tr.world = NULL;

	memset( &s_worldData, 0, sizeof( s_worldData ) );
	String::NCpyZ( s_worldData.name, name, sizeof( s_worldData.name ) );

	String::NCpyZ( s_worldData.baseName, String::SkipPath( s_worldData.name ), sizeof( s_worldData.name ) );
	String::StripExtension( s_worldData.baseName, s_worldData.baseName );

	startMarker = (byte*)ri.Hunk_Alloc( 0, h_low );
	c_gridVerts = 0;

	header = (bsp46_dheader_t *)buffer;
	fileBase = (byte *)header;

	i = LittleLong( header->version );
#ifndef _SKIP_BSP_CHECK
	if ( i != BSP47_VERSION ) {
		ri.Error( ERR_DROP, "RE_LoadWorldMap: %s has wrong version number (%i should be %i)",
				  name, i, BSP47_VERSION );
	}
#endif

	// swap all the lumps
	for ( i = 0 ; i < sizeof( bsp46_dheader_t ) / 4 ; i++ ) {
		( (int *)header )[i] = LittleLong( ( (int *)header )[i] );
	}

	// load into heap
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
	R_LoadShaders( &header->lumps[BSP46LUMP_SHADERS] );
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
	R_LoadLightmaps( &header->lumps[BSP46LUMP_LIGHTMAPS] );
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
	R_LoadPlanes( &header->lumps[BSP46LUMP_PLANES] );
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
	R_LoadFogs( &header->lumps[BSP46LUMP_FOGS], &header->lumps[BSP46LUMP_BRUSHES], &header->lumps[BSP46LUMP_BRUSHSIDES] );
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
	R_LoadSurfaces( &header->lumps[BSP46LUMP_SURFACES], &header->lumps[BSP46LUMP_DRAWVERTS], &header->lumps[BSP46LUMP_DRAWINDEXES] );
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
	R_LoadMarksurfaces( &header->lumps[BSP46LUMP_LEAFSURFACES] );
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
	R_LoadNodesAndLeafs( &header->lumps[BSP46LUMP_NODES], &header->lumps[BSP46LUMP_LEAFS] );
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
	R_LoadSubmodels( &header->lumps[BSP46LUMP_MODELS] );
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
	R_LoadVisibility( &header->lumps[BSP46LUMP_VISIBILITY] );
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
	R_LoadEntities( &header->lumps[BSP46LUMP_ENTITIES] );
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );
	R_LoadLightGrid( &header->lumps[BSP46LUMP_LIGHTGRID] );
	ri.Cmd_ExecuteText( EXEC_NOW, "updatescreen\n" );

	s_worldData.dataSize = (byte *)ri.Hunk_Alloc( 0, h_low ) - startMarker;

	// only set tr.world now that we know the entire level has loaded properly
	tr.world = &s_worldData;

	// reset fog to world fog (if present)
//	R_SetFog(FOG_CMD_SWITCHFOG, FOG_MAP,20,0,0,0,0);

//----(SA)	set the sun shader if there is one
	if ( tr.sunShaderName ) {
		tr.sunShader = R_FindShader( tr.sunShaderName, LIGHTMAP_NONE, qtrue );
	}

//----(SA)	end
	ri.FS_FreeFile( buffer );
}

