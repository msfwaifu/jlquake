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

#include "model.h"
#include "../main.h"
#include "../commands.h"
#include "../cvars.h"
#include "../decals.h"
#include "../../../common/Common.h"
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"
#include "../../../common/command_buffer.h"
#include "../../../common/endian.h"
#include "../../../common/file_formats/bsp47.h"
#include "RenderModelBSP46.h"
#include "../SurfaceSkip.h"
#include "../SurfaceFace.h"
#include "../SurfaceGrid.h"
#include "../SurfaceTriangles.h"
#include "../SurfaceFoliage.h"

#define LIGHTMAP_SIZE       128

#define MAX_FACE_POINTS     64

world_t s_worldData;

static byte* fileBase;

static void HSVtoRGB( float h, float s, float v, float rgb[ 3 ] ) {
	h *= 5;

	int i = ( int )floor( h );
	float f = h - i;

	float p = v * ( 1 - s );
	float q = v * ( 1 - s * f );
	float t = v * ( 1 - s * ( 1 - f ) );

	switch ( i ) {
	case 0:
		rgb[ 0 ] = v;
		rgb[ 1 ] = t;
		rgb[ 2 ] = p;
		break;
	case 1:
		rgb[ 0 ] = q;
		rgb[ 1 ] = v;
		rgb[ 2 ] = p;
		break;
	case 2:
		rgb[ 0 ] = p;
		rgb[ 1 ] = v;
		rgb[ 2 ] = t;
		break;
	case 3:
		rgb[ 0 ] = p;
		rgb[ 1 ] = q;
		rgb[ 2 ] = v;
		break;
	case 4:
		rgb[ 0 ] = t;
		rgb[ 1 ] = p;
		rgb[ 2 ] = v;
		break;
	case 5:
		rgb[ 0 ] = v;
		rgb[ 1 ] = p;
		rgb[ 2 ] = q;
		break;
	}
}

static void R_ColorShiftLightingBytes( byte in[ 4 ], byte out[ 4 ] ) {
	// shift the color data based on overbright range
	int shift = r_mapOverBrightBits->integer - tr.overbrightBits;

	// shift the data based on overbright range
	int r = in[ 0 ] << shift;
	int g = in[ 1 ] << shift;
	int b = in[ 2 ] << shift;

	// normalize by color instead of saturating to white
	if ( ( r | g | b ) > 255 ) {
		int max = r > g ? r : g;
		max = max > b ? max : b;
		r = r * 255 / max;
		g = g * 255 / max;
		b = b * 255 / max;
	}

	out[ 0 ] = r;
	out[ 1 ] = g;
	out[ 2 ] = b;
	out[ 3 ] = in[ 3 ];
}

float R_ProcessLightmap( byte* buf_p, int in_padding, int width, int height, byte* image ) {
	float maxIntensity = 0;
	if ( r_lightmap->integer > 1 ) {
		// color code by intensity as development tool	(FIXME: check range)
		for ( int j = 0; j < width * height; j++ ) {
			float r = buf_p[ j * in_padding + 0 ];
			float g = buf_p[ j * in_padding + 1 ];
			float b = buf_p[ j * in_padding + 2 ];
			float intensity;
			float out[ 3 ];

			intensity = 0.33f * r + 0.685f * g + 0.063f * b;

			if ( intensity > 255 ) {
				intensity = 1.0f;
			} else {
				intensity /= 255.0f;
			}

			if ( intensity > maxIntensity ) {
				maxIntensity = intensity;
			}

			HSVtoRGB( intensity, 1.00, 0.50, out );

			if ( r_lightmap->integer == 3 ) {
				// Arnout: artists wanted the colours to be inversed
				image[ j * 4 + 0 ] = out[ 2 ] * 255;
				image[ j * 4 + 1 ] = out[ 1 ] * 255;
				image[ j * 4 + 2 ] = out[ 0 ] * 255;
			} else {
				image[ j * 4 + 0 ] = out[ 0 ] * 255;
				image[ j * 4 + 1 ] = out[ 1 ] * 255;
				image[ j * 4 + 2 ] = out[ 2 ] * 255;
				image[ j * 4 + 3 ] = 255;
			}
		}
	} else {
		for ( int j = 0; j < width * height; j++ ) {
			R_ColorShiftLightingBytes( &buf_p[ j * in_padding ], &image[ j * 4 ] );
			image[ j * 4 + 3 ] = 255;
		}
	}
	return maxIntensity;
}

static void R_LoadLightmaps( bsp_lump_t* l ) {
	// ydnar: clear lightmaps first
	tr.numLightmaps = 0;
	memset( tr.lightmaps, 0, sizeof ( tr.lightmaps ) );

	int len = l->filelen;
	if ( !len ) {
		return;
	}
	byte* buf = fileBase + l->fileofs;

	// we are about to upload textures
	R_SyncRenderThread();

	// create all the lightmaps
	tr.numLightmaps = len / ( LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3 );
	if ( tr.numLightmaps == 1 ) {
		//FIXME: HACK: maps with only one lightmap turn up fullbright for some reason.
		//this avoids this, but isn't the correct solution.
		tr.numLightmaps++;
	}

	// if we are in r_vertexLight mode, we don't need the lightmaps at all
	if ( r_vertexLight->integer ) {
		return;
	}

	float maxIntensity = 0;
	for ( int i = 0; i < tr.numLightmaps; i++ ) {
		// expand the 24 bit on-disk to 32 bit
		byte* buf_p = buf + i * LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3;

		byte image[ LIGHTMAP_SIZE * LIGHTMAP_SIZE * 4 ];
		float intensity = R_ProcessLightmap( buf_p, 3, LIGHTMAP_SIZE, LIGHTMAP_SIZE, image );
		if ( intensity > maxIntensity ) {
			maxIntensity = intensity;
		}
		tr.lightmaps[ i ] = R_CreateImage( va( "*lightmap%d",i ), image,
			LIGHTMAP_SIZE, LIGHTMAP_SIZE, false, false, GL_CLAMP );
	}

	if ( r_lightmap->integer > 1 ) {
		common->Printf( "Brightest lightmap value: %d\n", ( int )( maxIntensity * 255 ) );
	}
}

static void R_LoadVisibility( bsp_lump_t* l ) {
	int len = ( s_worldData.numClusters + 63 ) & ~63;
	s_worldData.novis = new byte[ len ];
	Com_Memset( s_worldData.novis, 0xff, len );

	len = l->filelen;
	if ( !len ) {
		return;
	}
	byte* buf = fileBase + l->fileofs;

	s_worldData.numClusters = LittleLong( ( ( int* )buf )[ 0 ] );
	s_worldData.clusterBytes = LittleLong( ( ( int* )buf )[ 1 ] );

	byte* dest = new byte[ len - 8 ];
	Com_Memcpy( dest, buf + 8, len - 8 );
	s_worldData.vis = dest;
}

static shader_t* ShaderForShaderNum( int shaderNum, int lightmapNum ) {
	shaderNum = LittleLong( shaderNum );
	if ( shaderNum < 0 || shaderNum >= s_worldData.numShaders ) {
		common->Error( "ShaderForShaderNum: bad num %i", shaderNum );
	}
	bsp46_dshader_t* dsh = &s_worldData.shaders[ shaderNum ];

	if ( r_vertexLight->integer ) {
		lightmapNum = LIGHTMAP_BY_VERTEX;
	}

	if ( !( GGameType & ( GAME_WolfMP | GAME_ET ) ) && r_fullbright->integer ) {
		lightmapNum = LIGHTMAP_WHITEIMAGE;
	}

	shader_t* shader = R_FindShader( dsh->shader, lightmapNum, true );

	// if the shader had errors, just use default shader
	if ( shader->defaultShader ) {
		return tr.defaultShader;
	}

	return shader;
}

//	handles final surface classification
static void FinishGenericSurface( idWorldSurface* surf, bsp46_dsurface_t* ds, const idVec3& pt ) {
	// set bounding sphere
	surf->boundingSphere = surf->bounds.ToSphere();

	// take the plane normal from the lightmap vector and classify it
	surf->plane.Normal()[ 0 ] = LittleFloat( ds->lightmapVecs[ 2 ][ 0 ] );
	surf->plane.Normal()[ 1 ] = LittleFloat( ds->lightmapVecs[ 2 ][ 1 ] );
	surf->plane.Normal()[ 2 ] = LittleFloat( ds->lightmapVecs[ 2 ][ 2 ] );
	surf->plane.FitThroughPoint( pt );
}

static idWorldSurface* ParseFace( bsp46_dsurface_t* ds, bsp46_drawVert_t* verts, int* indexes ) {
	idSurfaceFace* surf = new idSurfaceFace;
	int lightmapNum = LittleLong( ds->lightmapNum );

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader value
	surf->shader = ShaderForShaderNum( ds->shaderNum, lightmapNum );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	int numPoints = LittleLong( ds->numVerts );
	if ( numPoints > MAX_FACE_POINTS ) {
		common->Printf( S_COLOR_YELLOW "WARNING: MAX_FACE_POINTS exceeded: %i\n", numPoints );
		numPoints = MAX_FACE_POINTS;
		surf->shader = tr.defaultShader;
	}

	int numIndexes = LittleLong( ds->numIndexes );

	surf->numVertexes = numPoints;
	surf->vertexes = new idWorldVertex[ numPoints ];
	surf->numIndexes = numIndexes;
	surf->indexes = new int[ numIndexes ];

	surf->bounds.Clear();
	verts += LittleLong( ds->firstVert );
	for ( int i = 0; i < numPoints; i++ ) {
		for ( int j = 0; j < 3; j++ ) {
			surf->vertexes[ i ].xyz[ j ] = LittleFloat( verts[ i ].xyz[ j ] );
			surf->vertexes[ i ].normal[ j ] = LittleFloat( verts[ i ].normal[ j ] );
		}
		surf->bounds.AddPoint( surf->vertexes[ i ].xyz );
		for ( int j = 0; j < 2; j++ ) {
			surf->vertexes[ i ].st[ j ] = LittleFloat( verts[ i ].st[ j ] );
			surf->vertexes[ i ].lightmap[ j ] = LittleFloat( verts[ i ].lightmap[ j ] );
		}
		R_ColorShiftLightingBytes( verts[ i ].color, surf->vertexes[ i ].color );
	}

	indexes += LittleLong( ds->firstIndex );
	for ( int i = 0; i < numIndexes; i++ ) {
		surf->indexes[ i ] = LittleLong( indexes[ i ] );
	}

	// finish surface
	FinishGenericSurface( surf, ds, surf->vertexes[ 0 ].xyz );

	return surf;
}

static idWorldSurface* ParseMesh( bsp46_dsurface_t* ds, bsp46_drawVert_t* verts ) {
	// we may have a nodraw surface, because they might still need to
	// be around for movement clipping
	if ( s_worldData.shaders[ LittleLong( ds->shaderNum ) ].surfaceFlags & BSP46SURF_NODRAW ) {
		idSurfaceSkip* surf = new idSurfaceSkip;
		int lightmapNum = LittleLong( ds->lightmapNum );

		// get fog volume
		surf->fogIndex = LittleLong( ds->fogNum ) + 1;

		// get shader value
		surf->shader = ShaderForShaderNum( ds->shaderNum, lightmapNum );
		if ( r_singleShader->integer && !surf->shader->isSky ) {
			surf->shader = tr.defaultShader;
		}
		return surf;
	}

	idSurfaceGrid* surf = new idSurfaceGrid;
	int lightmapNum = LittleLong( ds->lightmapNum );

	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader value
	surf->shader = ShaderForShaderNum( ds->shaderNum, lightmapNum );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	int width = LittleLong( ds->patchWidth );
	int height = LittleLong( ds->patchHeight );

	verts += LittleLong( ds->firstVert );
	int numPoints = width * height;
	idWorldVertex points[ MAX_PATCH_SIZE * MAX_PATCH_SIZE ];
	for ( int i = 0; i < numPoints; i++ ) {
		for ( int j = 0; j < 3; j++ ) {
			points[ i ].xyz[ j ] = LittleFloat( verts[ i ].xyz[ j ] );
			points[ i ].normal[ j ] = LittleFloat( verts[ i ].normal[ j ] );
		}
		for ( int j = 0; j < 2; j++ ) {
			points[ i ].st[ j ] = LittleFloat( verts[ i ].st[ j ] );
			points[ i ].lightmap[ j ] = LittleFloat( verts[ i ].lightmap[ j ] );
		}
		R_ColorShiftLightingBytes( verts[ i ].color, points[ i ].color );
	}

	// pre-tesseleate
	R_SubdividePatchToGrid( surf, width, height, points );

	// copy the level of detail origin, which is the center
	// of the group of all curves that must subdivide the same
	// to avoid cracking
	idVec3 bounds[ 2 ];
	for ( int i = 0; i < 3; i++ ) {
		bounds[ 0 ][ i ] = LittleFloat( ds->lightmapVecs[ 0 ][ i ] );
		bounds[ 1 ][ i ] = LittleFloat( ds->lightmapVecs[ 1 ][ i ] );
	}
	surf->lodOrigin = (bounds[ 0 ] + bounds[ 1 ]) * 0.5f;
	surf->lodRadius = (bounds[ 0 ] - surf->lodOrigin).Length();

	// finish surface
	FinishGenericSurface( surf, ds, surf->vertexes[ 0 ].xyz );
	return surf;
}

static idWorldSurface* ParseTriSurf( bsp46_dsurface_t* ds, bsp46_drawVert_t* verts, int* indexes ) {
	idSurfaceTriangles* surf = new idSurfaceTriangles;
	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader
	surf->shader = ShaderForShaderNum( ds->shaderNum,
		GGameType & GAME_ET ? LittleLong( ds->lightmapNum ) : LIGHTMAP_BY_VERTEX );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	int numVerts = LittleLong( ds->numVerts );
	int numIndexes = LittleLong( ds->numIndexes );

	surf->numVertexes = numVerts;
	surf->vertexes = new idWorldVertex[ numVerts ];
	surf->numIndexes = numIndexes;
	surf->indexes = new int[ numIndexes ];

	// copy vertexes
	surf->bounds.Clear();
	verts += LittleLong( ds->firstVert );
	for ( int i = 0; i < numVerts; i++ ) {
		for ( int j = 0; j < 3; j++ ) {
			surf->vertexes[ i ].xyz[ j ] = LittleFloat( verts[ i ].xyz[ j ] );
			surf->vertexes[ i ].normal[ j ] = LittleFloat( verts[ i ].normal[ j ] );
		}
		surf->bounds.AddPoint( surf->vertexes[ i ].xyz );
		for ( int j = 0; j < 2; j++ ) {
			surf->vertexes[ i ].st[ j ] = LittleFloat( verts[ i ].st[ j ] );
			surf->vertexes[ i ].lightmap[ j ] = LittleFloat( verts[ i ].lightmap[ j ] );
		}

		R_ColorShiftLightingBytes( verts[ i ].color, surf->vertexes[ i ].color );
	}

	// copy indexes
	indexes += LittleLong( ds->firstIndex );
	for ( int i = 0; i < numIndexes; i++ ) {
		surf->indexes[ i ] = LittleLong( indexes[ i ] );
		if ( surf->indexes[ i ] < 0 || surf->indexes[ i ] >= numVerts ) {
			common->Error( "Bad index in triangle surface" );
		}
	}

	// finish surface
	FinishGenericSurface( surf, ds, surf->vertexes[ 0 ].xyz );
	return surf;
}

static idWorldSurface* ParseFlare( bsp46_dsurface_t* ds ) {
	idSurfaceSkip* surf = new idSurfaceSkip;
	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader
	surf->shader = ShaderForShaderNum( ds->shaderNum, LIGHTMAP_BY_VERTEX );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	return surf;
}

static idWorldSurface* ParseFoliage( bsp46_dsurface_t* ds, bsp46_drawVert_t* verts, int* indexes ) {
	idSurfaceFoliage* surf = new idSurfaceFoliage;
	// get fog volume
	surf->fogIndex = LittleLong( ds->fogNum ) + 1;

	// get shader
	surf->shader = ShaderForShaderNum( ds->shaderNum, LIGHTMAP_BY_VERTEX );
	if ( r_singleShader->integer && !surf->shader->isSky ) {
		surf->shader = tr.defaultShader;
	}

	// foliage surfaces have their actual vert count in patchHeight
	// and the instance count in patchWidth
	// the instances are just additional drawverts

	// get counts
	int numVerts = LittleLong( ds->patchHeight );
	int numIndexes = LittleLong( ds->numIndexes );
	int numInstances = LittleLong( ds->patchWidth );

	// set up surface
	surf->numVertexes = numVerts;
	surf->numIndexes = numIndexes;
	surf->numInstances = numInstances;
	surf->vertexes = new idWorldVertex[ numVerts ];
	surf->indexes = new int[ numIndexes ];
	surf->instances = new foliageInstance_t[ numInstances ];

	// get foliage drawscale
	float scale = r_drawfoliage->value;
	if ( scale < 0.0f ) {
		scale = 1.0f;
	} else if ( scale > 2.0f ) {
		scale = 2.0f;
	}

	// copy vertexes
	idBounds bounds;
	bounds.Clear();
	verts += LittleLong( ds->firstVert );
	for ( int i = 0; i < numVerts; i++ ) {
		// copy xyz and normal
		for ( int j = 0; j < 3; j++ ) {
			surf->vertexes[ i ].xyz[ j ] = LittleFloat( verts[ i ].xyz[ j ] );
			surf->vertexes[ i ].normal[ j ] = LittleFloat( verts[ i ].normal[ j ] );
		}

		// scale height
		surf->vertexes[ i ].xyz.z *= scale;

		// finish
		bounds.AddPoint( surf->vertexes[ i ].xyz );

		// copy texture coordinates
		for ( int j = 0; j < 2; j++ ) {
			surf->vertexes[ i ].st[ j ] = LittleFloat( verts[ i ].st[ j ] );
			surf->vertexes[ i ].lightmap[ j ] = LittleFloat( verts[ i ].lightmap[ j ] );
		}
	}

	// copy indexes
	indexes += LittleLong( ds->firstIndex );
	for ( int i = 0; i < numIndexes; i++ ) {
		surf->indexes[ i ] = LittleLong( indexes[ i ] );
		if ( surf->indexes[ i ] < 0 || surf->indexes[ i ] >= numVerts ) {
			common->Error( "Bad index in triangle surface" );
		}
	}

	// copy origins and colors
	surf->bounds.Clear();
	verts += numVerts;
	for ( int i = 0; i < numInstances; i++ ) {
		// copy xyz
		for ( int j = 0; j < 3; j++ ) {
			surf->instances[ i ].origin[ j ] = LittleFloat( verts[ i ].xyz[ j ] );
		}
		idVec3 neworg;
		neworg.FromOldVec3( surf->instances[ i ].origin );
		surf->bounds.AddBounds( bounds + neworg );

		// copy color
		R_ColorShiftLightingBytes( verts[ i ].color, surf->instances[ i ].color );
	}

	// finish surface
	FinishGenericSurface( surf, ds, surf->vertexes[ 0 ].xyz );
	return surf;
}

//	returns true if there are grid points merged on a width edge
static bool R_MergedWidthPoints( idSurfaceGrid* grid, int offset ) {
	for ( int i = 1; i < grid->width - 1; i++ ) {
		for ( int j = i + 1; j < grid->width - 1; j++ ) {
			if ( idMath::Fabs( grid->vertexes[ i + offset ].xyz.x - grid->vertexes[ j + offset ].xyz.x ) > .1 ) {
				continue;
			}
			if ( idMath::Fabs( grid->vertexes[ i + offset ].xyz.y - grid->vertexes[ j + offset ].xyz.y ) > .1 ) {
				continue;
			}
			if ( idMath::Fabs( grid->vertexes[ i + offset ].xyz.z - grid->vertexes[ j + offset ].xyz.z ) > .1 ) {
				continue;
			}
			return true;
		}
	}
	return false;
}

//	returns true if there are grid points merged on a height edge
static bool R_MergedHeightPoints( idSurfaceGrid* grid, int offset ) {
	for ( int i = 1; i < grid->height - 1; i++ ) {
		for ( int j = i + 1; j < grid->height - 1; j++ ) {
			if ( idMath::Fabs( grid->vertexes[ grid->width * i + offset ].xyz.x - grid->vertexes[ grid->width * j + offset ].xyz.x ) > .1 ) {
				continue;
			}
			if ( idMath::Fabs( grid->vertexes[ grid->width * i + offset ].xyz.y - grid->vertexes[ grid->width * j + offset ].xyz.y ) > .1 ) {
				continue;
			}
			if ( idMath::Fabs( grid->vertexes[ grid->width * i + offset ].xyz.z - grid->vertexes[ grid->width * j + offset ].xyz.z ) > .1 ) {
				continue;
			}
			return true;
		}
	}
	return false;
}

//	NOTE: never sync LoD through grid edges with merged points!
//
//	FIXME: write generalized version that also avoids cracks between a patch and one that meets half way?
static void R_FixSharedVertexLodError_r( int start, idSurfaceGrid* grid1 ) {
	int k, l, m, n, offset1, offset2;

	for ( int j = start; j < s_worldData.numsurfaces; j++ ) {
		// if this surface is not a grid
		if ( !s_worldData.surfaces[ j ]->IsGrid() ) {
			continue;
		}
		//
		idSurfaceGrid* grid2 = ( idSurfaceGrid* )s_worldData.surfaces[ j ];
		// if the LOD errors are already fixed for this patch
		if ( grid2->lodFixed == 2 ) {
			continue;
		}
		// grids in the same LOD group should have the exact same lod radius
		if ( grid1->lodRadius != grid2->lodRadius ) {
			continue;
		}
		// grids in the same LOD group should have the exact same lod origin
		if ( grid1->lodOrigin != grid2->lodOrigin ) {
			continue;
		}
		//
		bool touch = false;
		for ( n = 0; n < 2; n++ ) {
			//
			if ( n ) {
				offset1 = ( grid1->height - 1 ) * grid1->width;
			} else {
				offset1 = 0;
			}
			if ( R_MergedWidthPoints( grid1, offset1 ) ) {
				continue;
			}
			for ( k = 1; k < grid1->width - 1; k++ ) {
				for ( m = 0; m < 2; m++ ) {
					if ( m ) {
						offset2 = ( grid2->height - 1 ) * grid2->width;
					} else {
						offset2 = 0;
					}
					if ( R_MergedWidthPoints( grid2, offset2 ) ) {
						continue;
					}
					for ( l = 1; l < grid2->width - 1; l++ ) {
						if ( idMath::Fabs( grid1->vertexes[ k + offset1 ].xyz.x - grid2->vertexes[ l + offset2 ].xyz.x ) > .1 ) {
							continue;
						}
						if ( idMath::Fabs( grid1->vertexes[ k + offset1 ].xyz.y - grid2->vertexes[ l + offset2 ].xyz.y ) > .1 ) {
							continue;
						}
						if ( idMath::Fabs( grid1->vertexes[ k + offset1 ].xyz.z - grid2->vertexes[ l + offset2 ].xyz.z ) > .1 ) {
							continue;
						}
						// ok the points are equal and should have the same lod error
						grid2->widthLodError[ l ] = grid1->widthLodError[ k ];
						touch = true;
					}
				}
				for ( m = 0; m < 2; m++ ) {
					if ( m ) {
						offset2 = grid2->width - 1;
					} else {
						offset2 = 0;
					}
					if ( R_MergedHeightPoints( grid2, offset2 ) ) {
						continue;
					}
					for ( l = 1; l < grid2->height - 1; l++ ) {
						if ( idMath::Fabs( grid1->vertexes[ k + offset1 ].xyz.x - grid2->vertexes[ grid2->width * l + offset2 ].xyz.x ) > .1 ) {
							continue;
						}
						if ( idMath::Fabs( grid1->vertexes[ k + offset1 ].xyz.y - grid2->vertexes[ grid2->width * l + offset2 ].xyz.y ) > .1 ) {
							continue;
						}
						if ( idMath::Fabs( grid1->vertexes[ k + offset1 ].xyz.z - grid2->vertexes[ grid2->width * l + offset2 ].xyz.z ) > .1 ) {
							continue;
						}
						// ok the points are equal and should have the same lod error
						grid2->heightLodError[ l ] = grid1->widthLodError[ k ];
						touch = true;
					}
				}
			}
		}
		for ( n = 0; n < 2; n++ ) {
			if ( n ) {
				offset1 = grid1->width - 1;
			} else {
				offset1 = 0;
			}
			if ( R_MergedHeightPoints( grid1, offset1 ) ) {
				continue;
			}
			for ( k = 1; k < grid1->height - 1; k++ ) {
				for ( m = 0; m < 2; m++ ) {
					if ( m ) {
						offset2 = ( grid2->height - 1 ) * grid2->width;
					} else {
						offset2 = 0;
					}
					if ( R_MergedWidthPoints( grid2, offset2 ) ) {
						continue;
					}
					for ( l = 1; l < grid2->width - 1; l++ ) {
						if ( idMath::Fabs( grid1->vertexes[ grid1->width * k + offset1 ].xyz.x - grid2->vertexes[ l + offset2 ].xyz.x ) > .1 ) {
							continue;
						}
						if ( idMath::Fabs( grid1->vertexes[ grid1->width * k + offset1 ].xyz.y - grid2->vertexes[ l + offset2 ].xyz.y ) > .1 ) {
							continue;
						}
						if ( idMath::Fabs( grid1->vertexes[ grid1->width * k + offset1 ].xyz.z - grid2->vertexes[ l + offset2 ].xyz.z ) > .1 ) {
							continue;
						}
						// ok the points are equal and should have the same lod error
						grid2->widthLodError[ l ] = grid1->heightLodError[ k ];
						touch = true;
					}
				}
				for ( m = 0; m < 2; m++ ) {
					if ( m ) {
						offset2 = grid2->width - 1;
					} else {
						offset2 = 0;
					}
					if ( R_MergedHeightPoints( grid2, offset2 ) ) {
						continue;
					}
					for ( l = 1; l < grid2->height - 1; l++ ) {
						if ( idMath::Fabs( grid1->vertexes[ grid1->width * k + offset1 ].xyz[ 0 ] - grid2->vertexes[ grid2->width * l + offset2 ].xyz.x ) > .1 ) {
							continue;
						}
						if ( idMath::Fabs( grid1->vertexes[ grid1->width * k + offset1 ].xyz[ 1 ] - grid2->vertexes[ grid2->width * l + offset2 ].xyz.y ) > .1 ) {
							continue;
						}
						if ( idMath::Fabs( grid1->vertexes[ grid1->width * k + offset1 ].xyz[ 2 ] - grid2->vertexes[ grid2->width * l + offset2 ].xyz.z ) > .1 ) {
							continue;
						}
						// ok the points are equal and should have the same lod error
						grid2->heightLodError[ l ] = grid1->heightLodError[ k ];
						touch = true;
					}
				}
			}
		}
		if ( touch ) {
			grid2->lodFixed = 2;
			R_FixSharedVertexLodError_r( start, grid2 );
			//NOTE: this would be correct but makes things really slow
			//grid2->lodFixed = 1;
		}
	}
}

//	This function assumes that all patches in one group are nicely stitched
// together for the highest LoD. If this is not the case this function will
// still do its job but won't fix the highest LoD cracks.
static void R_FixSharedVertexLodError() {
	for ( int i = 0; i < s_worldData.numsurfaces; i++ ) {
		// if this surface is not a grid
		if ( !s_worldData.surfaces[ i ]->IsGrid() ) {
			continue;
		}
		idSurfaceGrid* grid1 = ( idSurfaceGrid* )s_worldData.surfaces[ i ];
		if ( grid1->lodFixed ) {
			continue;
		}
		//
		grid1->lodFixed = 2;
		// recursively fix other patches in the same LOD group
		R_FixSharedVertexLodError_r( i + 1, grid1 );
	}
}

static bool R_StitchPatches( idSurfaceGrid* grid1, idSurfaceGrid* grid2 ) {
	for ( int n = 0; n < 2; n++ ) {
		int offset1;
		if ( n ) {
			offset1 = ( grid1->height - 1 ) * grid1->width;
		} else {
			offset1 = 0;
		}
		if ( R_MergedWidthPoints( grid1, offset1 ) ) {
			continue;
		}
		for ( int k = 0; k < grid1->width - 2; k += 2 ) {
			for ( int m = 0; m < 2; m++ ) {
				if ( grid2->width >= MAX_GRID_SIZE ) {
					break;
				}
				int offset2;
				if ( m ) {
					offset2 = ( grid2->height - 1 ) * grid2->width;
				} else {
					offset2 = 0;
				}
				for ( int l = 0; l < grid2->width - 1; l++ ) {
					{
					const idVec3& v1 = grid1->vertexes[ k + offset1 ].xyz;
					const idVec3& v2 = grid2->vertexes[ l + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.y - v2.y ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.z - v2.z ) > .1 ) {
						continue;
					}
					}

					{
					const idVec3& v1 = grid1->vertexes[ k + 2 + offset1 ].xyz;
					const idVec3& v2 = grid2->vertexes[ l + 1 + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.y - v2.y ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.z - v2.z ) > .1 ) {
						continue;
					}
					}
					//
					{
					const idVec3& v1 = grid2->vertexes[ l + offset2 ].xyz;
					const idVec3& v2 = grid2->vertexes[ l + 1 + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) < .01 &&
						 idMath::Fabs( v1.y - v2.y ) < .01 &&
						 idMath::Fabs( v1.z - v2.z ) < .01 ) {
						continue;
					}
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					int row;
					if ( m ) {
						row = grid2->height - 1;
					} else {
						row = 0;
					}
					R_GridInsertColumn( grid2, l + 1, row,
						grid1->vertexes[ k + 1 + offset1 ].xyz, grid1->widthLodError[ k + 1 ] );
					return true;
				}
			}
			for ( int m = 0; m < 2; m++ ) {
				if ( grid2->height >= MAX_GRID_SIZE ) {
					break;
				}
				int offset2;
				if ( m ) {
					offset2 = grid2->width - 1;
				} else {
					offset2 = 0;
				}
				for ( int l = 0; l < grid2->height - 1; l++ ) {
					{
					const idVec3& v1 = grid1->vertexes[ k + offset1 ].xyz;
					const idVec3& v2 = grid2->vertexes[ grid2->width * l + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.y - v2.y ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.z - v2.z ) > .1 ) {
						continue;
					}
					}

					{
					const idVec3& v1 = grid1->vertexes[ k + 2 + offset1 ].xyz;
					const idVec3& v2 = grid2->vertexes[ grid2->width * ( l + 1 ) + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.y - v2.y ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.z - v2.z ) > .1 ) {
						continue;
					}
					}
					{
					const idVec3& v1 = grid2->vertexes[ grid2->width * l + offset2 ].xyz;
					const idVec3& v2 = grid2->vertexes[ grid2->width * ( l + 1 ) + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) < .01 &&
						 idMath::Fabs( v1.y - v2.y ) < .01 &&
						 idMath::Fabs( v1.z - v2.z ) < .01 ) {
						continue;
					}
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					int column;
					if ( m ) {
						column = grid2->width - 1;
					} else {
						column = 0;
					}
					R_GridInsertRow( grid2, l + 1, column,
						grid1->vertexes[ k + 1 + offset1 ].xyz, grid1->widthLodError[ k + 1 ] );
					return true;
				}
			}
		}
	}
	for ( int n = 0; n < 2; n++ ) {
		int offset1;
		if ( n ) {
			offset1 = grid1->width - 1;
		} else {
			offset1 = 0;
		}
		if ( R_MergedHeightPoints( grid1, offset1 ) ) {
			continue;
		}
		for ( int k = 0; k < grid1->height - 2; k += 2 ) {
			for ( int m = 0; m < 2; m++ ) {
				if ( grid2->width >= MAX_GRID_SIZE ) {
					break;
				}
				int offset2;
				if ( m ) {
					offset2 = ( grid2->height - 1 ) * grid2->width;
				} else {
					offset2 = 0;
				}
				for ( int l = 0; l < grid2->width - 1; l++ ) {
					{
					const idVec3& v1 = grid1->vertexes[ grid1->width * k + offset1 ].xyz;
					const idVec3& v2 = grid2->vertexes[ l + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.y - v2.y ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.z - v2.z ) > .1 ) {
						continue;
					}
					}

					{
					const idVec3& v1 = grid1->vertexes[ grid1->width * ( k + 2 ) + offset1 ].xyz;
					const idVec3& v2 = grid2->vertexes[ l + 1 + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.y - v2.y ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.z - v2.z ) > .1 ) {
						continue;
					}
					}
					{
					const idVec3& v1 = grid2->vertexes[ l + offset2 ].xyz;
					const idVec3& v2 = grid2->vertexes[ ( l + 1 ) + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) < .01 &&
						 idMath::Fabs( v1.y - v2.y ) < .01 &&
						 idMath::Fabs( v1.z - v2.z ) < .01 ) {
						continue;
					}
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					int row;
					if ( m ) {
						row = grid2->height - 1;
					} else {
						row = 0;
					}
					R_GridInsertColumn( grid2, l + 1, row,
						grid1->vertexes[ grid1->width * ( k + 1 ) + offset1 ].xyz, grid1->heightLodError[ k + 1 ] );
					return true;
				}
			}
			for ( int m = 0; m < 2; m++ ) {
				if ( grid2->height >= MAX_GRID_SIZE ) {
					break;
				}
				int offset2;
				if ( m ) {
					offset2 = grid2->width - 1;
				} else {
					offset2 = 0;
				}
				for ( int l = 0; l < grid2->height - 1; l++ ) {
					{
					const idVec3& v1 = grid1->vertexes[ grid1->width * k + offset1 ].xyz;
					const idVec3& v2 = grid2->vertexes[ grid2->width * l + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.y - v2.y ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.z - v2.z ) > .1 ) {
						continue;
					}
					}

					{
					const idVec3& v1 = grid1->vertexes[ grid1->width * ( k + 2 ) + offset1 ].xyz;
					const idVec3& v2 = grid2->vertexes[ grid2->width * ( l + 1 ) + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.y - v2.y ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.z - v2.z ) > .1 ) {
						continue;
					}
					}
					{
					const idVec3& v1 = grid2->vertexes[ grid2->width * l + offset2 ].xyz;
					const idVec3& v2 = grid2->vertexes[ grid2->width * ( l + 1 ) + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) < .01 &&
						 idMath::Fabs( v1.y - v2.y ) < .01 &&
						 idMath::Fabs( v1.z - v2.z ) < .01 ) {
						continue;
					}
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					int column;
					if ( m ) {
						column = grid2->width - 1;
					} else {
						column = 0;
					}
					R_GridInsertRow( grid2, l + 1, column,
						grid1->vertexes[ grid1->width * ( k + 1 ) + offset1 ].xyz, grid1->heightLodError[ k + 1 ] );
					return true;
				}
			}
		}
	}
	for ( int n = 0; n < 2; n++ ) {
		int offset1;
		if ( n ) {
			offset1 = ( grid1->height - 1 ) * grid1->width;
		} else {
			offset1 = 0;
		}
		if ( R_MergedWidthPoints( grid1, offset1 ) ) {
			continue;
		}
		for ( int k = grid1->width - 1; k > 1; k -= 2 ) {
			for ( int m = 0; m < 2; m++ ) {
				if ( grid2->width >= MAX_GRID_SIZE ) {
					break;
				}
				int offset2;
				if ( m ) {
					offset2 = ( grid2->height - 1 ) * grid2->width;
				} else {
					offset2 = 0;
				}
				for ( int l = 0; l < grid2->width - 1; l++ ) {
					{
					const idVec3& v1 = grid1->vertexes[ k + offset1 ].xyz;
					const idVec3& v2 = grid2->vertexes[ l + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.y - v2.y ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.z - v2.z ) > .1 ) {
						continue;
					}
					}

					{
					const idVec3& v1 = grid1->vertexes[ k - 2 + offset1 ].xyz;
					const idVec3& v2 = grid2->vertexes[ l + 1 + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.y - v2.y ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.z - v2.z ) > .1 ) {
						continue;
					}
					}
					{
					const idVec3& v1 = grid2->vertexes[ l + offset2 ].xyz;
					const idVec3& v2 = grid2->vertexes[ ( l + 1 ) + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) < .01 &&
						 idMath::Fabs( v1.y - v2.y ) < .01 &&
						 idMath::Fabs( v1.z - v2.z ) < .01 ) {
						continue;
					}
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					int row;
					if ( m ) {
						row = grid2->height - 1;
					} else {
						row = 0;
					}
					R_GridInsertColumn( grid2, l + 1, row,
						grid1->vertexes[ k - 1 + offset1 ].xyz, grid1->widthLodError[ k + 1 ] );
					return true;
				}
			}
			for ( int m = 0; m < 2; m++ ) {
				if ( grid2->height >= MAX_GRID_SIZE ) {
					break;
				}
				int offset2;
				if ( m ) {
					offset2 = grid2->width - 1;
				} else {
					offset2 = 0;
				}
				for ( int l = 0; l < grid2->height - 1; l++ ) {
					{
					const idVec3& v1 = grid1->vertexes[ k + offset1 ].xyz;
					const idVec3& v2 = grid2->vertexes[ grid2->width * l + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.y - v2.y ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.z - v2.z ) > .1 ) {
						continue;
					}
					}

					{
					const idVec3& v1 = grid1->vertexes[ k - 2 + offset1 ].xyz;
					const idVec3& v2 = grid2->vertexes[ grid2->width * ( l + 1 ) + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.y - v2.y ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.z - v2.z ) > .1 ) {
						continue;
					}
					}
					{
					const idVec3& v1 = grid2->vertexes[ grid2->width * l + offset2 ].xyz;
					const idVec3& v2 = grid2->vertexes[ grid2->width * ( l + 1 ) + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) < .01 &&
						 idMath::Fabs( v1.y - v2.y ) < .01 &&
						 idMath::Fabs( v1.z - v2.z ) < .01 ) {
						continue;
					}
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					int column;
					if ( m ) {
						column = grid2->width - 1;
					} else {
						column = 0;
					}
					if ( !R_GridInsertRow( grid2, l + 1, column,
						grid1->vertexes[ k - 1 + offset1 ].xyz, grid1->widthLodError[ k + 1 ] ) ) {
						break;
					}
					return true;
				}
			}
		}
	}
	for ( int n = 0; n < 2; n++ ) {
		int offset1;
		if ( n ) {
			offset1 = grid1->width - 1;
		} else {
			offset1 = 0;
		}
		if ( R_MergedHeightPoints( grid1, offset1 ) ) {
			continue;
		}
		for ( int k = grid1->height - 1; k > 1; k -= 2 ) {
			for ( int m = 0; m < 2; m++ ) {
				if ( grid2->width >= MAX_GRID_SIZE ) {
					break;
				}
				int offset2;
				if ( m ) {
					offset2 = ( grid2->height - 1 ) * grid2->width;
				} else {
					offset2 = 0;
				}
				for ( int l = 0; l < grid2->width - 1; l++ ) {
					{
					const idVec3& v1 = grid1->vertexes[ grid1->width * k + offset1 ].xyz;
					const idVec3& v2 = grid2->vertexes[ l + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.y - v2.y ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.z - v2.z ) > .1 ) {
						continue;
					}
					}

					{
					const idVec3& v1 = grid1->vertexes[ grid1->width * ( k - 2 ) + offset1 ].xyz;
					const idVec3& v2 = grid2->vertexes[ l + 1 + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.y - v2.y ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.z - v2.z ) > .1 ) {
						continue;
					}
					}
					{
					const idVec3& v1 = grid2->vertexes[ l + offset2 ].xyz;
					const idVec3& v2 = grid2->vertexes[ ( l + 1 ) + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) < .01 &&
						 idMath::Fabs( v1.y - v2.y ) < .01 &&
						 idMath::Fabs( v1.z - v2.z ) < .01 ) {
						continue;
					}
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert column into grid2 right after after column l
					int row;
					if ( m ) {
						row = grid2->height - 1;
					} else {
						row = 0;
					}
					R_GridInsertColumn( grid2, l + 1, row,
						grid1->vertexes[ grid1->width * ( k - 1 ) + offset1 ].xyz, grid1->heightLodError[ k + 1 ] );
					return true;
				}
			}
			for ( int m = 0; m < 2; m++ ) {
				if ( grid2->height >= MAX_GRID_SIZE ) {
					break;
				}
				int offset2;
				if ( m ) {
					offset2 = grid2->width - 1;
				} else {
					offset2 = 0;
				}
				for ( int l = 0; l < grid2->height - 1; l++ ) {
					{
					const idVec3& v1 = grid1->vertexes[ grid1->width * k + offset1 ].xyz;
					const idVec3& v2 = grid2->vertexes[ grid2->width * l + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.y - v2.y ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.z - v2.z ) > .1 ) {
						continue;
					}
					}

					{
					const idVec3& v1 = grid1->vertexes[ grid1->width * ( k - 2 ) + offset1 ].xyz;
					const idVec3& v2 = grid2->vertexes[ grid2->width * ( l + 1 ) + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.y - v2.y ) > .1 ) {
						continue;
					}
					if ( idMath::Fabs( v1.z - v2.z ) > .1 ) {
						continue;
					}
					}
					{
					const idVec3& v1 = grid2->vertexes[ grid2->width * l + offset2 ].xyz;
					const idVec3& v2 = grid2->vertexes[ grid2->width * ( l + 1 ) + offset2 ].xyz;
					if ( idMath::Fabs( v1.x - v2.x ) < .01 &&
						 idMath::Fabs( v1.y - v2.y ) < .01 &&
						 idMath::Fabs( v1.z - v2.z ) < .01 ) {
						continue;
					}
					}
					//
					//ri.Printf( PRINT_ALL, "found highest LoD crack between two patches\n" );
					// insert row into grid2 right after after row l
					int column;
					if ( m ) {
						column = grid2->width - 1;
					} else {
						column = 0;
					}
					R_GridInsertRow( grid2, l + 1, column,
						grid1->vertexes[ grid1->width * ( k - 1 ) + offset1 ].xyz, grid1->heightLodError[ k + 1 ] );
					return true;
				}
			}
		}
	}
	return false;
}

//	This function will try to stitch patches in the same LoD group together
// for the highest LoD.
//
//	Only single missing vertice cracks will be fixed.
//
//	Vertices will be joined at the patch side a crack is first found, at the
// other side of the patch (on the same row or column) the vertices will not
// be joined and cracks might still appear at that side.
static int R_TryStitchingPatch( idSurfaceGrid* grid1 ) {
	int numstitches = 0;
	for ( int j = 0; j < s_worldData.numsurfaces; j++ ) {
		// if this surface is not a grid
		if ( !s_worldData.surfaces[ j ]->IsGrid() ) {
			continue;
		}
		idSurfaceGrid* grid2 = ( idSurfaceGrid* )s_worldData.surfaces[ j ];
		// grids in the same LOD group should have the exact same lod radius
		if ( grid1->lodRadius != grid2->lodRadius ) {
			continue;
		}
		// grids in the same LOD group should have the exact same lod origin
		if ( grid1->lodOrigin != grid2->lodOrigin ) {
			continue;
		}
		while ( R_StitchPatches( grid1, grid2 ) ) {
			numstitches++;
		}
	}
	return numstitches;
}

static void R_StitchAllPatches() {
	int numstitches = 0;
	bool stitched;
	do {
		stitched = false;
		for ( int i = 0; i < s_worldData.numsurfaces; i++ ) {
			// if this surface is not a grid
			if ( !s_worldData.surfaces[ i ]->IsGrid() ) {
				continue;
			}
			idSurfaceGrid* grid1 = ( idSurfaceGrid* )s_worldData.surfaces[ i ];
			if ( grid1->lodStitched ) {
				continue;
			}
			grid1->lodStitched = true;
			stitched = true;
			//
			numstitches += R_TryStitchingPatch( grid1 );
		}
	} while ( stitched );
	common->Printf( "stitched %d LoD cracks\n", numstitches );
}

static void R_LoadSurfaces( bsp_lump_t* surfs, bsp_lump_t* verts, bsp_lump_t* indexLump ) {
	bsp46_dsurface_t* in = ( bsp46_dsurface_t* )( fileBase + surfs->fileofs );
	if ( surfs->filelen % sizeof ( *in ) ) {
		common->Error( "LoadMap: funny lump size in %s", s_worldData.name );
	}
	int count = surfs->filelen / sizeof ( *in );

	bsp46_drawVert_t* dv = ( bsp46_drawVert_t* )( fileBase + verts->fileofs );
	if ( verts->filelen % sizeof ( *dv ) ) {
		common->Error( "LoadMap: funny lump size in %s", s_worldData.name );
	}

	int* indexes = ( int* )( fileBase + indexLump->fileofs );
	if ( indexLump->filelen % sizeof ( *indexes ) ) {
		common->Error( "LoadMap: funny lump size in %s", s_worldData.name );
	}

	idWorldSurface** out = new idWorldSurface*[ count ];
	Com_Memset( out, 0, sizeof ( idWorldSurface* ) * count );

	s_worldData.surfaces = out;
	s_worldData.numsurfaces = count;

	int numFaces = 0;
	int numMeshes = 0;
	int numTriSurfs = 0;
	int numFlares = 0;
	int numFoliage = 0;

	for ( int i = 0; i < count; i++, in++, out++ ) {
		switch ( LittleLong( in->surfaceType ) ) {
		case BSP46MST_PATCH:
			*out = ParseMesh( in, dv );
			numMeshes++;
			break;
		case BSP46MST_TRIANGLE_SOUP:
			*out = ParseTriSurf( in, dv, indexes );
			numTriSurfs++;
			break;
		case BSP46MST_PLANAR:
			if ( GGameType & GAME_ET ) {
				// ydnar: faces and triangle surfaces are now homogenous
				*out = ParseTriSurf( in, dv, indexes );
			} else {
				*out = ParseFace( in, dv, indexes );
			}
			numFaces++;
			break;
		case BSP46MST_FLARE:
			*out = ParseFlare( in );
			numFlares++;
			break;
		case BSP47MST_FOLIAGE:	// ydnar
			*out = ParseFoliage( in, dv, indexes );
			numFoliage++;
			break;
		default:
			common->Error( "Bad surfaceType" );
		}
	}

	R_StitchAllPatches();

	R_FixSharedVertexLodError();

	common->Printf( "...loaded %d faces, %i meshes, %i trisurfs, %i flares %i foliage\n",
		numFaces, numMeshes, numTriSurfs, numFlares, numFoliage );
}

static void R_SetParent( mbrush46_node_t* node, mbrush46_node_t* parent ) {
	node->parent = parent;
	if ( node->contents != -1 ) {
		// add node surfaces to bounds
		if ( node->nummarksurfaces > 0 ) {
			// add node surfaces to bounds
			idWorldSurface** mark = node->firstmarksurface;
			int c = node->nummarksurfaces;
			while ( c-- ) {
				if ( !( *mark )->AddToNodeBounds() ) {
					continue;
				}
				AddPointToBounds( ( *mark )->bounds[ 0 ].ToFloatPtr(), node->surfMins, node->surfMaxs );
				AddPointToBounds( ( *mark )->bounds[ 1 ].ToFloatPtr(), node->surfMins, node->surfMaxs );
				mark++;
			}
		}
		return;
	}
	R_SetParent( node->children[ 0 ], node );
	R_SetParent( node->children[ 1 ], node );

	// ydnar: surface bounds
	AddPointToBounds( node->children[ 0 ]->surfMins, node->surfMins, node->surfMaxs );
	AddPointToBounds( node->children[ 0 ]->surfMins, node->surfMins, node->surfMaxs );
	AddPointToBounds( node->children[ 1 ]->surfMins, node->surfMins, node->surfMaxs );
	AddPointToBounds( node->children[ 1 ]->surfMaxs, node->surfMins, node->surfMaxs );
}

static void R_LoadNodesAndLeafs( bsp_lump_t* nodeLump, bsp_lump_t* leafLump ) {
	bsp46_dnode_t* in = ( bsp46_dnode_t* )( fileBase + nodeLump->fileofs );
	if ( nodeLump->filelen % sizeof ( bsp46_dnode_t ) ||
		 leafLump->filelen % sizeof ( bsp46_dleaf_t ) ) {
		common->Error( "LoadMap: funny lump size in %s", s_worldData.name );
	}
	int numNodes = nodeLump->filelen / sizeof ( bsp46_dnode_t );
	int numLeafs = leafLump->filelen / sizeof ( bsp46_dleaf_t );

	mbrush46_node_t* out = new mbrush46_node_t[ numNodes + numLeafs ];
	Com_Memset( out, 0, sizeof ( mbrush46_node_t ) * ( numNodes + numLeafs ) );

	s_worldData.nodes = out;
	s_worldData.numnodes = numNodes + numLeafs;
	s_worldData.numDecisionNodes = numNodes;

	// ydnar: skybox optimization
	s_worldData.numSkyNodes = 0;
	s_worldData.skyNodes = new mbrush46_node_t*[ WORLD_MAX_SKY_NODES ];

	// load nodes
	for ( int i = 0; i < numNodes; i++, in++, out++ ) {
		for ( int j = 0; j < 3; j++ ) {
			out->mins[ j ] = LittleLong( in->mins[ j ] );
			out->maxs[ j ] = LittleLong( in->maxs[ j ] );
		}

		// ydnar: surface bounds
		VectorCopy( out->mins, out->surfMins );
		VectorCopy( out->maxs, out->surfMaxs );

		int p = LittleLong( in->planeNum );
		out->plane = s_worldData.planes + p;

		out->contents = BRUSH46_CONTENTS_NODE;	// differentiate from leafs

		for ( int j = 0; j < 2; j++ ) {
			p = LittleLong( in->children[ j ] );
			if ( p >= 0 ) {
				out->children[ j ] = s_worldData.nodes + p;
			} else {
				out->children[ j ] = s_worldData.nodes + numNodes + ( -1 - p );
			}
		}
	}

	// load leafs
	bsp46_dleaf_t* inLeaf = ( bsp46_dleaf_t* )( fileBase + leafLump->fileofs );
	for ( int i = 0; i < numLeafs; i++, inLeaf++, out++ ) {
		for ( int j = 0; j < 3; j++ ) {
			out->mins[ j ] = LittleLong( inLeaf->mins[ j ] );
			out->maxs[ j ] = LittleLong( inLeaf->maxs[ j ] );
		}

		// ydnar: surface bounds
		ClearBounds( out->surfMins, out->surfMaxs );

		out->cluster = LittleLong( inLeaf->cluster );
		out->area = LittleLong( inLeaf->area );

		if ( out->cluster >= s_worldData.numClusters ) {
			s_worldData.numClusters = out->cluster + 1;
		}

		out->firstmarksurface = s_worldData.marksurfaces +
								LittleLong( inLeaf->firstLeafSurface );
		out->nummarksurfaces = LittleLong( inLeaf->numLeafSurfaces );
	}

	// chain decendants
	R_SetParent( s_worldData.nodes, NULL );
}

static void R_LoadShaders( bsp_lump_t* l ) {
	bsp46_dshader_t* in = ( bsp46_dshader_t* )( fileBase + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->Error( "LoadMap: funny lump size in %s", s_worldData.name );
	}
	int count = l->filelen / sizeof ( *in );
	bsp46_dshader_t* out = new bsp46_dshader_t[ count ];

	s_worldData.shaders = out;
	s_worldData.numShaders = count;

	Com_Memcpy( out, in, count * sizeof ( *out ) );

	for ( int i = 0; i < count; i++ ) {
		out[ i ].surfaceFlags = LittleLong( out[ i ].surfaceFlags );
		out[ i ].contentFlags = LittleLong( out[ i ].contentFlags );
	}
}

static void R_LoadMarksurfaces( bsp_lump_t* l ) {
	int* in = ( int* )( fileBase + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->Error( "LoadMap: funny lump size in %s", s_worldData.name );
	}
	int count = l->filelen / sizeof ( *in );
	idWorldSurface** out = new idWorldSurface*[ count ];

	s_worldData.marksurfaces = out;
	s_worldData.nummarksurfaces = count;

	for ( int i = 0; i < count; i++ ) {
		int j = LittleLong( in[ i ] );
		out[ i ] = s_worldData.surfaces[ j ];
	}
}

static void R_LoadPlanes( bsp_lump_t* l ) {
	bsp46_dplane_t* in = ( bsp46_dplane_t* )( fileBase + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->Error( "LoadMap: funny lump size in %s", s_worldData.name );
	}
	int count = l->filelen / sizeof ( *in );
	//JL Why * 2?
	cplane_t* out = new cplane_t[ count * 2 ];
	Com_Memset( out, 0, sizeof ( cplane_t ) * count * 2 );

	s_worldData.planes = out;
	s_worldData.numplanes = count;

	for ( int i = 0; i < count; i++, in++, out++ ) {
		for ( int j = 0; j < 3; j++ ) {
			out->normal[ j ] = LittleFloat( in->normal[ j ] );
		}

		out->dist = LittleFloat( in->dist );
		out->type = PlaneTypeForNormal( out->normal );
		SetPlaneSignbits( out );
	}
}

unsigned ColorBytes4( float r, float g, float b, float a ) {
	unsigned i;

	( ( byte* )&i )[ 0 ] = r * 255;
	( ( byte* )&i )[ 1 ] = g * 255;
	( ( byte* )&i )[ 2 ] = b * 255;
	( ( byte* )&i )[ 3 ] = a * 255;

	return i;
}

static void R_LoadFogs( bsp_lump_t* l, bsp_lump_t* brushesLump, bsp_lump_t* sidesLump ) {
	bsp46_dfog_t* fogs = ( bsp46_dfog_t* )( fileBase + l->fileofs );
	if ( l->filelen % sizeof ( *fogs ) ) {
		common->Error( "LoadMap: funny lump size in %s", s_worldData.name );
	}
	int count = l->filelen / sizeof ( *fogs );

	// create fog strucutres for them
	s_worldData.numfogs = count + 1;
	s_worldData.fogs = new mbrush46_fog_t[ s_worldData.numfogs ];
	Com_Memset( s_worldData.fogs, 0, sizeof ( mbrush46_fog_t ) * s_worldData.numfogs );
	mbrush46_fog_t* out = s_worldData.fogs + 1;

	// ydnar: reset global fog
	s_worldData.globalFog = -1;

	if ( !count ) {
		return;
	}

	bsp46_dbrush_t* brushes = ( bsp46_dbrush_t* )( fileBase + brushesLump->fileofs );
	if ( brushesLump->filelen % sizeof ( *brushes ) ) {
		common->Error( "LoadMap: funny lump size in %s", s_worldData.name );
	}
	int brushesCount = brushesLump->filelen / sizeof ( *brushes );

	bsp46_dbrushside_t* sides = ( bsp46_dbrushside_t* )( fileBase + sidesLump->fileofs );
	if ( sidesLump->filelen % sizeof ( *sides ) ) {
		common->Error( "LoadMap: funny lump size in %s", s_worldData.name );
	}
	int sidesCount = sidesLump->filelen / sizeof ( *sides );

	for ( int i = 0; i < count; i++, fogs++, out++ ) {
		out->originalBrushNumber = LittleLong( fogs->brushNum );

		int firstSide = 0;
		// ydnar: global fog has a brush number of -1, and no visible side
		if ( GGameType & GAME_ET && out->originalBrushNumber == -1 ) {
			VectorSet( out->bounds[ 0 ], MIN_WORLD_COORD, MIN_WORLD_COORD, MIN_WORLD_COORD );
			VectorSet( out->bounds[ 1 ], MAX_WORLD_COORD, MAX_WORLD_COORD, MAX_WORLD_COORD );
		} else {
			if ( ( unsigned )out->originalBrushNumber >= ( unsigned )brushesCount ) {
				common->Error( "fog brushNumber out of range" );
			}

			// ydnar: find which bsp submodel the fog volume belongs to
			for ( int j = 0; j < s_worldData.numBModels; j++ ) {
				if ( out->originalBrushNumber >= s_worldData.bmodels[ j ].firstBrush &&
					 out->originalBrushNumber < ( s_worldData.bmodels[ j ].firstBrush + s_worldData.bmodels[ j ].numBrushes ) ) {
					out->modelNum = j;
					break;
				}
			}

			bsp46_dbrush_t* brush = brushes + out->originalBrushNumber;

			firstSide = LittleLong( brush->firstSide );
			if ( ( unsigned )firstSide > ( unsigned )( sidesCount - 6 ) ) {
				common->Error( "fog brush sideNumber out of range" );
			}

			// brushes are always sorted with the axial sides first
			int sideNum = firstSide + 0;
			int planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[ 0 ][ 0 ] = -s_worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 1;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[ 1 ][ 0 ] = s_worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 2;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[ 0 ][ 1 ] = -s_worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 3;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[ 1 ][ 1 ] = s_worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 4;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[ 0 ][ 2 ] = -s_worldData.planes[ planeNum ].dist;

			sideNum = firstSide + 5;
			planeNum = LittleLong( sides[ sideNum ].planeNum );
			out->bounds[ 1 ][ 2 ] = s_worldData.planes[ planeNum ].dist;
		}

		// get information from the shader for fog parameters
		shader_t* shader = R_FindShader( fogs->shader, LIGHTMAP_NONE, true );

		out->parms = shader->fogParms;

		// Arnout: colorInt is now set in the shader so we can modify it
		out->shader = shader;

		out->colorInt = ColorBytes4( shader->fogParms.color[ 0 ] * tr.identityLight,
			shader->fogParms.color[ 1 ] * tr.identityLight,
			shader->fogParms.color[ 2 ] * tr.identityLight, 1.0 );

		float d = shader->fogParms.depthForOpaque < 1 ? 1 : shader->fogParms.depthForOpaque;
		out->tcScale = 1.0f / ( d * 8 );

		// ydnar: global fog sets clearcolor/zfar
		if ( GGameType & GAME_ET && out->originalBrushNumber == -1 ) {
			s_worldData.globalFog = i + 1;
			VectorCopy( shader->fogParms.color, s_worldData.globalOriginalFog );
			s_worldData.globalOriginalFog[ 3 ] =  shader->fogParms.depthForOpaque;
		}

		// set the gradient vector
		int sideNum = LittleLong( fogs->visibleSide );

		if ( sideNum < 0 || sideNum >= sidesCount ) {
			out->hasSurface = false;
		} else {
			out->hasSurface = true;
			int planeNum = LittleLong( sides[ firstSide + sideNum ].planeNum );
			VectorSubtract( oldvec3_origin, s_worldData.planes[ planeNum ].normal, out->surface );
			out->surface[ 3 ] = -s_worldData.planes[ planeNum ].dist;
		}
	}
}

static void R_LoadLightGrid( bsp_lump_t* l ) {
	world_t* w = &s_worldData;

	w->lightGridInverseSize[ 0 ] = 1.0f / w->lightGridSize[ 0 ];
	w->lightGridInverseSize[ 1 ] = 1.0f / w->lightGridSize[ 1 ];
	w->lightGridInverseSize[ 2 ] = 1.0f / w->lightGridSize[ 2 ];

	float* wMins = w->bmodels[ 0 ].bounds[ 0 ];
	float* wMaxs = w->bmodels[ 0 ].bounds[ 1 ];

	vec3_t maxs;
	for ( int i = 0; i < 3; i++ ) {
		w->lightGridOrigin[ i ] = w->lightGridSize[ i ] * ceil( wMins[ i ] / w->lightGridSize[ i ] );
		maxs[ i ] = w->lightGridSize[ i ] * floor( wMaxs[ i ] / w->lightGridSize[ i ] );
		w->lightGridBounds[ i ] = ( maxs[ i ] - w->lightGridOrigin[ i ] ) / w->lightGridSize[ i ] + 1;
	}

	int numGridPoints = w->lightGridBounds[ 0 ] * w->lightGridBounds[ 1 ] * w->lightGridBounds[ 2 ];

	if ( l->filelen != numGridPoints * 8 ) {
		common->Printf( S_COLOR_YELLOW "WARNING: light grid mismatch\n" );
		w->lightGridData = NULL;
		return;
	}

	w->lightGridData = new byte[ l->filelen ];
	Com_Memcpy( w->lightGridData, fileBase + l->fileofs, l->filelen );

	// deal with overbright bits
	for ( int i = 0; i < numGridPoints; i++ ) {
		R_ColorShiftLightingBytes( &w->lightGridData[ i * 8 ], &w->lightGridData[ i * 8 ] );
		R_ColorShiftLightingBytes( &w->lightGridData[ i * 8 + 3 ], &w->lightGridData[ i * 8 + 3 ] );
	}
}

static void R_LoadEntities( bsp_lump_t* l ) {
	world_t* w = &s_worldData;
	w->lightGridSize[ 0 ] = 64;
	w->lightGridSize[ 1 ] = 64;
	w->lightGridSize[ 2 ] = 128;

	const char* p = ( const char* )( fileBase + l->fileofs );

	// store for reference by the cgame
	w->entityString = new char[ l->filelen + 1 ];
	String::Cpy( w->entityString, p );
	w->entityString[ l->filelen ] = 0;
	w->entityParsePoint = w->entityString;

	char* token = String::ParseExt( &p, true );
	if ( !*token || *token != '{' ) {
		return;
	}

	// only parse the world spawn
	while ( 1 ) {
		// parse key
		token = String::ParseExt( &p, true );

		if ( !*token || *token == '}' ) {
			break;
		}
		char keyname[ MAX_TOKEN_CHARS_Q3 ];
		String::NCpyZ( keyname, token, sizeof ( keyname ) );

		// parse value
		token = String::ParseExt( &p, true );

		if ( !*token || *token == '}' ) {
			break;
		}
		char value[ MAX_TOKEN_CHARS_Q3 ];
		String::NCpyZ( value, token, sizeof ( value ) );

		if ( !( GGameType & ( GAME_WolfMP | GAME_ET ) ) ) {
			// check for remapping of shaders for vertex lighting
			const char* keybase = "vertexremapshader";
			if ( !String::NCmp( keyname, keybase, String::Length( keybase ) ) ) {
				char* s = strchr( value, ';' );
				if ( !s ) {
					common->Printf( S_COLOR_YELLOW "WARNING: no semi colon in vertexshaderremap '%s'\n", value );
					break;
				}
				*s++ = 0;
				if ( r_vertexLight->integer ) {
					R_RemapShader( value, s, "0" );
				}
				continue;
			}
		}

		// check for remapping of shaders
		const char* keybase = "remapshader";
		if ( !String::NCmp( keyname, keybase, String::Length( keybase ) ) ) {
			char* s = strchr( value, ';' );
			if ( !s ) {
				common->Printf( S_COLOR_YELLOW "WARNING: no semi colon in shaderremap '%s'\n", value );
				break;
			}
			*s++ = 0;
			R_RemapShader( value, s, "0" );
			continue;
		}

		// check for a different grid size
		if ( !String::ICmp( keyname, "gridsize" ) ) {
			sscanf( value, "%f %f %f", &w->lightGridSize[ 0 ], &w->lightGridSize[ 1 ], &w->lightGridSize[ 2 ] );
			continue;
		}
	}
}

static void R_LoadSubmodels( bsp_lump_t* l ) {
	bsp46_dmodel_t* in = ( bsp46_dmodel_t* )( fileBase + l->fileofs );
	if ( l->filelen % sizeof ( *in ) ) {
		common->Error( "LoadMap: funny lump size in %s", s_worldData.name );
	}
	int count = l->filelen / sizeof ( *in );

	mbrush46_model_t* out = new mbrush46_model_t[ count ];
	s_worldData.numBModels = count;
	s_worldData.bmodels = out;

	for ( int i = 0; i < count; i++, in++, out++ ) {
		idRenderModel* model = new idRenderModelBSP46();
		R_AddModel( model );

		assert( model != NULL );			// this should never happen

		model->type = MOD_BRUSH46;
		model->q3_bmodel = out;
		String::Sprintf( model->name, sizeof ( model->name ), "*%d", i );

		for ( int j = 0; j < 3; j++ ) {
			out->bounds[ 0 ][ j ] = LittleFloat( in->mins[ j ] );
			out->bounds[ 1 ][ j ] = LittleFloat( in->maxs[ j ] );
		}

		out->firstSurface = s_worldData.surfaces + LittleLong( in->firstSurface );
		out->numSurfaces = LittleLong( in->numSurfaces );

		// ydnar: for attaching fog brushes to models
		out->firstBrush = LittleLong( in->firstBrush );
		out->numBrushes = LittleLong( in->numBrushes );

		// ydnar: allocate decal memory
		int j = ( i == 0 ? MAX_WORLD_DECALS : MAX_ENTITY_DECALS );
		out->decals = new mbrush46_decal_t[ j ];
		memset( out->decals, 0, j * sizeof ( *out->decals ) );
	}
}

static void ExecUpdateScreen() {
	if ( GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) ) {
		Cbuf_ExecuteText( EXEC_NOW, "updatescreen\n" );
	}
}

void R_LoadBrush46Model( void* buffer ) {
	bsp46_dheader_t* header = ( bsp46_dheader_t* )buffer;
	fileBase = ( byte* )buffer;

	int version = LittleLong( header->version );
	if ( version != BSP46_VERSION && version != BSP47_VERSION ) {
		common->Error( "RE_LoadWorldMap: %s has wrong version number (%i should be %i)",
			s_worldData.name, version, BSP46_VERSION );
	}

	// swap all the lumps
	for ( int i = 0; i < ( int )sizeof ( bsp46_dheader_t ) / 4; i++ ) {
		( ( int* )header )[ i ] = LittleLong( ( ( int* )header )[ i ] );
	}

	// load into heap
	ExecUpdateScreen();
	R_LoadShaders( &header->lumps[ BSP46LUMP_SHADERS ] );
	ExecUpdateScreen();
	R_LoadLightmaps( &header->lumps[ BSP46LUMP_LIGHTMAPS ] );
	ExecUpdateScreen();
	R_LoadPlanes( &header->lumps[ BSP46LUMP_PLANES ] );
	ExecUpdateScreen();
	R_LoadSurfaces( &header->lumps[ BSP46LUMP_SURFACES ], &header->lumps[ BSP46LUMP_DRAWVERTS ], &header->lumps[ BSP46LUMP_DRAWINDEXES ] );
	ExecUpdateScreen();
	R_LoadMarksurfaces( &header->lumps[ BSP46LUMP_LEAFSURFACES ] );
	ExecUpdateScreen();
	R_LoadNodesAndLeafs( &header->lumps[ BSP46LUMP_NODES ], &header->lumps[ BSP46LUMP_LEAFS ] );
	ExecUpdateScreen();
	R_LoadSubmodels( &header->lumps[ BSP46LUMP_MODELS ] );
	//	moved fog lump loading here, so fogs can be tagged with a model num
	ExecUpdateScreen();
	R_LoadFogs( &header->lumps[ BSP46LUMP_FOGS ], &header->lumps[ BSP46LUMP_BRUSHES ], &header->lumps[ BSP46LUMP_BRUSHSIDES ] );
	ExecUpdateScreen();
	R_LoadVisibility( &header->lumps[ BSP46LUMP_VISIBILITY ] );
	ExecUpdateScreen();
	R_LoadEntities( &header->lumps[ BSP46LUMP_ENTITIES ] );
	ExecUpdateScreen();
	R_LoadLightGrid( &header->lumps[ BSP46LUMP_LIGHTGRID ] );
	ExecUpdateScreen();
}

void R_FreeBsp46( world_t* mod ) {
	delete[] mod->novis;
	if ( mod->vis ) {
		delete[] mod->vis;
	}
	for ( int i = 0; i < mod->numsurfaces; i++ ) {
		delete mod->surfaces[ i ];
	}
	delete[] mod->surfaces;
	delete[] mod->nodes;
	delete[] mod->skyNodes;
	delete[] mod->shaders;
	delete[] mod->marksurfaces;
	delete[] mod->planes;
	delete[] mod->fogs;
	delete[] mod->lightGridData;
	delete[] mod->entityString;
	delete[] mod->bmodels;
}

void R_FreeBsp46Model( idRenderModel* mod ) {
	delete[] mod->q3_bmodel->decals;
}

const byte* R_ClusterPVS( int cluster ) {
	if ( !tr.world || !tr.world->vis || cluster < 0 || cluster >= tr.world->numClusters ) {
		return tr.world->novis;
	}

	return tr.world->vis + cluster * tr.world->clusterBytes;
}

mbrush46_node_t* R_PointInLeaf( const vec3_t p ) {
	if ( !tr.world ) {
		common->Error( "R_PointInLeaf: bad model" );
	}

	mbrush46_node_t* node = tr.world->nodes;
	while ( 1 ) {
		if ( node->contents != -1 ) {
			break;
		}
		cplane_t* plane = node->plane;
		float d = DotProduct( p, plane->normal ) - plane->dist;
		if ( d > 0 ) {
			node = node->children[ 0 ];
		} else {
			node = node->children[ 1 ];
		}
	}

	return node;
}
