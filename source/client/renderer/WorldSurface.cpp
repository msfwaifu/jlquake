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

#include "WorldSurface.h"
#include "decals.h"
#include "cvars.h"
#include "backend.h"
#include "../../common/content_types.h"
#include "../../common/common_defs.h"

idWorldSurface::~idWorldSurface() {
	delete[] vertexes;
	delete[] indexes;
}

bool idWorldSurface::IsGrid() const {
	return false;
}

void idWorldSurface::ProjectDecal( decalProjector_t* dp, mbrush46_model_t* bmodel ) const {
}

bool idWorldSurface::ClipDecal( struct decalProjector_t* dp ) const {
	//	early outs
	if ( dp->shader == NULL ) {
		return false;
	}
	if ( ( shader->surfaceFlags & ( BSP46SURF_NOIMPACT | BSP46SURF_NOMARKS ) ) ||
		 ( shader->contentFlags & BSP46CONTENTS_FOG ) ) {
		return false;
	}

	//	add to counts
	tr.pc.c_decalTestSurfaces++;

	//	test bounding sphere
	vec3_t old;
	boundingSphere.GetOrigin().ToOldVec3( old );
	if ( !R_TestDecalBoundingSphere( dp, old, ( boundingSphere.GetRadius() * boundingSphere.GetRadius() ) ) ) {
		return false;
	}

	//	planar surface
	if ( plane.Normal()[ 0 ] || plane.Normal()[ 1 ] || plane.Normal()[ 2 ] ) {
		//	backface check
		vec3_t old;
		plane.Normal().ToOldVec3( old );
		float d = DotProduct( dp->planes[ 0 ], old );
		if ( d < -0.0001 ) {
			return false;
		}

		//	plane-sphere check
		d = DotProduct( dp->center, old ) - plane.Dist();
		if ( fabs( d ) >= dp->radius ) {
			return false;
		}
	}

	//	add to counts
	tr.pc.c_decalClipSurfaces++;
	return true;
}

bool idWorldSurface::CheckAddMarks( const vec3_t mins, const vec3_t maxs, const vec3_t dir ) const {
	return false;
}

void idWorldSurface::MarkFragments( const vec3_t projectionDir,
	int numPlanes, const vec3_t* normals, const float* dists,
	int maxPoints, vec3_t pointBuffer,
	int maxFragments, markFragment_t* fragmentBuffer,
	int* returnedPoints, int* returnedFragments,
	const vec3_t mins, const vec3_t maxs ) const {
}

void idWorldSurface::MarkFragmentsWolf( const vec3_t projectionDir,
	int numPlanes, const vec3_t* normals, const float* dists,
	int maxPoints, vec3_t pointBuffer,
	int maxFragments, markFragment_t* fragmentBuffer,
	int* returnedPoints, int* returnedFragments,
	const vec3_t mins, const vec3_t maxs,
	bool oldMapping, const vec3_t center, float radius, const vec3_t bestnormal, int orientation, int numPoints ) const {
}

bool idWorldSurface::AddToNodeBounds() const {
	return true;
}

//	Tries to back face cull surfaces before they are lighted or added to the
// sorting list.
//
//	This will also allow mirrors on both sides of a model without recursion.
bool idWorldSurface::Cull( shader_t* shader, int* frontFace ) {
	// force to non-front facing
	*frontFace = 0;

	if ( r_nocull->integer ) {
		return false;
	}

	if ( GGameType & GAME_ET ) {
		return DoCullET( shader, frontFace );
	}

	return DoCull( shader );
}

bool idWorldSurface::DoCull( shader_t* shader ) const {
	return false;
}

bool idWorldSurface::DoCullET( shader_t* shader, int* frontFace ) const {
	// ydnar: made surface culling generic, inline with q3map2 surface classification

	// plane cull
	if ( plane.Normal() != vec3_origin && r_facePlaneCull->integer ) {
		vec3_t old;
		plane.Normal().ToOldVec3( old );
		float d = DotProduct( tr.orient.viewOrigin, old ) - plane.Dist();
		if ( d > 0.0f ) {
			*frontFace = 1;
		}

		// don't cull exactly on the plane, because there are levels of rounding
		// through the BSP, ICD, and hardware that may cause pixel gaps if an
		// epsilon isn't allowed here
		if ( shader->cullType == CT_FRONT_SIDED ) {
			if ( d < -8.0f ) {
				tr.pc.c_plane_cull_out++;
				return true;
			}
		} else if ( shader->cullType == CT_BACK_SIDED ) {
			if ( d > 8.0f ) {
				tr.pc.c_plane_cull_out++;
				return true;
			}
		}

		tr.pc.c_plane_cull_in++;
	}

	// try sphere cull
	int cull;
	if ( tr.currentEntityNum != REF_ENTITYNUM_WORLD ) {
		cull = R_CullLocalSphere( boundingSphere );
	} else {
		cull = R_CullSphere( boundingSphere );
	}
	if ( cull == CULL_OUT ) {
		tr.pc.c_sphere_cull_out++;
		return true;
	}

	tr.pc.c_sphere_cull_in++;

	// must be visible
	return false;
}

//	The given surface is going to be drawn, and it touches a leaf that is
// touched by one or more dlights, so try to throw out more dlights if possible.
int idWorldSurface::MarkDynamicLights( int dlightBits ) {
	if ( GGameType & GAME_ET ) {
		return DoMarkDynamicLightsET( dlightBits );
	}

	dlightBits = DoMarkDynamicLights( dlightBits );

	if ( dlightBits ) {
		tr.pc.c_dlightSurfaces++;
	}

	return dlightBits;
}

int idWorldSurface::DoMarkDynamicLights( int dlightBits ) {
	return 0;
}

// ydnar: made this use generic surface
int idWorldSurface::DoMarkDynamicLightsET( int dlightBits ) {
	// ydnar: made surface dlighting generic, inline with q3map2 surface classification

	// try to cull out dlights
	for ( int i = 0; i < tr.refdef.num_dlights; i++ ) {
		if ( !( dlightBits & ( 1 << i ) ) ) {
			continue;
		}

		// junior dlights don't affect world surfaces
		if ( tr.refdef.dlights[ i ].flags & REF_JUNIOR_DLIGHT ) {
			dlightBits &= ~( 1 << i );
			continue;
		}

		// lightning dlights affect all surfaces
		if ( tr.refdef.dlights[ i ].flags & REF_DIRECTED_DLIGHT ) {
			continue;
		}

		// test surface bounding sphere against dlight bounding sphere
		vec3_t origin;
		VectorCopy( tr.refdef.dlights[ i ].transformed, origin );
		float radius = tr.refdef.dlights[ i ].radius;

		if ( ( boundingSphere.GetOrigin().x + boundingSphere.GetRadius() ) < ( origin[ 0 ] - radius ) ||
			 ( boundingSphere.GetOrigin().x - boundingSphere.GetRadius() ) > ( origin[ 0 ] + radius ) ||
			 ( boundingSphere.GetOrigin().y + boundingSphere.GetRadius() ) < ( origin[ 1 ] - radius ) ||
			 ( boundingSphere.GetOrigin().y - boundingSphere.GetRadius() ) > ( origin[ 1 ] + radius ) ||
			 ( boundingSphere.GetOrigin().z + boundingSphere.GetRadius() ) < ( origin[ 2 ] - radius ) ||
			 ( boundingSphere.GetOrigin().z - boundingSphere.GetRadius() ) > ( origin[ 2 ] + radius ) ) {
			dlightBits &= ~( 1 << i );
		}
	}

	// set counters
	if ( dlightBits == 0 ) {
		tr.pc.c_dlightSurfacesCulled++;
	} else {
		tr.pc.c_dlightSurfaces++;
	}

	// set surface dlight bits and return
	this->dlightBits[ tr.smpFrame ] = dlightBits;
	return dlightBits;
}
