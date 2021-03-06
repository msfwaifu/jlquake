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

#include "RenderModelMD4.h"
#include "../../../common/endian.h"
#include "../../../common/Common.h"
#include "../../../common/strings.h"

#define LL( x ) x = LittleLong( x )

idRenderModelMD4::idRenderModelMD4() {
}

idRenderModelMD4::~idRenderModelMD4() {
}

bool idRenderModelMD4::Load( idList<byte>& buffer, idSkinTranslation* skinTranslation ) {
	md4Header_t* pinmodel = ( md4Header_t* )buffer.Ptr();

	int version = LittleLong( pinmodel->version );
	if ( version != MD4_VERSION ) {
		common->Printf( S_COLOR_YELLOW "R_LoadMD4: %s has wrong version (%i should be %i)\n",
			name, version, MD4_VERSION );
		return false;
	}

	type = MOD_MD4;
	int size = LittleLong( pinmodel->ofsEnd );
	q3_dataSize += size;
	md4Header_t* md4 = q3_md4 = ( md4Header_t* )Mem_Alloc( size );

	Com_Memcpy( md4, buffer.Ptr(), LittleLong( pinmodel->ofsEnd ) );

	LL( md4->ident );
	LL( md4->version );
	LL( md4->numFrames );
	LL( md4->numBones );
	LL( md4->numLODs );
	LL( md4->ofsFrames );
	LL( md4->ofsLODs );
	LL( md4->ofsEnd );

	if ( md4->numFrames < 1 ) {
		common->Printf( S_COLOR_YELLOW "R_LoadMD4: %s has no frames\n", name );
		return false;
	}

	// we don't need to swap tags in the renderer, they aren't used

	// swap all the frames
	int frameSize = ( qintptr )( &( ( md4Frame_t* )0 )->bones[ md4->numBones ] );
	for ( int i = 0; i < md4->numFrames; i++ ) {
		md4Frame_t* frame = ( md4Frame_t* )( ( byte* )md4 + md4->ofsFrames + i * frameSize );
		frame->radius = LittleFloat( frame->radius );
		for ( int j = 0; j < 3; j++ ) {
			frame->bounds[ 0 ][ j ] = LittleFloat( frame->bounds[ 0 ][ j ] );
			frame->bounds[ 1 ][ j ] = LittleFloat( frame->bounds[ 1 ][ j ] );
			frame->localOrigin[ j ] = LittleFloat( frame->localOrigin[ j ] );
		}
		for ( int j = 0; j < md4->numBones * ( int )sizeof ( md4Bone_t ) / 4; j++ ) {
			( ( float* )frame->bones )[ j ] = LittleFloat( ( ( float* )frame->bones )[ j ] );
		}
	}

	// swap all the LOD's
	q3_md4Lods = new mmd4Lod_t[ md4->numLODs ];
	md4LOD_t* lod = ( md4LOD_t* )( ( byte* )md4 + md4->ofsLODs );
	for ( int lodindex = 0; lodindex < md4->numLODs; lodindex++ ) {
		// swap all the surfaces
		q3_md4Lods[ lodindex ].numSurfaces = lod->numSurfaces;
		q3_md4Lods[ lodindex ].surfaces = new idSurfaceMD4[ lod->numSurfaces ];
		md4Surface_t* surf = ( md4Surface_t* )( ( byte* )lod + lod->ofsSurfaces );
		for ( int i = 0; i < lod->numSurfaces; i++ ) {
			q3_md4Lods[ lodindex ].surfaces[ i ].SetMd4Data( surf );
			LL( surf->ident );
			LL( surf->numTriangles );
			LL( surf->ofsTriangles );
			LL( surf->numVerts );
			LL( surf->ofsVerts );
			LL( surf->ofsEnd );

			if ( surf->numVerts > SHADER_MAX_VERTEXES ) {
				common->Error( "R_LoadMD3: %s has more than %i verts on a surface (%i)",
					name, SHADER_MAX_VERTEXES, surf->numVerts );
			}
			if ( surf->numTriangles * 3 > SHADER_MAX_INDEXES ) {
				common->Error( "R_LoadMD3: %s has more than %i triangles on a surface (%i)",
					name, SHADER_MAX_INDEXES / 3, surf->numTriangles );
			}

			// lowercase the surface name so skin compares are faster
			String::ToLower( surf->name );

			// register the shaders
			shader_t* sh = R_FindShader( surf->shader, LIGHTMAP_NONE, true );
			if ( sh->defaultShader ) {
				surf->shaderIndex = 0;
			} else {
				surf->shaderIndex = sh->index;
			}

			// swap all the triangles
			md4Triangle_t* tri = ( md4Triangle_t* )( ( byte* )surf + surf->ofsTriangles );
			for ( int j = 0; j < surf->numTriangles; j++, tri++ ) {
				LL( tri->indexes[ 0 ] );
				LL( tri->indexes[ 1 ] );
				LL( tri->indexes[ 2 ] );
			}

			// swap all the vertexes
			// FIXME
			// This makes TFC's skeletons work.  Shouldn't be necessary anymore, but left
			// in for reference.
			//v = (md4Vertex_t *) ( (byte *)surf + surf->ofsVerts + 12);
			md4Vertex_t* v = ( md4Vertex_t* )( ( byte* )surf + surf->ofsVerts );
			for ( int j = 0; j < surf->numVerts; j++ ) {
				v->normal[ 0 ] = LittleFloat( v->normal[ 0 ] );
				v->normal[ 1 ] = LittleFloat( v->normal[ 1 ] );
				v->normal[ 2 ] = LittleFloat( v->normal[ 2 ] );

				v->texCoords[ 0 ] = LittleFloat( v->texCoords[ 0 ] );
				v->texCoords[ 1 ] = LittleFloat( v->texCoords[ 1 ] );

				v->numWeights = LittleLong( v->numWeights );

				for ( int k = 0; k < v->numWeights; k++ ) {
					v->weights[ k ].boneIndex = LittleLong( v->weights[ k ].boneIndex );
					v->weights[ k ].boneWeight = LittleFloat( v->weights[ k ].boneWeight );
					v->weights[ k ].offset[ 0 ] = LittleFloat( v->weights[ k ].offset[ 0 ] );
					v->weights[ k ].offset[ 1 ] = LittleFloat( v->weights[ k ].offset[ 1 ] );
					v->weights[ k ].offset[ 2 ] = LittleFloat( v->weights[ k ].offset[ 2 ] );
				}
				// FIXME
				// This makes TFC's skeletons work.  Shouldn't be necessary anymore, but left
				// in for reference.
				//v = (md4Vertex_t *)( ( byte * )&v->weights[v->numWeights] + 12 );
				v = ( md4Vertex_t* )( ( byte* )&v->weights[ v->numWeights ] );
			}

			// find the next surface
			surf = ( md4Surface_t* )( ( byte* )surf + surf->ofsEnd );
		}

		// find the next LOD
		lod = ( md4LOD_t* )( ( byte* )lod + lod->ofsEnd );
	}

	return true;
}
