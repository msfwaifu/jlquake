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

// HEADER FILES ------------------------------------------------------------

#include "../../qcommon.h"
#include "local.h"

// MACROS ------------------------------------------------------------------

// always use bbox vs. bbox collision and never capsule vs. bbox or vice versa
//#define ALWAYS_BBOX_VS_BBOX
// always use capsule vs. capsule collision and never capsule vs. bbox or vice versa
//#define ALWAYS_CAPSULE_VS_CAPSULE

//#define CAPSULE_DEBUG

#define RADIUS_EPSILON      1.0f

#define MAX_POSITION_LEAFS  1024

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

/*
===============================================================================

TRACING

===============================================================================
*/

//==========================================================================
//
//	QClipMap46::BoxTraceQ3
//
//==========================================================================

void QClipMap46::BoxTraceQ3(q3trace_t* Results, const vec3_t Start, const vec3_t End,
	const vec3_t Mins, const vec3_t Maxs, clipHandle_t Model, int BrushMask, int Capsule)
{
	Trace(Results, Start, End, Mins, Maxs, Model, vec3_origin, BrushMask, Capsule, NULL);
}

//==========================================================================
//
//	QClipMap46::TransformedBoxTraceQ3
//
//	Handles offseting and rotation of the end points for moving and
// rotating entities
//
//==========================================================================

void QClipMap46::TransformedBoxTraceQ3(q3trace_t* Results, const vec3_t Start,
	const vec3_t End, const vec3_t Mins, const vec3_t Maxs, clipHandle_t Model, int BrushMask,
	const vec3_t Origin, const vec3_t Angles, int Capsule)
{
	if (!Mins)
	{
		Mins = vec3_origin;
	}
	if (!Maxs)
	{
		Maxs = vec3_origin;
	}

	// adjust so that mins and maxs are always symetric, which
	// avoids some complications with plane expanding of rotated
	// bmodels
	vec3_t offset;
	vec3_t symetricSize[2];
	vec3_t start_l, end_l;
	for (int i = 0; i < 3; i++)
	{
		offset[i] = (Mins[i] + Maxs[i]) * 0.5;
		symetricSize[0][i] = Mins[i] - offset[i];
		symetricSize[1][i] = Maxs[i] - offset[i];
		start_l[i] = Start[i] + offset[i];
		end_l[i] = End[i] + offset[i];
	}

	// subtract origin offset
	VectorSubtract(start_l, Origin, start_l);
	VectorSubtract(end_l, Origin, end_l);

	// rotate start and end into the models frame of reference
	bool rotated;
	if (Model != BOX_MODEL_HANDLE && (Angles[0] || Angles[1] || Angles[2]))
	{
		rotated = true;
	}
	else
	{
		rotated = false;
	}

	float halfwidth = symetricSize[1][0];
	float halfheight = symetricSize[1][2];

	sphere_t sphere;
	sphere.use = Capsule;
	sphere.radius = (halfwidth > halfheight) ? halfheight : halfwidth;
	sphere.halfheight = halfheight;
	float t = halfheight - sphere.radius;

	vec3_t matrix[3];
	if (rotated)
	{
		// rotation on trace line (start-end) instead of rotating the bmodel
		// NOTE: This is still incorrect for bounding boxes because the actual bounding
		//		 box that is swept through the model is not rotated. We cannot rotate
		//		 the bounding box or the bmodel because that would make all the brush
		//		 bevels invalid.
		//		 However this is correct for capsules since a capsule itself is rotated too.
		AnglesToAxis(Angles, matrix);
		RotatePoint(start_l, matrix);
		RotatePoint(end_l, matrix);
		// rotated sphere offset for capsule
		sphere.offset[0] = matrix[0][2] * t;
		sphere.offset[1] = -matrix[1][2] * t;
		sphere.offset[2] = matrix[2][2] * t;
	}
	else
	{
		VectorSet(sphere.offset, 0, 0, t);
	}

	// sweep the box through the model
	q3trace_t trace;
	Trace(&trace, start_l, end_l, symetricSize[0], symetricSize[1], Model, Origin, BrushMask, Capsule, &sphere);

	// if the bmodel was rotated and there was a collision
	if (rotated && trace.fraction != 1.0)
	{
		// rotation of bmodel collision plane
		vec3_t transpose[3];
		TransposeMatrix(matrix, transpose);
		RotatePoint(trace.plane.normal, transpose);
	}

	// re-calculate the end position of the trace because the trace.endpos
	// calculated by CM_Trace could be rotated and have an offset
	trace.endpos[0] = Start[0] + trace.fraction * (End[0] - Start[0]);
	trace.endpos[1] = Start[1] + trace.fraction * (End[1] - Start[1]);
	trace.endpos[2] = Start[2] + trace.fraction * (End[2] - Start[2]);

	*Results = trace;
}

//==========================================================================
//
//	QClipMap46::Trace
//
//==========================================================================

void QClipMap46::Trace(q3trace_t* results, const vec3_t start, const vec3_t end,
	const vec3_t mins, const vec3_t maxs, clipHandle_t model, const vec3_t origin,
	int brushmask, int capsule, sphere_t* sphere)
{
	int i;
	traceWork_t tw;
	vec3_t offset;
	cmodel_t* cmod;

	cmod = ClipHandleToModel(model);

	checkcount++;		// for multi-check avoidance

	c_traces++;				// for statistics, may be zeroed

	// fill in a default trace
	Com_Memset(&tw, 0, sizeof(tw));
	tw.trace.fraction = 1;	// assume it goes the entire distance until shown otherwise
	VectorCopy(origin, tw.modelOrigin);

	if (!numNodes)
	{
		*results = tw.trace;

		return;	// map not loaded, shouldn't happen
	}

	// allow NULL to be passed in for 0,0,0
	if (!mins)
	{
		mins = vec3_origin;
	}
	if (!maxs)
	{
		maxs = vec3_origin;
	}

	// set basic parms
	tw.contents = brushmask;

	// adjust so that mins and maxs are always symetric, which
	// avoids some complications with plane expanding of rotated
	// bmodels
	for (i = 0; i < 3; i++)
	{
		offset[i] = (mins[i] + maxs[i]) * 0.5;
		tw.size[0][i] = mins[i] - offset[i];
		tw.size[1][i] = maxs[i] - offset[i];
		tw.start[i] = start[i] + offset[i];
		tw.end[i] = end[i] + offset[i];
	}

	// if a sphere is already specified
	if (sphere)
	{
		tw.sphere = *sphere;
	}
	else
	{
		tw.sphere.use = capsule;
		tw.sphere.radius = (tw.size[1][0] > tw.size[1][2]) ? tw.size[1][2] : tw.size[1][0];
		tw.sphere.halfheight = tw.size[1][2];
		VectorSet(tw.sphere.offset, 0, 0, tw.size[1][2] - tw.sphere.radius);
	}

	bool positionTest = start[0] == end[0] && start[1] == end[1] && start[2] == end[2];

	tw.maxOffset = tw.size[1][0] + tw.size[1][1] + tw.size[1][2];

	// tw.offsets[signbits] = vector to apropriate corner from origin
	tw.offsets[0][0] = tw.size[0][0];
	tw.offsets[0][1] = tw.size[0][1];
	tw.offsets[0][2] = tw.size[0][2];

	tw.offsets[1][0] = tw.size[1][0];
	tw.offsets[1][1] = tw.size[0][1];
	tw.offsets[1][2] = tw.size[0][2];

	tw.offsets[2][0] = tw.size[0][0];
	tw.offsets[2][1] = tw.size[1][1];
	tw.offsets[2][2] = tw.size[0][2];

	tw.offsets[3][0] = tw.size[1][0];
	tw.offsets[3][1] = tw.size[1][1];
	tw.offsets[3][2] = tw.size[0][2];

	tw.offsets[4][0] = tw.size[0][0];
	tw.offsets[4][1] = tw.size[0][1];
	tw.offsets[4][2] = tw.size[1][2];

	tw.offsets[5][0] = tw.size[1][0];
	tw.offsets[5][1] = tw.size[0][1];
	tw.offsets[5][2] = tw.size[1][2];

	tw.offsets[6][0] = tw.size[0][0];
	tw.offsets[6][1] = tw.size[1][1];
	tw.offsets[6][2] = tw.size[1][2];

	tw.offsets[7][0] = tw.size[1][0];
	tw.offsets[7][1] = tw.size[1][1];
	tw.offsets[7][2] = tw.size[1][2];

	//
	// check for point special case
	//
	if (tw.size[0][0] == 0 && tw.size[0][1] == 0 && tw.size[0][2] == 0)
	{
		tw.isPoint = true;
		VectorClear(tw.extents);
	}
	else
	{
		tw.isPoint = false;
		tw.extents[0] = tw.size[1][0];
		tw.extents[1] = tw.size[1][1];
		tw.extents[2] = tw.size[1][2];
	}

	if (GGameType & GAME_ET)
	{
		if (positionTest)
		{
			CalcTraceBounds(&tw, false);
		}
		else
		{
			vec3_t dir;
			VectorSubtract(tw.end, tw.start, dir);
			VectorCopy(dir, tw.dir);
			VectorNormalize(dir);
			MakeNormalVectors(dir, tw.tracePlane1.normal, tw.tracePlane2.normal);
			tw.tracePlane1.dist = DotProduct(tw.tracePlane1.normal, tw.start);
			tw.tracePlane2.dist = DotProduct(tw.tracePlane2.normal, tw.start);
			if (tw.isPoint)
			{
				tw.traceDist1 = tw.traceDist2 = 1.0f;
			}
			else
			{
				tw.traceDist1 = tw.traceDist2 = 0.0f;
				for (i = 0; i < 8; i++)
				{
					float dist = Q_fabs(DotProduct(tw.tracePlane1.normal, tw.offsets[i]) - tw.tracePlane1.dist);
					if (dist > tw.traceDist1)
					{
						tw.traceDist1 = dist;
					}
					dist = Q_fabs(DotProduct(tw.tracePlane2.normal, tw.offsets[i]) - tw.tracePlane2.dist);
					if (dist > tw.traceDist2)
					{
						tw.traceDist2 = dist;
					}
				}
				// expand for epsilon
				tw.traceDist1 += 1.0f;
				tw.traceDist2 += 1.0f;
			}

			CalcTraceBounds(&tw, true);
		}
	}
	else
	{
		//
		// calculate bounds
		//
		if (tw.sphere.use)
		{
			for (i = 0; i < 3; i++)
			{
				if (tw.start[i] < tw.end[i])
				{
					tw.bounds[0][i] = tw.start[i] - Q_fabs(tw.sphere.offset[i]) - tw.sphere.radius;
					tw.bounds[1][i] = tw.end[i] + Q_fabs(tw.sphere.offset[i]) + tw.sphere.radius;
				}
				else
				{
					tw.bounds[0][i] = tw.end[i] - Q_fabs(tw.sphere.offset[i]) - tw.sphere.radius;
					tw.bounds[1][i] = tw.start[i] + Q_fabs(tw.sphere.offset[i]) + tw.sphere.radius;
				}
			}
		}
		else
		{
			for (i = 0; i < 3; i++)
			{
				if (tw.start[i] < tw.end[i])
				{
					tw.bounds[0][i] = tw.start[i] + tw.size[0][i];
					tw.bounds[1][i] = tw.end[i] + tw.size[1][i];
				}
				else
				{
					tw.bounds[0][i] = tw.end[i] + tw.size[0][i];
					tw.bounds[1][i] = tw.start[i] + tw.size[1][i];
				}
			}
		}
	}

	//
	// check for position test special case
	//
	if (positionTest)
	{
		if (model)
		{
			if (GGameType & (GAME_WolfSP | GAME_ET))
			{
				//	Always box vs. box.
				if (model == BOX_MODEL_HANDLE || model == CAPSULE_MODEL_HANDLE)
				{
					tw.sphere.use = false;
				}
				TestInLeaf(&tw, &cmod->leaf);
			}
			else
			{
#ifdef ALWAYS_BBOX_VS_BBOX	// bk010201 - FIXME - compile time flag?
				if (model == BOX_MODEL_HANDLE || model == CAPSULE_MODEL_HANDLE)
				{
					tw.sphere.use = false;
					TestInLeaf(&tw, &cmod->leaf);
				}
				else
#elif defined(ALWAYS_CAPSULE_VS_CAPSULE)
				if (model == BOX_MODEL_HANDLE || model == CAPSULE_MODEL_HANDLE)
				{
					TestCapsuleInCapsule(&tw, model);
				}
				else
#endif
				if (model == CAPSULE_MODEL_HANDLE)
				{
					if (tw.sphere.use)
					{
						TestCapsuleInCapsule(&tw, model);
					}
					else
					{
						TestBoundingBoxInCapsule(&tw, model);
					}
				}
				else
				{
					TestInLeaf(&tw, &cmod->leaf);
				}
			}
		}
		else
		{
			PositionTest(&tw);
		}
	}
	else
	{
		//
		// general sweeping through world
		//
		if (model)
		{
			if (GGameType & (GAME_WolfSP | GAME_ET))
			{
				//	Always box vs. box.
				if (model == BOX_MODEL_HANDLE || model == CAPSULE_MODEL_HANDLE)
				{
					tw.sphere.use = false;
				}
				TraceThroughLeaf(&tw, &cmod->leaf);
			}
			else
			{
#ifdef ALWAYS_BBOX_VS_BBOX
				if (model == BOX_MODEL_HANDLE || model == CAPSULE_MODEL_HANDLE)
				{
					tw.sphere.use = false;
					TraceThroughLeaf(&tw, &cmod->leaf);
				}
				else
#elif defined(ALWAYS_CAPSULE_VS_CAPSULE)
				if (model == BOX_MODEL_HANDLE || model == CAPSULE_MODEL_HANDLE)
				{
					TraceCapsuleThroughCapsule(&tw, model);
				}
				else
#endif
				if (model == CAPSULE_MODEL_HANDLE)
				{
					if (tw.sphere.use)
					{
						TraceCapsuleThroughCapsule(&tw, model);
					}
					else
					{
						TraceBoundingBoxThroughCapsule(&tw, model);
					}
				}
				else
				{
					TraceThroughLeaf(&tw, &cmod->leaf);
				}
			}
		}
		else
		{
			TraceThroughTree(&tw, 0, 0, 1, tw.start, tw.end);
		}
	}

	// generate endpos from the original, unmodified start/end
	if (tw.trace.fraction == 1)
	{
		VectorCopy(end, tw.trace.endpos);
	}
	else
	{
		for (i = 0; i < 3; i++)
		{
			tw.trace.endpos[i] = start[i] + tw.trace.fraction * (end[i] - start[i]);
		}
	}

	// If allsolid is set (was entirely inside something solid), the plane is not valid.
	// If fraction == 1.0, we never hit anything, and thus the plane is not valid.
	// Otherwise, the normal on the plane should have unit length
	qassert(tw.trace.allsolid ||
		tw.trace.fraction == 1.0 ||
		VectorLengthSquared(tw.trace.plane.normal) > 0.9999);
	*results = tw.trace;
}

//==========================================================================
//
//	QClipMap46::TraceThroughTree
//
//	Traverse all the contacted leafs from the start to the end position.
// If the trace is a point, they will be exactly in order, but for larger
// trace volumes it is possible to hit something in a later leaf with
// a smaller intercept fraction.
//
//==========================================================================

void QClipMap46::TraceThroughTree(traceWork_t* tw, int num, float p1f, float p2f, vec3_t p1, vec3_t p2)
{
	cNode_t* node;
	cplane_t* plane;
	float t1, t2, offset;
	float frac, frac2;
	float idist;
	vec3_t mid;
	int side;
	float midf;

	if (tw->trace.fraction <= p1f)
	{
		return;		// already hit something nearer
	}

	// if < 0, we are in a leaf node
	if (num < 0)
	{
		TraceThroughLeaf(tw, &leafs[-1 - num]);
		return;
	}

	//
	// find the point distances to the seperating plane
	// and the offset for the size of the box
	//
	node = nodes + num;
	plane = node->plane;

	// adjust the plane distance apropriately for mins/maxs
	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
		offset = tw->extents[plane->type];
	}
	else
	{
		t1 = DotProduct(plane->normal, p1) - plane->dist;
		t2 = DotProduct(plane->normal, p2) - plane->dist;
		if (tw->isPoint)
		{
			offset = 0;
		}
		else
		{
#if 0	// bk010201 - DEAD
			// an axial brush right behind a slanted bsp plane
			// will poke through when expanded, so adjust
			// by sqrt(3)
			offset = fabs(tw->extents[0] * plane->normal[0]) +
					 fabs(tw->extents[1] * plane->normal[1]) +
					 fabs(tw->extents[2] * plane->normal[2]);

			offset *= 2;
			offset = tw->maxOffset;
#endif
			if (GGameType & GAME_ET)
			{
				offset = tw->maxOffset;
			}
			else
			{
				// this is silly
				offset = 2048;
			}
		}
	}

	// see which sides we need to consider
	if (t1 >= offset + 1 && t2 >= offset + 1)
	{
		TraceThroughTree(tw, node->children[0], p1f, p2f, p1, p2);
		return;
	}
	if (t1 < -offset - 1 && t2 < -offset - 1)
	{
		TraceThroughTree(tw, node->children[1], p1f, p2f, p1, p2);
		return;
	}

	// put the crosspoint SURFACE_CLIP_EPSILON pixels on the near side
	if (t1 < t2)
	{
		idist = 1.0 / (t1 - t2);
		side = 1;
		frac2 = (t1 + offset + SURFACE_CLIP_EPSILON) * idist;
		frac = (t1 - offset + SURFACE_CLIP_EPSILON) * idist;
	}
	else if (t1 > t2)
	{
		idist = 1.0 / (t1 - t2);
		side = 0;
		frac2 = (t1 - offset - SURFACE_CLIP_EPSILON) * idist;
		frac = (t1 + offset + SURFACE_CLIP_EPSILON) * idist;
	}
	else
	{
		side = 0;
		frac = 1;
		frac2 = 0;
	}

	// move up to the node
	if (frac < 0)
	{
		frac = 0;
	}
	if (frac > 1)
	{
		frac = 1;
	}

	midf = p1f + (p2f - p1f) * frac;

	mid[0] = p1[0] + frac * (p2[0] - p1[0]);
	mid[1] = p1[1] + frac * (p2[1] - p1[1]);
	mid[2] = p1[2] + frac * (p2[2] - p1[2]);

	TraceThroughTree(tw, node->children[side], p1f, midf, p1, mid);


	// go past the node
	if (frac2 < 0)
	{
		frac2 = 0;
	}
	if (frac2 > 1)
	{
		frac2 = 1;
	}

	midf = p1f + (p2f - p1f) * frac2;

	mid[0] = p1[0] + frac2 * (p2[0] - p1[0]);
	mid[1] = p1[1] + frac2 * (p2[1] - p1[1]);
	mid[2] = p1[2] + frac2 * (p2[2] - p1[2]);

	TraceThroughTree(tw, node->children[side ^ 1], midf, p2f, mid, p2);
}

//==========================================================================
//
//	QClipMap46::TraceThroughLeaf
//
//==========================================================================

void QClipMap46::TraceThroughLeaf(traceWork_t* tw, cLeaf_t* leaf)
{
	int k;
	int brushnum;
	cbrush_t* b;
	cPatch_t* patch;

	// trace line against all brushes in the leaf
	for (k = 0; k < leaf->numLeafBrushes; k++)
	{
		brushnum = leafbrushes[leaf->firstLeafBrush + k];

		b = &brushes[brushnum];
		if (b->checkcount == checkcount)
		{
			continue;	// already checked this brush in another leaf
		}
		b->checkcount = checkcount;

		if (!(b->contents & tw->contents))
		{
			continue;
		}

		if (GGameType & GAME_ET && cm_optimize->integer)
		{
			if (!QClipMap46::TraceThroughBounds(tw, b->bounds[0], b->bounds[1]))
			{
				continue;
			}
		}

		float fraction = tw->trace.fraction;

		TraceThroughBrush(tw, b);
		if (!tw->trace.fraction)
		{
			return;
		}

		if (GGameType & GAME_ET && tw->trace.fraction < fraction)
		{
			QClipMap46::CalcTraceBounds(tw, true);
		}
	}

	// trace line against all patches in the leaf
	if (!cm_noCurves->integer)
	{
		for (k = 0; k < leaf->numLeafSurfaces; k++)
		{
			patch = surfaces[leafsurfaces[leaf->firstLeafSurface + k]];
			if (!patch)
			{
				continue;
			}
			if (patch->checkcount == checkcount)
			{
				continue;	// already checked this patch in another leaf
			}
			patch->checkcount = checkcount;

			if (!(patch->contents & tw->contents))
			{
				continue;
			}

			if (GGameType & GAME_ET && cm_optimize->integer)
			{
				if (!QClipMap46::TraceThroughBounds(tw, patch->pc->bounds[0], patch->pc->bounds[1]))
				{
					continue;
				}
			}

			float fraction = tw->trace.fraction;

			TraceThroughPatch(tw, patch);
			if (!tw->trace.fraction)
			{
				return;
			}

			if (GGameType & GAME_ET && tw->trace.fraction < fraction)
			{
				QClipMap46::CalcTraceBounds(tw, true);
			}
		}
	}
}

void QClipMap46::CalcTraceBounds(traceWork_t* tw, bool expand)
{
	if (tw->sphere.use)
	{
		for (int i = 0; i < 3; i++)
		{
			if (tw->start[i] < tw->end[i])
			{
				tw->bounds[0][i] = tw->start[i] - Q_fabs(tw->sphere.offset[i]) - tw->sphere.radius;
				tw->bounds[1][i] = tw->start[i] + tw->trace.fraction * tw->dir[i] + Q_fabs(tw->sphere.offset[i]) + tw->sphere.radius;
			}
			else
			{
				tw->bounds[0][i] = tw->start[i] + tw->trace.fraction * tw->dir[i] - Q_fabs(tw->sphere.offset[i]) - tw->sphere.radius;
				tw->bounds[1][i] = tw->start[i] + Q_fabs(tw->sphere.offset[i]) + tw->sphere.radius;
			}
		}
	}
	else
	{
		for (int i = 0; i < 3; i++)
		{
			if (tw->start[i] < tw->end[i])
			{
				tw->bounds[0][i] = tw->start[i] + tw->size[0][i];
				tw->bounds[1][i] = tw->start[i] + tw->trace.fraction * tw->dir[i] + tw->size[1][i];
			}
			else
			{
				tw->bounds[0][i] = tw->start[i] + tw->trace.fraction * tw->dir[i] + tw->size[0][i];
				tw->bounds[1][i] = tw->start[i] + tw->size[1][i];
			}
		}
	}

	if (expand)
	{
		// expand for epsilon
		for (int i = 0; i < 3; i++)
		{
			tw->bounds[0][i] -= 1.0f;
			tw->bounds[1][i] += 1.0f;
		}
	}
}

int QClipMap46::TraceThroughBounds(const traceWork_t* tw, const vec3_t mins, const vec3_t maxs)
{
	for (int i = 0; i < 3; i++)
	{
		if (mins[i] > tw->bounds[1][i])
		{
			return false;
		}
		if (maxs[i] < tw->bounds[0][i])
		{
			return false;
		}
	}

	vec3_t center;
	VectorAdd(mins, maxs, center);
	VectorScale(center, 0.5f, center);
	vec3_t extents;
	VectorSubtract(maxs, center, extents);

	if (Q_fabs(BoxDistanceFromPlane(center, extents, &tw->tracePlane1)) > tw->traceDist1)
	{
		return false;
	}
	if (Q_fabs(BoxDistanceFromPlane(center, extents, &tw->tracePlane2)) > tw->traceDist2)
	{
		return false;
	}

	// trace might go through the bounds
	return true;
}

float QClipMap46::BoxDistanceFromPlane(const vec3_t center, const vec3_t extents, const cplane_t* plane)
{
	float d1 = DotProduct(center, plane->normal) - plane->dist;
	float d2 = Q_fabs(extents[0] * plane->normal[0]) +
			   Q_fabs(extents[1] * plane->normal[1]) +
			   Q_fabs(extents[2] * plane->normal[2]);

	if (d1 - d2 > 0.0f)
	{
		return d1 - d2;
	}
	if (d1 + d2 < 0.0f)
	{
		return d1 + d2;
	}
	return 0.0f;
}

//==========================================================================
//
//	QClipMap46::TraceThroughBrush
//
//==========================================================================

void QClipMap46::TraceThroughBrush(traceWork_t* tw, cbrush_t* brush)
{
	int i;
	cplane_t* plane, * clipplane;
	float dist;
	float enterFrac, leaveFrac;
	float d1, d2;
	qboolean getout, startout;
	float f;
	cbrushside_t* side, * leadside;
	float t;
	vec3_t startp;
	vec3_t endp;

	enterFrac = -1.0;
	leaveFrac = 1.0;
	clipplane = NULL;

	if (!brush->numsides)
	{
		return;
	}

	c_brush_traces++;

	getout = false;
	startout = false;

	leadside = NULL;

	if (tw->sphere.use)
	{
		//
		// compare the trace against all planes of the brush
		// find the latest time the trace crosses a plane towards the interior
		// and the earliest time the trace crosses a plane towards the exterior
		//
		for (i = 0; i < brush->numsides; i++)
		{
			side = brush->sides + i;
			plane = side->plane;

			// adjust the plane distance apropriately for radius
			dist = plane->dist + tw->sphere.radius;

			// find the closest point on the capsule to the plane
			t = DotProduct(plane->normal, tw->sphere.offset);
			if (t > 0)
			{
				VectorSubtract(tw->start, tw->sphere.offset, startp);
				VectorSubtract(tw->end, tw->sphere.offset, endp);
			}
			else
			{
				VectorAdd(tw->start, tw->sphere.offset, startp);
				VectorAdd(tw->end, tw->sphere.offset, endp);
			}

			d1 = DotProduct(startp, plane->normal) - dist;
			d2 = DotProduct(endp, plane->normal) - dist;

			if (d2 > 0)
			{
				getout = true;	// endpoint is not in solid
			}
			if (d1 > 0)
			{
				startout = true;
			}

			// if completely in front of face, no intersection with the entire brush
			if (d1 > 0 && (d2 >= SURFACE_CLIP_EPSILON || d2 >= d1))
			{
				return;
			}

			// if it doesn't cross the plane, the plane isn't relevent
			if (d1 <= 0 && d2 <= 0)
			{
				continue;
			}

			// crosses face
			if (d1 > d2)	// enter
			{
				f = (d1 - SURFACE_CLIP_EPSILON) / (d1 - d2);
				if (f < 0)
				{
					f = 0;
				}
				if (f > enterFrac)
				{
					enterFrac = f;
					clipplane = plane;
					leadside = side;
				}
			}
			else		// leave
			{
				f = (d1 + SURFACE_CLIP_EPSILON) / (d1 - d2);
				if (f > 1)
				{
					f = 1;
				}
				if (f < leaveFrac)
				{
					leaveFrac = f;
				}
			}
		}
	}
	else
	{
		//
		// compare the trace against all planes of the brush
		// find the latest time the trace crosses a plane towards the interior
		// and the earliest time the trace crosses a plane towards the exterior
		//
		for (i = 0; i < brush->numsides; i++)
		{
			side = brush->sides + i;
			plane = side->plane;

			// adjust the plane distance apropriately for mins/maxs
			dist = plane->dist - DotProduct(tw->offsets[plane->signbits], plane->normal);

			d1 = DotProduct(tw->start, plane->normal) - dist;
			d2 = DotProduct(tw->end, plane->normal) - dist;

			if (d2 > 0)
			{
				getout = true;	// endpoint is not in solid
			}
			if (d1 > 0)
			{
				startout = true;
			}

			// if completely in front of face, no intersection with the entire brush
			if (d1 > 0 && (d2 >= SURFACE_CLIP_EPSILON || d2 >= d1))
			{
				return;
			}

			// if it doesn't cross the plane, the plane isn't relevent
			if (d1 <= 0 && d2 <= 0)
			{
				continue;
			}

			// crosses face
			if (d1 > d2)	// enter
			{
				f = (d1 - SURFACE_CLIP_EPSILON) / (d1 - d2);
				if (f < 0)
				{
					f = 0;
				}
				if (f > enterFrac)
				{
					enterFrac = f;
					clipplane = plane;
					leadside = side;
				}
			}
			else		// leave
			{
				f = (d1 + SURFACE_CLIP_EPSILON) / (d1 - d2);
				if (f > 1)
				{
					f = 1;
				}
				if (f < leaveFrac)
				{
					leaveFrac = f;
				}
			}
		}
	}

	//
	// all planes have been checked, and the trace was not
	// completely outside the brush
	//
	if (!startout)		// original point was inside brush
	{
		tw->trace.startsolid = true;
		if (!getout)
		{
			tw->trace.allsolid = true;
			tw->trace.fraction = 0;
			if (!(GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET)))
			{
				tw->trace.contents = brush->contents;
			}
		}
		return;
	}

	if (enterFrac < leaveFrac)
	{
		if (enterFrac > -1 && enterFrac < tw->trace.fraction)
		{
			if (enterFrac < 0)
			{
				enterFrac = 0;
			}
			tw->trace.fraction = enterFrac;
			tw->trace.plane = *clipplane;
			tw->trace.surfaceFlags = leadside->surfaceFlags;
			tw->trace.contents = brush->contents;
		}
	}
}

//==========================================================================
//
//	QClipMap46::TraceThroughPatch
//
//==========================================================================

void QClipMap46::TraceThroughPatch(traceWork_t* tw, cPatch_t* patch)
{
	float oldFrac;

	c_patch_traces++;

	oldFrac = tw->trace.fraction;

	patch->pc->TraceThrough(tw);

	if (tw->trace.fraction < oldFrac)
	{
		tw->trace.surfaceFlags = patch->surfaceFlags;
		tw->trace.contents = patch->contents;
	}
}

//==========================================================================
//
//	QClipMap46::TraceBoundingBoxThroughCapsule
//
//	bounding box vs. capsule collision
//
//==========================================================================

void QClipMap46::TraceBoundingBoxThroughCapsule(traceWork_t* tw, clipHandle_t model)
{
	vec3_t mins, maxs, offset, size[2];
	clipHandle_t h;
	cmodel_t* cmod;
	int i;

	// mins maxs of the capsule
	ModelBounds(model, mins, maxs);

	// offset for capsule center
	for (i = 0; i < 3; i++)
	{
		offset[i] = (mins[i] + maxs[i]) * 0.5;
		size[0][i] = mins[i] - offset[i];
		size[1][i] = maxs[i] - offset[i];
		tw->start[i] -= offset[i];
		tw->end[i] -= offset[i];
	}

	// replace the bounding box with the capsule
	tw->sphere.use = true;
	tw->sphere.radius = (size[1][0] > size[1][2]) ? size[1][2] : size[1][0];
	tw->sphere.halfheight = size[1][2];
	VectorSet(tw->sphere.offset, 0, 0, size[1][2] - tw->sphere.radius);

	// replace the capsule with the bounding box
	h = TempBoxModel(tw->size[0], tw->size[1], false);
	// calculate collision
	cmod = ClipHandleToModel(h);
	TraceThroughLeaf(tw, &cmod->leaf);
}

//==========================================================================
//
//	QClipMap46::TraceCapsuleThroughCapsule
//
//	capsule vs. capsule collision (not rotated)
//
//==========================================================================

void QClipMap46::TraceCapsuleThroughCapsule(traceWork_t* tw, clipHandle_t model)
{
	int i;
	vec3_t mins, maxs;
	vec3_t top, bottom, starttop, startbottom, endtop, endbottom;
	vec3_t offset, symetricSize[2];
	float radius, halfwidth, halfheight, offs, h;

	ModelBounds(model, mins, maxs);
	// test trace bounds vs. capsule bounds
	if (tw->bounds[0][0] > maxs[0] + RADIUS_EPSILON ||
		tw->bounds[0][1] > maxs[1] + RADIUS_EPSILON ||
		tw->bounds[0][2] > maxs[2] + RADIUS_EPSILON ||
		tw->bounds[1][0] < mins[0] - RADIUS_EPSILON ||
		tw->bounds[1][1] < mins[1] - RADIUS_EPSILON ||
		tw->bounds[1][2] < mins[2] - RADIUS_EPSILON
		)
	{
		return;
	}
	// top origin and bottom origin of each sphere at start and end of trace
	VectorAdd(tw->start, tw->sphere.offset, starttop);
	VectorSubtract(tw->start, tw->sphere.offset, startbottom);
	VectorAdd(tw->end, tw->sphere.offset, endtop);
	VectorSubtract(tw->end, tw->sphere.offset, endbottom);

	// calculate top and bottom of the capsule spheres to collide with
	for (i = 0; i < 3; i++)
	{
		offset[i] = (mins[i] + maxs[i]) * 0.5;
		symetricSize[0][i] = mins[i] - offset[i];
		symetricSize[1][i] = maxs[i] - offset[i];
	}
	halfwidth = symetricSize[1][0];
	halfheight = symetricSize[1][2];
	radius = (halfwidth > halfheight) ? halfheight : halfwidth;
	offs = halfheight - radius;
	VectorCopy(offset, top);
	top[2] += offs;
	VectorCopy(offset, bottom);
	bottom[2] -= offs;
	// expand radius of spheres
	radius += tw->sphere.radius;
	// if there is horizontal movement
	if (tw->start[0] != tw->end[0] || tw->start[1] != tw->end[1])
	{
		// height of the expanded cylinder is the height of both cylinders minus the radius of both spheres
		h = halfheight + tw->sphere.halfheight - radius;
		// if the cylinder has a height
		if (h > 0)
		{
			// test for collisions between the cylinders
			TraceThroughVerticalCylinder(tw, offset, radius, h, tw->start, tw->end);
		}
	}
	// test for collision between the spheres
	TraceThroughSphere(tw, top, radius, startbottom, endbottom);
	TraceThroughSphere(tw, bottom, radius, starttop, endtop);
}

//==========================================================================
//
//	QClipMap46::TraceThroughVerticalCylinder
//
//	get the first intersection of the ray with the cylinder
// the cylinder extends halfheight above and below the origin
//
//==========================================================================

void QClipMap46::TraceThroughVerticalCylinder(traceWork_t* tw, vec3_t origin, float radius, float halfheight, vec3_t start, vec3_t end)
{
	float length, scale, fraction, l1, l2;
	float a, b, c, d, sqrtd;
	vec3_t v1, dir, start2d, end2d, org2d, intersection;

	// 2d coordinates
	VectorSet(start2d, start[0], start[1], 0);
	VectorSet(end2d, end[0], end[1], 0);
	VectorSet(org2d, origin[0], origin[1], 0);
	// if between lower and upper cylinder bounds
	if (start[2] <= origin[2] + halfheight &&
		start[2] >= origin[2] - halfheight)
	{
		// if inside the cylinder
		VectorSubtract(start2d, org2d, dir);
		l1 = VectorLengthSquared(dir);
		if (l1 < Square(radius))
		{
			tw->trace.fraction = 0;
			tw->trace.startsolid = true;
			VectorSubtract(end2d, org2d, dir);
			l1 = VectorLengthSquared(dir);
			if (l1 < Square(radius))
			{
				tw->trace.allsolid = true;
			}
			return;
		}
	}
	//
	VectorSubtract(end2d, start2d, dir);
	length = VectorNormalize(dir);
	//
	l1 = DistanceFromLineSquaredDir(org2d, start2d, end2d, dir);
	VectorSubtract(end2d, org2d, v1);
	l2 = VectorLengthSquared(v1);
	// if no intersection with the cylinder and the end point is at least an epsilon away
	if (l1 >= Square(radius) && l2 > Square(radius + SURFACE_CLIP_EPSILON))
	{
		return;
	}
	//
	//
	// (start[0] - origin[0] - t * dir[0]) ^ 2 + (start[1] - origin[1] - t * dir[1]) ^ 2 = radius ^ 2
	// (v1[0] + t * dir[0]) ^ 2 + (v1[1] + t * dir[1]) ^ 2 = radius ^ 2;
	// v1[0] ^ 2 + 2 * v1[0] * t * dir[0] + (t * dir[0]) ^ 2 +
	//						v1[1] ^ 2 + 2 * v1[1] * t * dir[1] + (t * dir[1]) ^ 2 = radius ^ 2
	// t ^ 2 * (dir[0] ^ 2 + dir[1] ^ 2) + t * (2 * v1[0] * dir[0] + 2 * v1[1] * dir[1]) +
	//						v1[0] ^ 2 + v1[1] ^ 2 - radius ^ 2 = 0
	//
	VectorSubtract(start, origin, v1);
	// dir is normalized so we can use a = 1
	a = 1.0f;	// * (dir[0] * dir[0] + dir[1] * dir[1]);
	b = 2.0f * (v1[0] * dir[0] + v1[1] * dir[1]);
	c = v1[0] * v1[0] + v1[1] * v1[1] - (radius + RADIUS_EPSILON) * (radius + RADIUS_EPSILON);

	d = b * b - 4.0f * c;	// * a;
	if (d > 0)
	{
		sqrtd = SquareRootFloat(d);
		// = (- b + sqrtd) * 0.5f;// / (2.0f * a);
		fraction = (-b - sqrtd) * 0.5f;	// / (2.0f * a);
		//
		if (fraction < 0)
		{
			fraction = 0;
		}
		else
		{
			fraction /= length;
		}
		if (fraction < tw->trace.fraction)
		{
			VectorSubtract(end, start, dir);
			VectorMA(start, fraction, dir, intersection);
			// if the intersection is between the cylinder lower and upper bound
			if (intersection[2] <= origin[2] + halfheight &&
				intersection[2] >= origin[2] - halfheight)
			{
				//
				tw->trace.fraction = fraction;
				VectorSubtract(intersection, origin, dir);
				dir[2] = 0;
				#ifdef CAPSULE_DEBUG
				l2 = VectorLength(dir);
				if (l2 <= radius)
				{
					int bah = 1;
				}
				#endif
				scale = 1 / (radius + RADIUS_EPSILON);
				VectorScale(dir, scale, dir);
				VectorCopy(dir, tw->trace.plane.normal);
				VectorAdd(tw->modelOrigin, intersection, intersection);
				tw->trace.plane.dist = DotProduct(tw->trace.plane.normal, intersection);
				tw->trace.contents = BSP46CONTENTS_BODY;
			}
		}
	}
	else if (d == 0)
	{
		//t[0] = (- b ) / 2 * a;
		// slide along the cylinder
	}
	// no intersection at all
}

//==========================================================================
//
//	QClipMap46::TraceThroughSphere
//
//	get the first intersection of the ray with the sphere
//
//==========================================================================

void QClipMap46::TraceThroughSphere(traceWork_t* tw, vec3_t origin, float radius, vec3_t start, vec3_t end)
{
	float l1, l2, length, scale, fraction;
	float a, b, c, d, sqrtd;
	vec3_t v1, dir, intersection;

	// if inside the sphere
	VectorSubtract(start, origin, dir);
	l1 = VectorLengthSquared(dir);
	if (l1 < Square(radius))
	{
		tw->trace.fraction = 0;
		tw->trace.startsolid = true;
		// test for allsolid
		VectorSubtract(end, origin, dir);
		l1 = VectorLengthSquared(dir);
		if (l1 < Square(radius))
		{
			tw->trace.allsolid = true;
		}
		return;
	}
	//
	VectorSubtract(end, start, dir);
	length = VectorNormalize(dir);
	//
	l1 = DistanceFromLineSquaredDir(origin, start, end, dir);
	VectorSubtract(end, origin, v1);
	l2 = VectorLengthSquared(v1);
	// if no intersection with the sphere and the end point is at least an epsilon away
	if (l1 >= Square(radius) && l2 > Square(radius + SURFACE_CLIP_EPSILON))
	{
		return;
	}
	//
	//	| origin - (start + t * dir) | = radius
	//	a = dir[0]^2 + dir[1]^2 + dir[2]^2;
	//	b = 2 * (dir[0] * (start[0] - origin[0]) + dir[1] * (start[1] - origin[1]) + dir[2] * (start[2] - origin[2]));
	//	c = (start[0] - origin[0])^2 + (start[1] - origin[1])^2 + (start[2] - origin[2])^2 - radius^2;
	//
	VectorSubtract(start, origin, v1);
	// dir is normalized so a = 1
	a = 1.0f;	//dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2];
	b = 2.0f * (dir[0] * v1[0] + dir[1] * v1[1] + dir[2] * v1[2]);
	c = v1[0] * v1[0] + v1[1] * v1[1] + v1[2] * v1[2] - (radius + RADIUS_EPSILON) * (radius + RADIUS_EPSILON);

	d = b * b - 4.0f * c;	// * a;
	if (d > 0)
	{
		sqrtd = SquareRootFloat(d);
		// = (- b + sqrtd) * 0.5f; // / (2.0f * a);
		fraction = (-b - sqrtd) * 0.5f;	// / (2.0f * a);
		//
		if (fraction < 0)
		{
			fraction = 0;
		}
		else
		{
			fraction /= length;
		}
		if (fraction < tw->trace.fraction)
		{
			tw->trace.fraction = fraction;
			VectorSubtract(end, start, dir);
			VectorMA(start, fraction, dir, intersection);
			VectorSubtract(intersection, origin, dir);
			#ifdef CAPSULE_DEBUG
			l2 = VectorLength(dir);
			if (l2 < radius)
			{
				int bah = 1;
			}
			#endif
			scale = 1 / (radius + RADIUS_EPSILON);
			VectorScale(dir, scale, dir);
			VectorCopy(dir, tw->trace.plane.normal);
			VectorAdd(tw->modelOrigin, intersection, intersection);
			tw->trace.plane.dist = DotProduct(tw->trace.plane.normal, intersection);
			tw->trace.contents = BSP46CONTENTS_BODY;
		}
	}
	else if (d == 0)
	{
		//t1 = (- b ) / 2;
		// slide along the sphere
	}
	// no intersection at all
}

/*
===============================================================================

POSITION TESTING

===============================================================================
*/

//==========================================================================
//
//	QClipMap46::PositionTest
//
//==========================================================================

void QClipMap46::PositionTest(traceWork_t* tw)
{
	int leafs[MAX_POSITION_LEAFS];
	leafList_t ll;

	// identify the leafs we are touching
	VectorAdd(tw->start, tw->size[0], ll.bounds[0]);
	VectorAdd(tw->start, tw->size[1], ll.bounds[1]);

	for (int i = 0; i < 3; i++)
	{
		ll.bounds[0][i] -= 1;
		ll.bounds[1][i] += 1;
	}

	ll.count = 0;
	ll.maxcount = MAX_POSITION_LEAFS;
	ll.list = leafs;
	ll.topnode = -1;
	ll.lastLeaf = 0;

	BoxLeafnums_r(&ll, 0);

	checkcount++;

	// test the contents of the leafs
	for (int i = 0; i < ll.count; i++)
	{
		TestInLeaf(tw, &this->leafs[leafs[i]]);
		if (tw->trace.allsolid)
		{
			break;
		}
	}
}

//==========================================================================
//
//	QClipMap46::TestInLeaf
//
//==========================================================================

void QClipMap46::TestInLeaf(traceWork_t* tw, cLeaf_t* leaf)
{
	int k;
	int brushnum;
	cbrush_t* b;
	cPatch_t* patch;

	// test box position against all brushes in the leaf
	for (k = 0; k < leaf->numLeafBrushes; k++)
	{
		brushnum = leafbrushes[leaf->firstLeafBrush + k];
		b = &brushes[brushnum];
		if (b->checkcount == checkcount)
		{
			continue;	// already checked this brush in another leaf
		}
		b->checkcount = checkcount;

		if (!(b->contents & tw->contents))
		{
			continue;
		}

		TestBoxInBrush(tw, b);
		if (tw->trace.allsolid)
		{
			return;
		}
	}

	// test against all patches
	if (!cm_noCurves->integer)
	{
		for (k = 0; k < leaf->numLeafSurfaces; k++)
		{
			patch = surfaces[leafsurfaces[leaf->firstLeafSurface + k]];
			if (!patch)
			{
				continue;
			}
			if (patch->checkcount == checkcount)
			{
				continue;	// already checked this brush in another leaf
			}
			patch->checkcount = checkcount;

			if (!(patch->contents & tw->contents))
			{
				continue;
			}

			if (patch->pc->PositionTest(tw))
			{
				tw->trace.startsolid = tw->trace.allsolid = true;
				tw->trace.fraction = 0;
				if (!(GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET)))
				{
					tw->trace.contents = patch->contents;
				}
				return;
			}
		}
	}
}

//==========================================================================
//
//	QClipMap46::TestBoxInBrush
//
//==========================================================================

void QClipMap46::TestBoxInBrush(traceWork_t* tw, cbrush_t* brush)
{
	int i;
	cplane_t* plane;
	float dist;
	float d1;
	cbrushside_t* side;
	float t;
	vec3_t startp;

	if (!brush->numsides)
	{
		return;
	}

	// special test for axial
	if (tw->bounds[0][0] > brush->bounds[1][0] ||
		tw->bounds[0][1] > brush->bounds[1][1] ||
		tw->bounds[0][2] > brush->bounds[1][2] ||
		tw->bounds[1][0] < brush->bounds[0][0] ||
		tw->bounds[1][1] < brush->bounds[0][1] ||
		tw->bounds[1][2] < brush->bounds[0][2]
		)
	{
		return;
	}

	if (tw->sphere.use)
	{
		// the first six planes are the axial planes, so we only
		// need to test the remainder
		for (i = 6; i < brush->numsides; i++)
		{
			side = brush->sides + i;
			plane = side->plane;

			// adjust the plane distance apropriately for radius
			dist = plane->dist + tw->sphere.radius;
			// find the closest point on the capsule to the plane
			t = DotProduct(plane->normal, tw->sphere.offset);
			if (t > 0)
			{
				VectorSubtract(tw->start, tw->sphere.offset, startp);
			}
			else
			{
				VectorAdd(tw->start, tw->sphere.offset, startp);
			}
			d1 = DotProduct(startp, plane->normal) - dist;
			// if completely in front of face, no intersection
			if (d1 > 0)
			{
				return;
			}
		}
	}
	else
	{
		// the first six planes are the axial planes, so we only
		// need to test the remainder
		for (i = 6; i < brush->numsides; i++)
		{
			side = brush->sides + i;
			plane = side->plane;

			// adjust the plane distance apropriately for mins/maxs
			dist = plane->dist - DotProduct(tw->offsets[plane->signbits], plane->normal);

			d1 = DotProduct(tw->start, plane->normal) - dist;

			// if completely in front of face, no intersection
			if (d1 > 0)
			{
				return;
			}
		}
	}

	// inside this brush
	tw->trace.startsolid = tw->trace.allsolid = true;
	tw->trace.fraction = 0;
	tw->trace.contents = brush->contents;
}

//==========================================================================
//
//	QClipMap46::TestBoundingBoxInCapsule
//
//	bounding box inside capsule check
//
//==========================================================================

void QClipMap46::TestBoundingBoxInCapsule(traceWork_t* tw, clipHandle_t model)
{
	vec3_t mins, maxs, offset, size[2];
	clipHandle_t h;
	cmodel_t* cmod;
	int i;

	// mins maxs of the capsule
	ModelBounds(model, mins, maxs);

	// offset for capsule center
	for (i = 0; i < 3; i++)
	{
		offset[i] = (mins[i] + maxs[i]) * 0.5;
		size[0][i] = mins[i] - offset[i];
		size[1][i] = maxs[i] - offset[i];
		tw->start[i] -= offset[i];
		tw->end[i] -= offset[i];
	}

	// replace the bounding box with the capsule
	tw->sphere.use = true;
	tw->sphere.radius = (size[1][0] > size[1][2]) ? size[1][2] : size[1][0];
	tw->sphere.halfheight = size[1][2];
	VectorSet(tw->sphere.offset, 0, 0, size[1][2] - tw->sphere.radius);

	// replace the capsule with the bounding box
	h = TempBoxModel(tw->size[0], tw->size[1], false);
	// calculate collision
	cmod = ClipHandleToModel(h);
	TestInLeaf(tw, &cmod->leaf);
}

//==========================================================================
//
//	QClipMap46::TestCapsuleInCapsule
//
//	capsule inside capsule check
//
//==========================================================================

void QClipMap46::TestCapsuleInCapsule(traceWork_t* tw, clipHandle_t model)
{
	int i;
	vec3_t mins, maxs;
	vec3_t top, bottom;
	vec3_t p1, p2, tmp;
	vec3_t offset, symetricSize[2];
	float radius, halfwidth, halfheight, offs, r;

	ModelBounds(model, mins, maxs);

	VectorAdd(tw->start, tw->sphere.offset, top);
	VectorSubtract(tw->start, tw->sphere.offset, bottom);
	for (i = 0; i < 3; i++)
	{
		offset[i] = (mins[i] + maxs[i]) * 0.5;
		symetricSize[0][i] = mins[i] - offset[i];
		symetricSize[1][i] = maxs[i] - offset[i];
	}
	halfwidth = symetricSize[1][0];
	halfheight = symetricSize[1][2];
	radius = (halfwidth > halfheight) ? halfheight : halfwidth;
	offs = halfheight - radius;

	r = Square(tw->sphere.radius + radius);
	// check if any of the spheres overlap
	VectorCopy(offset, p1);
	p1[2] += offs;
	VectorSubtract(p1, top, tmp);
	if (VectorLengthSquared(tmp) < r)
	{
		tw->trace.startsolid = tw->trace.allsolid = true;
		tw->trace.fraction = 0;
	}
	VectorSubtract(p1, bottom, tmp);
	if (VectorLengthSquared(tmp) < r)
	{
		tw->trace.startsolid = tw->trace.allsolid = true;
		tw->trace.fraction = 0;
	}
	VectorCopy(offset, p2);
	p2[2] -= offs;
	VectorSubtract(p2, top, tmp);
	if (VectorLengthSquared(tmp) < r)
	{
		tw->trace.startsolid = tw->trace.allsolid = true;
		tw->trace.fraction = 0;
	}
	VectorSubtract(p2, bottom, tmp);
	if (VectorLengthSquared(tmp) < r)
	{
		tw->trace.startsolid = tw->trace.allsolid = true;
		tw->trace.fraction = 0;
	}
	// if between cylinder up and lower bounds
	if ((top[2] >= p1[2] && top[2] <= p2[2]) ||
		(bottom[2] >= p1[2] && bottom[2] <= p2[2]))
	{
		// 2d coordinates
		top[2] = p1[2] = 0;
		// if the cylinders overlap
		VectorSubtract(top, p1, tmp);
		if (VectorLengthSquared(tmp) < r)
		{
			tw->trace.startsolid = tw->trace.allsolid = true;
			tw->trace.fraction = 0;
		}
	}
}

/*
===============================================================================

BASIC MATH

===============================================================================
*/

//==========================================================================
//
//	QClipMap46::SquareRootFloat
//
//==========================================================================

float QClipMap46::SquareRootFloat(float number)
{
	int i;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	y  = number;
	i  = *(int*)&y;
	i  = 0x5f3759df - (i >> 1);
	y  = *(float*)&i;
	y  = y * (threehalfs - (x2 * y * y));
	y  = y * (threehalfs - (x2 * y * y));
	return number * y;
}

//==========================================================================
//
//  QClipMap46::HullCheckQ1
//
//==========================================================================

bool QClipMap46::HullCheckQ1(clipHandle_t Handle, vec3_t p1, vec3_t p2, q1trace_t* trace)
{
	throw DropException("Not implemented");
}

//==========================================================================
//
//	QClipMap46::BoxTraceQ2
//
//==========================================================================

q2trace_t QClipMap46::BoxTraceQ2(vec3_t Start, vec3_t End,
	vec3_t Mins, vec3_t Maxs, clipHandle_t Model, int BrushMask)
{
	throw DropException("Not implemented");
}

//==========================================================================
//
//	QClipMap46::TransformedBoxTraceQ2
//
//==========================================================================

q2trace_t QClipMap46::TransformedBoxTraceQ2(vec3_t Start, vec3_t End,
	vec3_t Mins, vec3_t Maxs, clipHandle_t Model, int BrushMask, vec3_t Origin, vec3_t Angles)
{
	throw DropException("Not implemented");
}
