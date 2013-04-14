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
#include "../../common/Common.h"
#include "../../common/strings.h"

gllightmapstate_t gl_lms;

mbrush38_surface_t* r_alpha_surfaces;

int r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

static float s_blocklights_q2[ 34 * 34 * 3 ];

/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

static void LM_InitBlock() {
	Com_Memset( gl_lms.allocated, 0, sizeof ( gl_lms.allocated ) );
}

//	Returns a texture number and the position inside it
static bool LM_AllocBlock( int w, int h, int* x, int* y ) {
	int best = BLOCK_HEIGHT;

	for ( int i = 0; i < BLOCK_WIDTH - w; i++ ) {
		int best2 = 0;

		int j;
		for ( j = 0; j < w; j++ ) {
			if ( gl_lms.allocated[ i + j ] >= best ) {
				break;
			}
			if ( gl_lms.allocated[ i + j ] > best2 ) {
				best2 = gl_lms.allocated[ i + j ];
			}
		}
		if ( j == w ) {
			// this is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if ( best + h > BLOCK_HEIGHT ) {
		return false;
	}

	for ( int i = 0; i < w; i++ ) {
		gl_lms.allocated[ *x + i ] = best + h;
	}

	return true;
}

static void LM_UploadBlock() {
	int texture = gl_lms.current_lightmap_texture;

	GL_Bind( tr.lightmaps[ texture ] );

	R_ReUploadImage( tr.lightmaps[ texture ], gl_lms.lightmap_buffer );
	if ( ++gl_lms.current_lightmap_texture == MAX_LIGHTMAPS ) {
		common->Error( "LM_UploadBlock() - MAX_LIGHTMAPS exceeded\n" );
	}
}

/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

static void R_SetCacheState( mbrush38_surface_t* surf ) {
	for ( int maps = 0; maps < BSP38_MAXLIGHTMAPS && surf->styles[ maps ] != 255; maps++ ) {
		surf->cached_light[ maps ] = tr.refdef.lightstyles[ surf->styles[ maps ] ].white;
	}
}

static void R_AddDynamicLightsQ2( mbrush38_surface_t* surf ) {
	int smax = ( surf->extents[ 0 ] >> 4 ) + 1;
	int tmax = ( surf->extents[ 1 ] >> 4 ) + 1;
	mbrush38_texinfo_t* tex = surf->texinfo;

	for ( int lnum = 0; lnum < tr.refdef.num_dlights; lnum++ ) {
		if ( !( surf->dlightbits & ( 1 << lnum ) ) ) {
			continue;		// not lit by this light
		}

		dlight_t* dl = &tr.refdef.dlights[ lnum ];
		float frad = dl->radius;
		float fdist = DotProduct( dl->origin, surf->plane->normal ) - surf->plane->dist;
		frad -= fabs( fdist );
		// rad is now the highest intensity on the plane

		float fminlight = DLIGHT_CUTOFF;	// FIXME: make configurable?
		if ( frad < fminlight ) {
			continue;
		}
		fminlight = frad - fminlight;

		vec3_t impact;
		for ( int i = 0; i < 3; i++ ) {
			impact[ i ] = dl->origin[ i ] - surf->plane->normal[ i ] * fdist;
		}

		vec3_t local;
		local[ 0 ] = DotProduct( impact, tex->vecs[ 0 ] ) + tex->vecs[ 0 ][ 3 ] - surf->texturemins[ 0 ];
		local[ 1 ] = DotProduct( impact, tex->vecs[ 1 ] ) + tex->vecs[ 1 ][ 3 ] - surf->texturemins[ 1 ];

		float* pfBL = s_blocklights_q2;
		float ftacc = 0;
		for ( int t = 0; t < tmax; t++, ftacc += 16 ) {
			int td = local[ 1 ] - ftacc;
			if ( td < 0 ) {
				td = -td;
			}

			float fsacc = 0;
			for ( int s = 0; s < smax; s++, fsacc += 16, pfBL += 3 ) {
				int sd = idMath::FtoiFast( local[ 0 ] - fsacc );

				if ( sd < 0 ) {
					sd = -sd;
				}

				if ( sd > td ) {
					fdist = sd + ( td >> 1 );
				} else {
					fdist = td + ( sd >> 1 );
				}

				if ( fdist < fminlight ) {
					pfBL[ 0 ] += ( frad - fdist ) * dl->color[ 0 ];
					pfBL[ 1 ] += ( frad - fdist ) * dl->color[ 1 ];
					pfBL[ 2 ] += ( frad - fdist ) * dl->color[ 2 ];
				}
			}
		}
	}
}

//	Combine and scale multiple lightmaps into the floating format in blocklights
static void R_BuildLightMapQ2( mbrush38_surface_t* surf, byte* dest, int stride ) {
	if ( surf->texinfo->flags & ( BSP38SURF_SKY | BSP38SURF_TRANS33 | BSP38SURF_TRANS66 | BSP38SURF_WARP ) ) {
		common->Error( "R_BuildLightMapQ2 called for non-lit surface" );
	}

	int smax = ( surf->extents[ 0 ] >> 4 ) + 1;
	int tmax = ( surf->extents[ 1 ] >> 4 ) + 1;
	int size = smax * tmax;
	if ( size > ( ( int )sizeof ( s_blocklights_q2 ) >> 4 ) ) {
		common->Error( "Bad s_blocklights_q2 size" );
	}

	// set to full bright if no light data
	if ( !surf->samples ) {
		for ( int i = 0; i < size * 3; i++ ) {
			s_blocklights_q2[ i ] = 255;
		}
	} else {
		// count the # of maps
		int nummaps = 0;
		while ( nummaps < BSP38_MAXLIGHTMAPS && surf->styles[ nummaps ] != 255 ) {
			nummaps++;
		}

		byte* lightmap = surf->samples;

		// add all the lightmaps
		if ( nummaps == 1 ) {
			for ( int maps = 0; maps < BSP38_MAXLIGHTMAPS && surf->styles[ maps ] != 255; maps++ ) {
				float* bl = s_blocklights_q2;

				float scale[ 4 ];
				for ( int i = 0; i < 3; i++ ) {
					scale[ i ] = r_modulate->value * tr.refdef.lightstyles[ surf->styles[ maps ] ].rgb[ i ];
				}

				if ( scale[ 0 ] == 1.0F && scale[ 1 ] == 1.0F && scale[ 2 ] == 1.0F ) {
					for ( int i = 0; i < size; i++, bl += 3 ) {
						bl[ 0 ] = lightmap[ i * 3 + 0 ];
						bl[ 1 ] = lightmap[ i * 3 + 1 ];
						bl[ 2 ] = lightmap[ i * 3 + 2 ];
					}
				} else {
					for ( int i = 0; i < size; i++, bl += 3 ) {
						bl[ 0 ] = lightmap[ i * 3 + 0 ] * scale[ 0 ];
						bl[ 1 ] = lightmap[ i * 3 + 1 ] * scale[ 1 ];
						bl[ 2 ] = lightmap[ i * 3 + 2 ] * scale[ 2 ];
					}
				}
				lightmap += size * 3;		// skip to next lightmap
			}
		} else {
			Com_Memset( s_blocklights_q2, 0, sizeof ( s_blocklights_q2[ 0 ] ) * size * 3 );

			for ( int maps = 0; maps < BSP38_MAXLIGHTMAPS && surf->styles[ maps ] != 255; maps++ ) {
				float* bl = s_blocklights_q2;

				float scale[ 4 ];
				for ( int i = 0; i < 3; i++ ) {
					scale[ i ] = r_modulate->value * tr.refdef.lightstyles[ surf->styles[ maps ] ].rgb[ i ];
				}

				if ( scale[ 0 ] == 1.0F && scale[ 1 ] == 1.0F && scale[ 2 ] == 1.0F ) {
					for ( int i = 0; i < size; i++, bl += 3 ) {
						bl[ 0 ] += lightmap[ i * 3 + 0 ];
						bl[ 1 ] += lightmap[ i * 3 + 1 ];
						bl[ 2 ] += lightmap[ i * 3 + 2 ];
					}
				} else {
					for ( int i = 0; i < size; i++, bl += 3 ) {
						bl[ 0 ] += lightmap[ i * 3 + 0 ] * scale[ 0 ];
						bl[ 1 ] += lightmap[ i * 3 + 1 ] * scale[ 1 ];
						bl[ 2 ] += lightmap[ i * 3 + 2 ] * scale[ 2 ];
					}
				}
				lightmap += size * 3;		// skip to next lightmap
			}
		}

		// add all the dynamic lights
		if ( surf->dlightframe == tr.frameCount ) {
			R_AddDynamicLightsQ2( surf );
		}
	}

	// put into texture format
	stride -= ( smax << 2 );
	float* bl = s_blocklights_q2;

	for ( int i = 0; i < tmax; i++, dest += stride ) {
		for ( int j = 0; j < smax; j++ ) {
			int r = idMath::FtoiFast( bl[ 0 ] );
			int g = idMath::FtoiFast( bl[ 1 ] );
			int b = idMath::FtoiFast( bl[ 2 ] );

			// catch negative lights
			if ( r < 0 ) {
				r = 0;
			}
			if ( g < 0 ) {
				g = 0;
			}
			if ( b < 0 ) {
				b = 0;
			}

			/*
			** determine the brightest of the three color components
			*/
			int max;
			if ( r > g ) {
				max = r;
			} else {
				max = g;
			}
			if ( b > max ) {
				max = b;
			}

			/*
			** rescale all the color components if the intensity of the greatest
			** channel exceeds 1.0
			*/
			if ( max > 255 ) {
				float t = 255.0F / max;

				r = r * t;
				g = g * t;
				b = b * t;
			}

			dest[ 0 ] = r;
			dest[ 1 ] = g;
			dest[ 2 ] = b;
			dest[ 3 ] = 255;

			bl += 3;
			dest += 4;
		}
	}
}

void GL_BeginBuildingLightmaps( model_t* m ) {
	Com_Memset( gl_lms.allocated, 0, sizeof ( gl_lms.allocated ) );

	tr.frameCount = 1;		// no dlightcache

	/*
	** setup the base lightstyles so the lightmaps won't have to be regenerated
	** the first time they're seen
	*/
	for ( int i = 0; i < MAX_LIGHTSTYLES; i++ ) {
		backEndData[ tr.smpFrame ]->lightstyles[ i ].rgb[ 0 ] = 1;
		backEndData[ tr.smpFrame ]->lightstyles[ i ].rgb[ 1 ] = 1;
		backEndData[ tr.smpFrame ]->lightstyles[ i ].rgb[ 2 ] = 1;
		backEndData[ tr.smpFrame ]->lightstyles[ i ].white = 3;
	}
	tr.refdef.lightstyles = backEndData[ tr.smpFrame ]->lightstyles;

	byte dummy[ 128 * 128 * 4 ];
	if ( !tr.lightmaps[ 0 ] ) {
		for ( int i = 0; i < MAX_LIGHTMAPS; i++ ) {
			tr.lightmaps[ i ] = R_CreateImage( va( "*lightmap%d", i ), dummy, BLOCK_WIDTH, BLOCK_HEIGHT, false, false, GL_CLAMP, false );
		}
	}

	gl_lms.current_lightmap_texture = 0;
}

void GL_CreateSurfaceLightmapQ2( mbrush38_surface_t* surf ) {
	if ( surf->flags & ( BRUSH38_SURF_DRAWSKY | BRUSH38_SURF_DRAWTURB ) ) {
		return;
	}

	int smax = ( surf->extents[ 0 ] >> 4 ) + 1;
	int tmax = ( surf->extents[ 1 ] >> 4 ) + 1;

	if ( !LM_AllocBlock( smax, tmax, &surf->light_s, &surf->light_t ) ) {
		LM_UploadBlock();
		LM_InitBlock();
		if ( !LM_AllocBlock( smax, tmax, &surf->light_s, &surf->light_t ) ) {
			common->FatalError( "Consecutive calls to LM_AllocBlock(%d,%d) failed\n", smax, tmax );
		}
	}

	surf->lightmaptexturenum = gl_lms.current_lightmap_texture;

	byte* base = gl_lms.lightmap_buffer;
	base += ( surf->light_t * BLOCK_WIDTH + surf->light_s ) * LIGHTMAP_BYTES;

	R_SetCacheState( surf );
	R_BuildLightMapQ2( surf, base, BLOCK_WIDTH * LIGHTMAP_BYTES );
}

void GL_EndBuildingLightmaps() {
	LM_UploadBlock();
}

static void R_UpdateSurfaceLightmap( mbrush38_surface_t* surf ) {
	if ( surf->texinfo->flags & ( BSP38SURF_SKY | BSP38SURF_TRANS33 | BSP38SURF_TRANS66 | BSP38SURF_WARP ) ) {
		return;
	}

	bool is_dynamic = false;

	int map;
	for ( map = 0; map < BSP38_MAXLIGHTMAPS && surf->styles[ map ] != 255; map++ ) {
		if ( tr.refdef.lightstyles[ surf->styles[ map ] ].white != surf->cached_light[ map ] ) {
			goto dynamic;
		}
	}

	// dynamic this frame or dynamic previously
	if ( surf->cached_dlight || surf->dlightframe == tr.frameCount ) {
dynamic:
		if ( r_dynamic->value ) {
			is_dynamic = true;
		}
	}

	if ( is_dynamic ) {
		int smax = ( surf->extents[ 0 ] >> 4 ) + 1;
		int tmax = ( surf->extents[ 1 ] >> 4 ) + 1;

		unsigned temp[ 128 * 128 ];
		R_BuildLightMapQ2( surf, ( byte* )temp, smax * 4 );

		if ( ( surf->styles[ map ] >= 32 || surf->styles[ map ] == 0 ) && ( surf->dlightframe != tr.frameCount ) ) {
			R_SetCacheState( surf );
		}
		surf->cached_dlight = surf->dlightframe == tr.frameCount;

		GL_Bind( tr.lightmaps[ surf->lightmaptexturenum ] );

		qglTexSubImage2D( GL_TEXTURE_2D, 0, surf->light_s, surf->light_t, smax, tmax,
			GL_RGBA, GL_UNSIGNED_BYTE, temp );
	}
}

//	Returns the proper texture for a given time and base texture
static image_t* R_TextureAnimationQ2( mbrush38_texinfo_t* tex ) {
	if ( !tex->next ) {
		return tex->image;
	}

	int c = backEnd.currentEntity->e.frame % tex->numframes;
	while ( c ) {
		tex = tex->next;
		c--;
	}

	return tex->image;
}

//	Does a water warp on the pre-fragmented mbrush38_glpoly_t chain
static void EmitWaterPolysQ2( mbrush38_surface_t* fa, int alpha, shaderStage_t* pStage ) {
	texModInfo_t texmods[2] = {};
	texmods[0].type = TMOD_TURBULENT_OLD;
	texmods[0].wave.frequency = 1.0f / idMath::TWO_PI;
	texmods[0].wave.amplitude = 1.0f / 16.0f;
	pStage->bundle[ 0 ].texMods = texmods;
	pStage->bundle[ 0 ].numTexMods = 1;
	if ( fa->texinfo->flags & BSP38SURF_FLOWING ) {
		texmods[ 1 ].type = TMOD_SCROLL;
		texmods[ 1 ].scroll[ 0 ] = -0.5;
		pStage->bundle[ 0 ].numTexMods = 2;
	}
	pStage->rgbGen = CGEN_IDENTITY_LIGHTING;
	setArraysOnce = true;
	EnableArrays( tess.numVertexes );
	shader_t shader = {};
	shader.stages[ 0 ] = pStage;
	tess.shader = &shader;
	tess.xstages = shader.stages;
	RB_IterateStagesGenericTemp( &tess, 0 );
	tess.numVertexes = 0;
	tess.numIndexes = 0;
	DisableArrays();
}

static void R_RenderBrushWaterPolyQ2( mbrush38_surface_t* fa ) {
	c_brush_polys++;

	image_t* image = R_TextureAnimationQ2( fa->texinfo );

	// warp texture, no lightmaps
	shaderStage_t stage = {};
	stage.bundle[ 0 ].image[ 0 ] = image;
	stage.bundle[ 0 ].numImageAnimations = 1;
	stage.bundle[ 0 ].tcGen = TCGEN_TEXTURE;
	stage.alphaGen = AGEN_IDENTITY;
	stage.stateBits = GLS_DEFAULT;
	EmitWaterPolysQ2( fa, 255, &stage );
}

static void R_RenderBrushPolyQ2( mbrush38_surface_t* fa, image_t* image ) {
	shaderStage_t stage1 = {};
	texModInfo_t texmod = {};
	stage1.bundle[ 0 ].image[ 0 ] = image;
	stage1.bundle[ 0 ].numImageAnimations = 1;
	stage1.rgbGen = CGEN_IDENTITY;
	stage1.alphaGen = AGEN_IDENTITY;
	stage1.stateBits = GLS_DEFAULT;
	stage1.bundle[ 0 ].tcGen = TCGEN_TEXTURE;
	if ( fa->texinfo->flags & BSP38SURF_FLOWING ) {
		texmod.type = TMOD_SCROLL;
		texmod.scroll[ 0 ] = -1.6;
		stage1.bundle[ 0 ].texMods = &texmod;
		stage1.bundle[ 0 ].numTexMods = 1;
	}
	shader_t shader = {};
	shader.stages[ 0 ] = &stage1;
	tess.shader = &shader;
	tess.xstages = shader.stages;

	// don't bother if we're set to fullbright
	if ( !r_fullbright->value && tr.worldModel->brush38_lightdata ) {
		shaderStage_t stage2 = {};
		stage2.bundle[ 0 ].image[ 0 ] = tr.lightmaps[ fa->lightmaptexturenum ];
		stage2.bundle[ 0 ].numImageAnimations = 1;
		stage2.bundle[ 0 ].tcGen = TCGEN_LIGHTMAP;
		stage2.rgbGen = CGEN_IDENTITY;
		stage2.alphaGen = AGEN_IDENTITY;
		stage2.stateBits = GLS_SRCBLEND_ZERO | GLS_DSTBLEND_SRC_COLOR;
		stage2.bundle[ 0 ].isLightmap = true;
		shader.stages[ 1 ] = &stage2;
	}

	setArraysOnce = false;
	EnableArrays( tess.numVertexes );
	RB_IterateStagesGenericTemp( &tess, 0 );
	RB_IterateStagesGenericTemp( &tess, 1 );
	DisableArrays();
}

void GL_RenderLightmappedPoly( mbrush38_surface_t* surf ) {
	for ( mbrush38_glpoly_t* p = surf->polys; p; p = p->next ) {
		int numVerts = tess.numVertexes;
		int numIndexes = tess.numIndexes;

		tess.numVertexes += p->numverts;
		tess.numIndexes += ( p->numverts - 2 ) * 3;

		float* v = p->verts[ 0 ];
		for ( int i = 0; i < p->numverts; i++, v += BRUSH38_VERTEXSIZE ) {
			tess.xyz[ numVerts + i ][ 0 ] = v[ 0 ];
			tess.xyz[ numVerts + i ][ 1 ] = v[ 1 ];
			tess.xyz[ numVerts + i ][ 2 ] = v[ 2 ];
			tess.texCoords[ numVerts + i ][ 0 ][ 0 ] = v[ 3 ];
			tess.texCoords[ numVerts + i ][ 0 ][ 1 ] = v[ 4 ];
			tess.texCoords[ numVerts + i ][ 1 ][ 0 ] = v[ 5 ];
			tess.texCoords[ numVerts + i ][ 1 ][ 1 ] = v[ 6 ];
			if ( surf->flags & BRUSH38_SURF_PLANEBACK ) {
				tess.normal[ numVerts + i ][ 0 ] = -surf->plane->normal[ 0 ];
				tess.normal[ numVerts + i ][ 1 ] = -surf->plane->normal[ 1 ];
				tess.normal[ numVerts + i ][ 2 ] = -surf->plane->normal[ 2 ];
			} else {
				tess.normal[ numVerts + i ][ 0 ] = surf->plane->normal[ 0 ];
				tess.normal[ numVerts + i ][ 1 ] = surf->plane->normal[ 1 ];
				tess.normal[ numVerts + i ][ 2 ] = surf->plane->normal[ 2 ];
			}
		}
		for ( int i = 0; i < p->numverts - 2; i++ ) {
			tess.indexes[ numIndexes + i * 3 + 0 ] = numVerts;
			tess.indexes[ numIndexes + i * 3 + 1 ] = numVerts + i + 1;
			tess.indexes[ numIndexes + i * 3 + 2 ] = numVerts + i + 2;
		}
	}

	GL_Cull( CT_FRONT_SIDED );
	if ( surf->texinfo->flags & ( BSP38SURF_TRANS33 | BSP38SURF_TRANS66 ) ) {

		// the textures are prescaled up for a better lighting range,
		// so scale it back down

		c_brush_polys++;
		int alpha;
		if ( surf->texinfo->flags & BSP38SURF_TRANS33 ) {
			alpha = 84;
		} else {
			alpha = 168;
		}
		if ( surf->flags & BRUSH38_SURF_DRAWTURB ) {
			shaderStage_t stage = {};
			stage.bundle[ 0 ].image[ 0 ] = surf->texinfo->image;
			stage.bundle[ 0 ].numImageAnimations = 1;
			stage.bundle[ 0 ].tcGen = TCGEN_TEXTURE;
			stage.stateBits = GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			stage.alphaGen = AGEN_CONST;
			stage.constantColor[ 3 ] = alpha;
			EmitWaterPolysQ2( surf, alpha, &stage );
		} else {
			shaderStage_t stage = {};
			stage.bundle[ 0 ].image[ 0 ] = surf->texinfo->image;
			stage.bundle[ 0 ].numImageAnimations = 1;
			stage.bundle[ 0 ].tcGen = TCGEN_TEXTURE;
			stage.rgbGen = CGEN_IDENTITY;
			stage.alphaGen = AGEN_CONST;
			stage.constantColor[ 3 ] = alpha;
			stage.stateBits = GLS_DEFAULT | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			setArraysOnce = true;
			EnableArrays( tess.numVertexes );
			shader_t shader = {};
			shader.stages[ 0 ] = &stage;
			tess.shader = &shader;
			tess.xstages = shader.stages;
			RB_IterateStagesGenericTemp( &tess, 0 );
			tess.numIndexes = 0;
			tess.numVertexes = 0;
			DisableArrays();
		}
		return;
	}

	if ( surf->flags & BRUSH38_SURF_DRAWTURB ) {
		R_RenderBrushWaterPolyQ2( surf );
		return;
	}

	R_UpdateSurfaceLightmap( surf );

	c_brush_polys++;

	image_t* image = R_TextureAnimationQ2( surf->texinfo );

	if ( !qglMultiTexCoord2fARB ) {
		R_RenderBrushPolyQ2( surf, image );
		tess.numVertexes = 0;
		tess.numIndexes = 0;
		return;
	}

	shader_t shader = {};
	tess.shader = &shader;
	tess.xstages = shader.stages;
	shader.multitextureEnv = GL_MODULATE;

	shaderStage_t stage = {};
	texModInfo_t texmod = {};
	stage.bundle[ 0 ].image[ 0 ] = image;
	stage.bundle[ 0 ].numImageAnimations = 1;
	stage.bundle[ 0 ].tcGen = TCGEN_TEXTURE;
	stage.bundle[ 1 ].image[ 0 ] = tr.lightmaps[ surf->lightmaptexturenum ];
	stage.bundle[ 1 ].numImageAnimations = 1;
	stage.bundle[ 1 ].tcGen = TCGEN_LIGHTMAP;
	stage.rgbGen = CGEN_IDENTITY;
	stage.alphaGen = AGEN_IDENTITY;
	stage.stateBits = GLS_DEFAULT;
	shader.stages[ 0 ] = &stage;
	if ( surf->texinfo->flags & BSP38SURF_FLOWING ) {
		texmod.type = TMOD_SCROLL;
		texmod.scroll[ 0 ] = -1.6;
		stage.bundle[ 0 ].texMods = &texmod;
		stage.bundle[ 0 ].numTexMods = 1;
	}
	setArraysOnce = false;
	EnableArrays( tess.numVertexes );
	RB_IterateStagesGenericTemp( &tess, 0 );
	DisableArrays();
	tess.numVertexes = 0;
	tess.numIndexes = 0;
}

//	Draw water surfaces and windows.
//	The BSP tree is waled front to back, so unwinding the chain of
// alpha_surfaces will draw back to front, giving proper ordering.
void R_DrawAlphaSurfaces(int& forcedSortIndex) {
	int savedShiftedEntityNum = tr.shiftedEntityNum;
	tr.shiftedEntityNum = REF_ENTITYNUM_WORLD << QSORT_ENTITYNUM_SHIFT;
	for ( mbrush38_surface_t* s = r_alpha_surfaces; s; s = s->texturechain ) {
		R_AddDrawSurf( (surfaceType_t*)s, tr.defaultShader, 0, false, false, ATI_TESS_NONE, forcedSortIndex );
	}
	r_alpha_surfaces = NULL;
	tr.shiftedEntityNum = savedShiftedEntityNum;
}
