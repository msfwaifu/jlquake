//**************************************************************************
//**
//**	$Id$
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

typedef int clipHandle_t;

struct q1plane_t
{
	vec3_t		normal;
	float		dist;
};

struct q1trace_t
{
	qboolean	allsolid;	// if true, plane is not valid
	qboolean	startsolid;	// if true, the initial point was in a solid area
	qboolean	inopen;
	qboolean	inwater;
	float		fraction;		// time completed, 1.0 = didn't hit anything
	vec3_t		endpos;			// final position
	q1plane_t		plane;			// surface normal at impact
	int			entityNum;			// entity the surface is on
};

struct q2csurface_t
{
	char		name[16];
	int			flags;
	int			value;
};

// a trace is returned when a box is swept through the world
struct q2trace_t
{
	qboolean	allsolid;	// if true, plane is not valid
	qboolean	startsolid;	// if true, the initial point was in a solid area
	float		fraction;	// time completed, 1.0 = didn't hit anything
	vec3_t		endpos;		// final position
	cplane_t	plane;		// surface normal at impact
	q2csurface_t	*surface;	// surface hit
	int			contents;	// contents on other side of surface hit
	struct edict_s	*ent;		// not set by CM_*() functions
};

// a trace is returned when a box is swept through the world
// trace->entityNum can also be 0 to (MAX_GENTITIES-1)
// or ENTITYNUM_NONE, ENTITYNUM_WORLD
struct q3trace_t
{
	qboolean	allsolid;	// if true, plane is not valid
	qboolean	startsolid;	// if true, the initial point was in a solid area
	float		fraction;	// time completed, 1.0 = didn't hit anything
	vec3_t		endpos;		// final position
	cplane_t	plane;		// surface normal at impact, transformed to world space
	int			surfaceFlags;	// surface hit
	int			contents;	// contents on other side of surface hit
	int			entityNum;	// entity the contacted surface is a part of
};

clipHandle_t CM_InlineModel(int Index);		// 0 = world, 1 + are bmodels

int CM_NumClusters();
int CM_NumInlineModels();
const char* CM_EntityString();
void CM_MapChecksums(int& CheckSum1, int& CheckSum2);

void CM_ModelBounds(clipHandle_t Model, vec3_t Mins, vec3_t Maxs);

int CM_LeafCluster(int LeafNum);
int CM_LeafArea(int LeafNum);
const byte* CM_LeafAmbientSoundLevel(int LeafNum);

int CM_GetNumTextures();
const char* CM_GetTextureName(int Index);

// creates a clipping hull for an arbitrary box
clipHandle_t CM_TempBoxModel(const vec3_t Mins, const vec3_t Maxs, bool Capsule = false);

clipHandle_t CM_ModelHull(clipHandle_t Model, int HullNum, vec3_t ClipMins, vec3_t ClipMaxs);
clipHandle_t CM_ModelHull(clipHandle_t Model, int HullNum);

int CM_PointLeafnum(const vec3_t Point);

// only returns non-solid leafs
// returns with TopNode set to the first node that splits the box
// overflow if return ListSize and if *LastLeaf != List[ListSize - 1]
int CM_BoxLeafnums(const vec3_t Mins, const vec3_t Maxs, int* List,
	int ListSize, int* TopNode = NULL, int* LastLeaf = NULL);

// returns an ORed contents mask
int CM_PointContentsQ1(const vec3_t Point, clipHandle_t Model);
int CM_PointContentsQ2(const vec3_t Point, clipHandle_t Model);
int CM_PointContentsQ3(const vec3_t Point, clipHandle_t Model);
int CM_TransformedPointContentsQ1(const vec3_t Point, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles);
int CM_TransformedPointContentsQ2(const vec3_t Point, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles);
int CM_TransformedPointContentsQ3(const vec3_t Point, clipHandle_t Model, const vec3_t Origin, const vec3_t Angles);

byte* CM_ClusterPVS(int Cluster);
byte* CM_ClusterPHS(int Cluster);

extern	int			c_pointcontents;
