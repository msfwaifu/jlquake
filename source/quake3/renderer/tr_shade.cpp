/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_shade.c

#include "tr_local.h"

/*

  THIS ENTIRE FILE IS BACK END

  This file deals with applying shaders to surface data in the tess struct.
*/

/*
=============================================================

SURFACE SHADERS

=============================================================
*/

static qboolean	setArraysOnce;

/*
=================
R_BindAnimatedImage

=================
*/
static void R_BindAnimatedImage( textureBundle_t *bundle ) {
	int		index;

	if ( bundle->isVideoMap ) {
		CIN_RunCinematic(bundle->videoMapHandle);
		CIN_UploadCinematic(bundle->videoMapHandle);
		return;
	}

	if ( bundle->numImageAnimations <= 1 ) {
		GL_Bind( bundle->image[0] );
		return;
	}

	// it is necessary to do this messy calc to make sure animations line up
	// exactly with waveforms of the same frequency
	index = myftol( tess.shaderTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE );
	index >>= FUNCTABLE_SIZE2;

	if ( index < 0 ) {
		index = 0;	// may happen with shader time offsets
	}
	index %= bundle->numImageAnimations;

	GL_Bind( bundle->image[ index ] );
}

/*
================
DrawTris

Draws triangle outlines for debugging
================
*/
static void DrawTris (shaderCommands_t *input) {
	GL_Bind( tr.whiteImage );
	qglColor3f (1,1,1);

	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );
	qglDepthRange( 0, 0 );

	qglDisableClientState (GL_COLOR_ARRAY);
	qglDisableClientState (GL_TEXTURE_COORD_ARRAY);

	qglVertexPointer (3, GL_FLOAT, 16, input->xyz);	// padded for SIMD

	if (qglLockArraysEXT) {
		qglLockArraysEXT(0, input->numVertexes);
		QGL_LogComment( "glLockArraysEXT\n" );
	}

	R_DrawElements( input->numIndexes, input->indexes );

	if (qglUnlockArraysEXT) {
		qglUnlockArraysEXT();
		QGL_LogComment( "glUnlockArraysEXT\n" );
	}
	qglDepthRange( 0, 1 );
}


/*
================
DrawNormals

Draws vertex normals for debugging
================
*/
static void DrawNormals (shaderCommands_t *input) {
	int		i;
	vec3_t	temp;

	GL_Bind( tr.whiteImage );
	qglColor3f (1,1,1);
	qglDepthRange( 0, 0 );	// never occluded
	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );

	qglBegin (GL_LINES);
	for (i = 0 ; i < input->numVertexes ; i++) {
		qglVertex3fv (input->xyz[i]);
		VectorMA (input->xyz[i], 2, input->normal[i], temp);
		qglVertex3fv (temp);
	}
	qglEnd ();

	qglDepthRange( 0, 1 );
}

/*
==============
RB_BeginSurface

We must set some things up before beginning any tesselation,
because a surface may be forced to perform a RB_End due
to overflow.
==============
*/
void RB_BeginSurface( shader_t *shader, int fogNum ) {

	shader_t *state = (shader->remappedShader) ? shader->remappedShader : shader;

	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.shader = state;
	tess.fogNum = fogNum;
	tess.dlightBits = 0;		// will be OR'd in by surface functions
	tess.xstages = state->stages;
	tess.numPasses = state->numUnfoggedPasses;
	tess.currentStageIteratorFunc = state->optimalStageIteratorFunc;

	tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
	if (tess.shader->clampTime && tess.shaderTime >= tess.shader->clampTime) {
		tess.shaderTime = tess.shader->clampTime;
	}


}

/*
===================
DrawMultitextured

output = t0 * t1 or t0 + t1

t0 = most upstream according to spec
t1 = most downstream according to spec
===================
*/
static void DrawMultitextured( shaderCommands_t *input, int stage ) {
	shaderStage_t	*pStage;

	pStage = tess.xstages[stage];

	GL_State( pStage->stateBits );

	// this is an ugly hack to work around a GeForce driver
	// bug with multitexture and clip planes
	if ( backEnd.viewParms.isPortal ) {
		qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}

	//
	// base
	//
	GL_SelectTexture( 0 );
	qglTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[0] );
	R_BindAnimatedImage( &pStage->bundle[0] );

	//
	// lightmap/secondary pass
	//
	GL_SelectTexture( 1 );
	qglEnable( GL_TEXTURE_2D );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );

	if ( r_lightmap->integer ) {
		GL_TexEnv( GL_REPLACE );
	} else {
		GL_TexEnv( tess.shader->multitextureEnv );
	}

	qglTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[1] );

	R_BindAnimatedImage( &pStage->bundle[1] );

	R_DrawElements( input->numIndexes, input->indexes );

	//
	// disable texturing on TEXTURE1, then select TEXTURE0
	//
	//qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
	qglDisable( GL_TEXTURE_2D );

	GL_SelectTexture( 0 );
}



/*
===================
ProjectDlightTexture

Perform dynamic lighting with another rendering pass
===================
*/
static void ProjectDlightTexture( void ) {
	int		i, l;
#if idppc_altivec
	vec_t	origin0, origin1, origin2;
	float   texCoords0, texCoords1;
	vector float floatColorVec0, floatColorVec1;
	vector float modulateVec, colorVec, zero;
	vector short colorShort;
	vector signed int colorInt;
	vector unsigned char floatColorVecPerm, modulatePerm, colorChar;
	vector unsigned char vSel = (vector unsigned char)(0x00, 0x00, 0x00, 0xff,
							   0x00, 0x00, 0x00, 0xff,
							   0x00, 0x00, 0x00, 0xff,
							   0x00, 0x00, 0x00, 0xff);
#else
	vec3_t	origin;
#endif
	float	*texCoords;
	byte	*colors;
	byte	clipBits[SHADER_MAX_VERTEXES];
	MAC_STATIC float	texCoordsArray[SHADER_MAX_VERTEXES][2];
	byte	colorArray[SHADER_MAX_VERTEXES][4];
	unsigned	hitIndexes[SHADER_MAX_INDEXES];
	int		numIndexes;
	float	scale;
	float	radius;
	vec3_t	floatColor;
	float	modulate;

	if ( !backEnd.refdef.num_dlights ) {
		return;
	}

#if idppc_altivec
	// There has to be a better way to do this so that floatColor 
	// and/or modulate are already 16-byte aligned.
	floatColorVecPerm = vec_lvsl(0,(float *)floatColor);
	modulatePerm = vec_lvsl(0,(float *)&modulate);
	modulatePerm = (vector unsigned char)vec_splat((vector unsigned int)modulatePerm,0);
	zero = (vector float)vec_splat_s8(0);
#endif

	for ( l = 0 ; l < backEnd.refdef.num_dlights ; l++ ) {
		dlight_t	*dl;

		if ( !( tess.dlightBits & ( 1 << l ) ) ) {
			continue;	// this surface definately doesn't have any of this light
		}
		texCoords = texCoordsArray[0];
		colors = colorArray[0];

		dl = &backEnd.refdef.dlights[l];
#if idppc_altivec
		origin0 = dl->transformed[0];
		origin1 = dl->transformed[1];
		origin2 = dl->transformed[2];
#else
		VectorCopy( dl->transformed, origin );
#endif
		radius = dl->radius;
		scale = 1.0f / radius;

		floatColor[0] = dl->color[0] * 255.0f;
		floatColor[1] = dl->color[1] * 255.0f;
		floatColor[2] = dl->color[2] * 255.0f;
#if idppc_altivec
		floatColorVec0 = vec_ld(0, floatColor);
		floatColorVec1 = vec_ld(11, floatColor);
		floatColorVec0 = vec_perm(floatColorVec0,floatColorVec0,floatColorVecPerm);
#endif
		for ( i = 0 ; i < tess.numVertexes ; i++, texCoords += 2, colors += 4 ) {
#if idppc_altivec
			vec_t dist0, dist1, dist2;
#else
			vec3_t	dist;
#endif
			int		clip;

			backEnd.pc.c_dlightVertexes++;

#if idppc_altivec
			//VectorSubtract( origin, tess.xyz[i], dist );
			dist0 = origin0 - tess.xyz[i][0];
			dist1 = origin1 - tess.xyz[i][1];
			dist2 = origin2 - tess.xyz[i][2];
			texCoords0 = 0.5f + dist0 * scale;
			texCoords1 = 0.5f + dist1 * scale;

			clip = 0;
			if ( texCoords0 < 0.0f ) {
				clip |= 1;
			} else if ( texCoords0 > 1.0f ) {
				clip |= 2;
			}
			if ( texCoords1 < 0.0f ) {
				clip |= 4;
			} else if ( texCoords1 > 1.0f ) {
				clip |= 8;
			}
			texCoords[0] = texCoords0;
			texCoords[1] = texCoords1;
			
			// modulate the strength based on the height and color
			if ( dist2 > radius ) {
				clip |= 16;
				modulate = 0.0f;
			} else if ( dist2 < -radius ) {
				clip |= 32;
				modulate = 0.0f;
			} else {
				dist2 = Q_fabs(dist2);
				if ( dist2 < radius * 0.5f ) {
					modulate = 1.0f;
				} else {
					modulate = 2.0f * (radius - dist2) * scale;
				}
			}
			clipBits[i] = clip;

			modulateVec = vec_ld(0,(float *)&modulate);
			modulateVec = vec_perm(modulateVec,modulateVec,modulatePerm);
			colorVec = vec_madd(floatColorVec0,modulateVec,zero);
			colorInt = vec_cts(colorVec,0);	// RGBx
			colorShort = vec_pack(colorInt,colorInt);		// RGBxRGBx
			colorChar = vec_packsu(colorShort,colorShort);	// RGBxRGBxRGBxRGBx
			colorChar = vec_sel(colorChar,vSel,vSel);		// RGBARGBARGBARGBA replace alpha with 255
			vec_ste((vector unsigned int)colorChar,0,(unsigned int *)colors);	// store color
#else
			VectorSubtract( origin, tess.xyz[i], dist );
			texCoords[0] = 0.5f + dist[0] * scale;
			texCoords[1] = 0.5f + dist[1] * scale;

			clip = 0;
			if ( texCoords[0] < 0.0f ) {
				clip |= 1;
			} else if ( texCoords[0] > 1.0f ) {
				clip |= 2;
			}
			if ( texCoords[1] < 0.0f ) {
				clip |= 4;
			} else if ( texCoords[1] > 1.0f ) {
				clip |= 8;
			}
			// modulate the strength based on the height and color
			if ( dist[2] > radius ) {
				clip |= 16;
				modulate = 0.0f;
			} else if ( dist[2] < -radius ) {
				clip |= 32;
				modulate = 0.0f;
			} else {
				dist[2] = Q_fabs(dist[2]);
				if ( dist[2] < radius * 0.5f ) {
					modulate = 1.0f;
				} else {
					modulate = 2.0f * (radius - dist[2]) * scale;
				}
			}
			clipBits[i] = clip;

			colors[0] = myftol(floatColor[0] * modulate);
			colors[1] = myftol(floatColor[1] * modulate);
			colors[2] = myftol(floatColor[2] * modulate);
			colors[3] = 255;
#endif
		}

		// build a list of triangles that need light
		numIndexes = 0;
		for ( i = 0 ; i < tess.numIndexes ; i += 3 ) {
			int		a, b, c;

			a = tess.indexes[i];
			b = tess.indexes[i+1];
			c = tess.indexes[i+2];
			if ( clipBits[a] & clipBits[b] & clipBits[c] ) {
				continue;	// not lighted
			}
			hitIndexes[numIndexes] = a;
			hitIndexes[numIndexes+1] = b;
			hitIndexes[numIndexes+2] = c;
			numIndexes += 3;
		}

		if ( !numIndexes ) {
			continue;
		}

		qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
		qglTexCoordPointer( 2, GL_FLOAT, 0, texCoordsArray[0] );

		qglEnableClientState( GL_COLOR_ARRAY );
		qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, colorArray );

		GL_Bind( tr.dlightImage );
		// include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
		// where they aren't rendered
		if ( dl->additive ) {
			GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );
		}
		else {
			GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );
		}
		R_DrawElements( numIndexes, hitIndexes );
		backEnd.pc.c_totalIndexes += numIndexes;
		backEnd.pc.c_dlightIndexes += numIndexes;
	}
}


/*
===================
RB_FogPass

Blends a fog texture on top of everything else
===================
*/
static void RB_FogPass( void ) {
	mbrush46_fog_t		*fog;
	int			i;

	qglEnableClientState( GL_COLOR_ARRAY );
	qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors );

	qglEnableClientState( GL_TEXTURE_COORD_ARRAY);
	qglTexCoordPointer( 2, GL_FLOAT, 0, tess.svars.texcoords[0] );

	fog = tr.world->fogs + tess.fogNum;

	for ( i = 0; i < tess.numVertexes; i++ ) {
		* ( int * )&tess.svars.colors[i] = fog->colorInt;
	}

	RB_CalcFogTexCoords( ( float * ) tess.svars.texcoords[0] );

	GL_Bind( tr.fogImage );

	if ( tess.shader->fogPass == FP_EQUAL ) {
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );
	} else {
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	}

	R_DrawElements( tess.numIndexes, tess.indexes );
}

/*
** RB_IterateStagesGeneric
*/
static void RB_IterateStagesGeneric( shaderCommands_t *input )
{
	int stage;

	for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
	{
		shaderStage_t *pStage = tess.xstages[stage];

		if ( !pStage )
		{
			break;
		}

		ComputeColors( pStage );
		ComputeTexCoords( pStage );

		if ( !setArraysOnce )
		{
			qglEnableClientState( GL_COLOR_ARRAY );
			qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, input->svars.colors );
		}

		//
		// do multitexture
		//
		if ( pStage->bundle[1].image[0] != 0 )
		{
			DrawMultitextured( input, stage );
		}
		else
		{
			if ( !setArraysOnce )
			{
				qglTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[0] );
			}

			//
			// set state
			//
			if ( pStage->bundle[0].vertexLightmap && r_vertexLight->integer && !r_uiFullScreen->integer && r_lightmap->integer )
			{
				GL_Bind( tr.whiteImage );
			}
			else 
				R_BindAnimatedImage( &pStage->bundle[0] );

			GL_State( pStage->stateBits );

			//
			// draw
			//
			R_DrawElements( input->numIndexes, input->indexes );
		}
		// allow skipping out to show just lightmaps during development
		if ( r_lightmap->integer && ( pStage->bundle[0].isLightmap || pStage->bundle[1].isLightmap || pStage->bundle[0].vertexLightmap ) )
		{
			break;
		}
	}
}


/*
** RB_StageIteratorGeneric
*/
void RB_StageIteratorGeneric( void )
{
	shaderCommands_t *input;

	input = &tess;

	RB_DeformTessGeometry();

	//
	// log this call
	//
	if ( r_logFile->integer ) 
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		QGL_LogComment( va("--- RB_StageIteratorGeneric( %s ) ---\n", tess.shader->name) );
	}

	//
	// set face culling appropriately
	//
	GL_Cull( input->shader->cullType );

	// set polygon offset if necessary
	if ( input->shader->polygonOffset )
	{
		qglEnable( GL_POLYGON_OFFSET_FILL );
		qglPolygonOffset( r_offsetFactor->value, r_offsetUnits->value );
	}

	//
	// if there is only a single pass then we can enable color
	// and texture arrays before we compile, otherwise we need
	// to avoid compiling those arrays since they will change
	// during multipass rendering
	//
	if ( tess.numPasses > 1 || input->shader->multitextureEnv )
	{
		setArraysOnce = qfalse;
		qglDisableClientState (GL_COLOR_ARRAY);
		qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
	}
	else
	{
		setArraysOnce = qtrue;

		qglEnableClientState( GL_COLOR_ARRAY);
		qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors );

		qglEnableClientState( GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer( 2, GL_FLOAT, 0, tess.svars.texcoords[0] );
	}

	//
	// lock XYZ
	//
	qglVertexPointer (3, GL_FLOAT, 16, input->xyz);	// padded for SIMD
	if (qglLockArraysEXT)
	{
		qglLockArraysEXT(0, input->numVertexes);
		QGL_LogComment( "glLockArraysEXT\n" );
	}

	//
	// enable color and texcoord arrays after the lock if necessary
	//
	if ( !setArraysOnce )
	{
		qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
		qglEnableClientState( GL_COLOR_ARRAY );
	}

	//
	// call shader function
	//
	RB_IterateStagesGeneric( input );

	// 
	// now do any dynamic lighting needed
	//
	if ( tess.dlightBits && tess.shader->sort <= SS_OPAQUE
		&& !(tess.shader->surfaceFlags & (BSP46SURF_NODLIGHT | BSP46SURF_SKY) ) ) {
		ProjectDlightTexture();
	}

	//
	// now do fog
	//
	if ( tess.fogNum && tess.shader->fogPass ) {
		RB_FogPass();
	}

	// 
	// unlock arrays
	//
	if (qglUnlockArraysEXT) 
	{
		qglUnlockArraysEXT();
		QGL_LogComment( "glUnlockArraysEXT\n" );
	}

	//
	// reset polygon offset
	//
	if ( input->shader->polygonOffset )
	{
		qglDisable( GL_POLYGON_OFFSET_FILL );
	}
}


/*
** RB_StageIteratorVertexLitTexture
*/
void RB_StageIteratorVertexLitTexture( void )
{
	shaderCommands_t *input;
	shader_t		*shader;

	input = &tess;

	shader = input->shader;

	//
	// compute colors
	//
	RB_CalcDiffuseColor( ( unsigned char * ) tess.svars.colors );

	//
	// log this call
	//
	if ( r_logFile->integer ) 
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		QGL_LogComment( va("--- RB_StageIteratorVertexLitTexturedUnfogged( %s ) ---\n", tess.shader->name) );
	}

	//
	// set face culling appropriately
	//
	GL_Cull( input->shader->cullType );

	//
	// set arrays and lock
	//
	qglEnableClientState( GL_COLOR_ARRAY);
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY);

	qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors );
	qglTexCoordPointer( 2, GL_FLOAT, 16, tess.texCoords[0][0] );
	qglVertexPointer (3, GL_FLOAT, 16, input->xyz);

	if ( qglLockArraysEXT )
	{
		qglLockArraysEXT(0, input->numVertexes);
		QGL_LogComment( "glLockArraysEXT\n" );
	}

	//
	// call special shade routine
	//
	R_BindAnimatedImage( &tess.xstages[0]->bundle[0] );
	GL_State( tess.xstages[0]->stateBits );
	R_DrawElements( input->numIndexes, input->indexes );

	// 
	// now do any dynamic lighting needed
	//
	if ( tess.dlightBits && tess.shader->sort <= SS_OPAQUE ) {
		ProjectDlightTexture();
	}

	//
	// now do fog
	//
	if ( tess.fogNum && tess.shader->fogPass ) {
		RB_FogPass();
	}

	// 
	// unlock arrays
	//
	if (qglUnlockArraysEXT) 
	{
		qglUnlockArraysEXT();
		QGL_LogComment( "glUnlockArraysEXT\n" );
	}
}

//define	REPLACE_MODE

void RB_StageIteratorLightmappedMultitexture( void ) {
	shaderCommands_t *input;

	input = &tess;

	//
	// log this call
	//
	if ( r_logFile->integer ) {
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		QGL_LogComment( va("--- RB_StageIteratorLightmappedMultitexture( %s ) ---\n", tess.shader->name) );
	}

	//
	// set face culling appropriately
	//
	GL_Cull( input->shader->cullType );

	//
	// set color, pointers, and lock
	//
	GL_State( GLS_DEFAULT );
	qglVertexPointer( 3, GL_FLOAT, 16, input->xyz );

#ifdef REPLACE_MODE
	qglDisableClientState( GL_COLOR_ARRAY );
	qglColor3f( 1, 1, 1 );
	qglShadeModel( GL_FLAT );
#else
	qglEnableClientState( GL_COLOR_ARRAY );
	qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.constantColor255 );
#endif

	//
	// select base stage
	//
	GL_SelectTexture( 0 );

	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	R_BindAnimatedImage( &tess.xstages[0]->bundle[0] );
	qglTexCoordPointer( 2, GL_FLOAT, 16, tess.texCoords[0][0] );

	//
	// configure second stage
	//
	GL_SelectTexture( 1 );
	qglEnable( GL_TEXTURE_2D );
	if ( r_lightmap->integer ) {
		GL_TexEnv( GL_REPLACE );
	} else {
		GL_TexEnv( GL_MODULATE );
	}
	R_BindAnimatedImage( &tess.xstages[0]->bundle[1] );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	qglTexCoordPointer( 2, GL_FLOAT, 16, tess.texCoords[0][1] );

	//
	// lock arrays
	//
	if ( qglLockArraysEXT ) {
		qglLockArraysEXT(0, input->numVertexes);
		QGL_LogComment( "glLockArraysEXT\n" );
	}

	R_DrawElements( input->numIndexes, input->indexes );

	//
	// disable texturing on TEXTURE1, then select TEXTURE0
	//
	qglDisable( GL_TEXTURE_2D );
	qglDisableClientState( GL_TEXTURE_COORD_ARRAY );

	GL_SelectTexture( 0 );
#ifdef REPLACE_MODE
	GL_TexEnv( GL_MODULATE );
	qglShadeModel( GL_SMOOTH );
#endif

	// 
	// now do any dynamic lighting needed
	//
	if ( tess.dlightBits && tess.shader->sort <= SS_OPAQUE ) {
		ProjectDlightTexture();
	}

	//
	// now do fog
	//
	if ( tess.fogNum && tess.shader->fogPass ) {
		RB_FogPass();
	}

	//
	// unlock arrays
	//
	if ( qglUnlockArraysEXT ) {
		qglUnlockArraysEXT();
		QGL_LogComment( "glUnlockArraysEXT\n" );
	}
}

/*
** RB_EndSurface
*/
void RB_EndSurface( void ) {
	shaderCommands_t *input;

	input = &tess;

	if (input->numIndexes == 0) {
		return;
	}

	if (input->indexes[SHADER_MAX_INDEXES-1] != 0) {
		ri.Error (ERR_DROP, "RB_EndSurface() - SHADER_MAX_INDEXES hit");
	}	
	if (input->xyz[SHADER_MAX_VERTEXES-1][0] != 0) {
		ri.Error (ERR_DROP, "RB_EndSurface() - SHADER_MAX_VERTEXES hit");
	}

	if ( tess.shader == tr.shadowShader ) {
		RB_ShadowTessEnd();
		return;
	}

	// for debugging of sort order issues, stop rendering after a given sort value
	if ( r_debugSort->integer && r_debugSort->integer < tess.shader->sort ) {
		return;
	}

	//
	// update performance counters
	//
	backEnd.pc.c_shaders++;
	backEnd.pc.c_vertexes += tess.numVertexes;
	backEnd.pc.c_indexes += tess.numIndexes;
	backEnd.pc.c_totalIndexes += tess.numIndexes * tess.numPasses;

	//
	// call off to shader specific tess end function
	//
	tess.currentStageIteratorFunc();

	//
	// draw debugging stuff
	//
	if ( r_showtris->integer ) {
		DrawTris (input);
	}
	if ( r_shownormals->integer ) {
		DrawNormals (input);
	}
	// clear shader so we can tell we don't have any unclosed surfaces
	tess.numIndexes = 0;

	QGL_LogComment( "----------\n" );
}

