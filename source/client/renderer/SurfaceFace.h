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

#ifndef __idSurfaceFace__
#define __idSurfaceFace__

#include "WorldSurface.h"

class idSurfaceFace : public idWorldSurface {
public:
	virtual cplane_t GetPlane() const;
	virtual void Draw();
	virtual bool CheckAddMarks( const vec3_t mins, const vec3_t maxs, const vec3_t dir ) const;
	virtual void MarkFragments( const vec3_t projectionDir,
		int numPlanes, const vec3_t* normals, const float* dists,
		int maxPoints, vec3_t pointBuffer,
		int maxFragments, markFragment_t* fragmentBuffer,
		int* returnedPoints, int* returnedFragments,
		const vec3_t mins, const vec3_t maxs ) const;
	virtual void MarkFragmentsWolf( const vec3_t projectionDir,
		int numPlanes, const vec3_t* normals, const float* dists,
		int maxPoints, vec3_t pointBuffer,
		int maxFragments, markFragment_t* fragmentBuffer,
		int* returnedPoints, int* returnedFragments,
		const vec3_t mins, const vec3_t maxs,
		bool oldMapping, const vec3_t center, float radius, const vec3_t bestnormal, int orientation, int numPoints ) const;

protected:
	virtual bool DoCull( shader_t* shader ) const;
	virtual int DoMarkDynamicLights( int dlightBits );

private:
	void MarkFragmentsOldMapping( const vec3_t projectionDir,
		int numPlanes, const vec3_t* normals, const float* dists,
		int maxPoints, vec3_t pointBuffer,
		int maxFragments, markFragment_t* fragmentBuffer,
		int* returnedPoints, int* returnedFragments,
		const vec3_t mins, const vec3_t maxs ) const;
	void MarkFragmentsWolfMapping( const vec3_t projectionDir,
		int numPlanes, const vec3_t* normals, const float* dists,
		int maxPoints, vec3_t pointBuffer,
		int maxFragments, markFragment_t* fragmentBuffer,
		int* returnedPoints, int* returnedFragments,
		const vec3_t mins, const vec3_t maxs,
		const vec3_t center, float radius, const vec3_t bestnormal, int orientation, int numPoints ) const;
};

#endif
