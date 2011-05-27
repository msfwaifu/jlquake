//**************************************************************************
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

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "render_local.h"

// MACROS ------------------------------------------------------------------

#define ANIM_CYCLE		2

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

mbrush29_texture_t*		r_notexture_mip;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	Mod_LoadTextures
//
//==========================================================================

static void Mod_LoadTextures(bsp29_lump_t* l)
{
	if (!l->filelen)
	{
		loadmodel->brush29_textures = NULL;
		return;
	}
	bsp29_dmiptexlump_t* m = (bsp29_dmiptexlump_t*)(mod_base + l->fileofs);

	m->nummiptex = LittleLong(m->nummiptex);

	loadmodel->brush29_numtextures = m->nummiptex;
	loadmodel->brush29_textures = new mbrush29_texture_t*[m->nummiptex];
	Com_Memset(loadmodel->brush29_textures, 0, sizeof(mbrush29_texture_t*) * m->nummiptex);

	for (int i = 0; i < m->nummiptex; i++)
	{
		m->dataofs[i] = LittleLong(m->dataofs[i]);
		if (m->dataofs[i] == -1)
		{
			continue;
		}
		bsp29_miptex_t* mt = (bsp29_miptex_t*)((byte*)m + m->dataofs[i]);
		mt->width = LittleLong(mt->width);
		mt->height = LittleLong(mt->height);
		for (int j = 0; j < BSP29_MIPLEVELS; j++)
		{
			mt->offsets[j] = LittleLong (mt->offsets[j]);
		}
		
		if ((mt->width & 15) || (mt->height & 15))
		{
			throw QException(va("Texture %s is not 16 aligned", mt->name));
		}
		int pixels = mt->width * mt->height / 64 * 85;
		mbrush29_texture_t* tx = (mbrush29_texture_t*)Mem_Alloc(sizeof(mbrush29_texture_t) + pixels);
		Com_Memset(tx, 0, sizeof(mbrush29_texture_t) + pixels);
		loadmodel->brush29_textures[i] = tx;

		Com_Memcpy(tx->name, mt->name, sizeof(tx->name));
		tx->width = mt->width;
		tx->height = mt->height;
		for (int j = 0; j < BSP29_MIPLEVELS; j++)
		{
			tx->offsets[j] = mt->offsets[j] + sizeof(mbrush29_texture_t) - sizeof(bsp29_miptex_t);
		}
		// the pixels immediately follow the structures
		Com_Memcpy(tx + 1, mt + 1, pixels);

		if (!QStr::NCmp(mt->name,"sky",3))
		{
			R_InitSky(tx);
		}
		else
		{
			byte* pic32 = R_ConvertImage8To32((byte*)(tx + 1), tx->width, tx->height, IMG8MODE_Normal);
			tx->gl_texture = GL_LoadTexture(mt->name, tx->width, tx->height, pic32, true);
			delete[] pic32;
		}
	}

	//
	// sequence the animations
	//
	for (int i = 0; i < m->nummiptex; i++)
	{
		mbrush29_texture_t* tx = loadmodel->brush29_textures[i];
		if (!tx || tx->name[0] != '+')
		{
			continue;
		}
		if (tx->anim_next)
		{
			continue;	// allready sequenced
		}

		// find the number of frames in the animation
		mbrush29_texture_t* anims[10];
		mbrush29_texture_t* altanims[10];
		Com_Memset(anims, 0, sizeof(anims));
		Com_Memset(altanims, 0, sizeof(altanims));

		int max = tx->name[1];
		int altmax = 0;
		if (max >= 'a' && max <= 'z')
		{
			max -= 'a' - 'A';
		}
		if (max >= '0' && max <= '9')
		{
			max -= '0';
			altmax = 0;
			anims[max] = tx;
			max++;
		}
		else if (max >= 'A' && max <= 'J')
		{
			altmax = max - 'A';
			max = 0;
			altanims[altmax] = tx;
			altmax++;
		}
		else
		{
			throw QException(va("Bad animating texture %s", tx->name));
		}

		for (int j = i + 1; j < m->nummiptex; j++)
		{
			mbrush29_texture_t* tx2 = loadmodel->brush29_textures[j];
			if (!tx2 || tx2->name[0] != '+')
			{
				continue;
			}
			if (QStr::Cmp(tx2->name + 2, tx->name + 2))
			{
				continue;
			}

			int num = tx2->name[1];
			if (num >= 'a' && num <= 'z')
			{
				num -= 'a' - 'A';
			}
			if (num >= '0' && num <= '9')
			{
				num -= '0';
				anims[num] = tx2;
				if (num + 1 > max)
				{
					max = num + 1;
				}
			}
			else if (num >= 'A' && num <= 'J')
			{
				num = num - 'A';
				altanims[num] = tx2;
				if (num + 1 > altmax)
				{
					altmax = num + 1;
				}
			}
			else
			{
				throw QException(va("Bad animating texture %s", tx->name));
			}
		}
		
		// link them all together
		for (int j = 0; j < max; j++)
		{
			mbrush29_texture_t* tx2 = anims[j];
			if (!tx2)
			{
				throw QException(va("Missing frame %i of %s", j, tx->name));
			}
			tx2->anim_total = max * ANIM_CYCLE;
			tx2->anim_min = j * ANIM_CYCLE;
			tx2->anim_max = (j + 1) * ANIM_CYCLE;
			tx2->anim_next = anims[(j + 1) % max];
			if (altmax)
			{
				tx2->alternate_anims = altanims[0];
			}
		}
		for (int j = 0; j < altmax; j++)
		{
			mbrush29_texture_t* tx2 = altanims[j];
			if (!tx2)
			{
				throw QException(va("Missing frame %i of %s", j, tx->name));
			}
			tx2->anim_total = altmax * ANIM_CYCLE;
			tx2->anim_min = j * ANIM_CYCLE;
			tx2->anim_max = (j + 1) * ANIM_CYCLE;
			tx2->anim_next = altanims[(j + 1) % altmax];
			if (max)
			{
				tx2->alternate_anims = anims[0];
			}
		}
	}
}

//==========================================================================
//
//	Mod_LoadLighting
//
//==========================================================================

static void Mod_LoadLighting(bsp29_lump_t* l)
{
	if (!l->filelen)
	{
		loadmodel->brush29_lightdata = NULL;
		return;
	}
	loadmodel->brush29_lightdata = new byte[l->filelen];
	Com_Memcpy(loadmodel->brush29_lightdata, mod_base + l->fileofs, l->filelen);
}

//==========================================================================
//
//	Mod_LoadVisibility
//
//==========================================================================

static void Mod_LoadVisibility(bsp29_lump_t* l)
{
	if (!l->filelen)
	{
		loadmodel->brush29_visdata = NULL;
		return;
	}
	loadmodel->brush29_visdata = new byte[l->filelen];
	Com_Memcpy(loadmodel->brush29_visdata, mod_base + l->fileofs, l->filelen);
}

//==========================================================================
//
//	Mod_LoadEntities
//
//==========================================================================

static void Mod_LoadEntities(bsp29_lump_t* l)
{
	if (!l->filelen)
	{
		loadmodel->brush29_entities = NULL;
		return;
	}
	loadmodel->brush29_entities = new char[l->filelen];
	Com_Memcpy(loadmodel->brush29_entities, mod_base + l->fileofs, l->filelen);
}

//==========================================================================
//
//	Mod_LoadVertexes
//
//==========================================================================

static void Mod_LoadVertexes(bsp29_lump_t* l)
{
	bsp29_dvertex_t* in = (bsp29_dvertex_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QException(va("MOD_LoadBmodel: funny lump size in %s", loadmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	mbrush29_vertex_t* out = new mbrush29_vertex_t[count];

	loadmodel->brush29_vertexes = out;
	loadmodel->brush29_numvertexes = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		out->position[0] = LittleFloat(in->point[0]);
		out->position[1] = LittleFloat(in->point[1]);
		out->position[2] = LittleFloat(in->point[2]);
	}
}

//==========================================================================
//
//	Mod_LoadEdges
//
//==========================================================================

static void Mod_LoadEdges(bsp29_lump_t* l)
{
	bsp29_dedge_t* in = (bsp29_dedge_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QException(va("MOD_LoadBmodel: funny lump size in %s",loadmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	mbrush29_edge_t* out = new mbrush29_edge_t[count + 1];
	Com_Memset(out, 0, sizeof(mbrush29_edge_t) * (count + 1));

	loadmodel->brush29_edges = out;
	loadmodel->brush29_numedges = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		out->v[0] = (unsigned short)LittleShort(in->v[0]);
		out->v[1] = (unsigned short)LittleShort(in->v[1]);
	}
}

//==========================================================================
//
//	Mod_LoadTexinfo
//
//==========================================================================

static void Mod_LoadTexinfo(bsp29_lump_t* l)
{
	bsp29_texinfo_t* in = (bsp29_texinfo_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QException(va("MOD_LoadBmodel: funny lump size in %s", loadmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	mbrush29_texinfo_t* out = new mbrush29_texinfo_t[count];
	Com_Memset(out, 0, sizeof(mbrush29_texinfo_t) * count);

	loadmodel->brush29_texinfo = out;
	loadmodel->brush29_numtexinfo = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		for (int j = 0; j < 8; j++)
		{
			out->vecs[0][j] = LittleFloat(in->vecs[0][j]);
		}
		float len1 = VectorLength(out->vecs[0]);
		float len2 = VectorLength(out->vecs[1]);
		len1 = (len1 + len2) / 2;
		if (len1 < 0.32)
		{
			out->mipadjust = 4;
		}
		else if (len1 < 0.49)
		{
			out->mipadjust = 3;
		}
		else if (len1 < 0.99)
		{
			out->mipadjust = 2;
		}
		else
		{
			out->mipadjust = 1;
		}

		int miptex = LittleLong(in->miptex);
		out->flags = LittleLong(in->flags);
	
		if (!loadmodel->brush29_textures)
		{
			out->texture = r_notexture_mip;	// checkerboard texture
			out->flags = 0;
		}
		else
		{
			if (miptex >= loadmodel->brush29_numtextures)
			{
				throw QException("miptex >= loadmodel->numtextures");
			}
			out->texture = loadmodel->brush29_textures[miptex];
			if (!out->texture)
			{
				out->texture = r_notexture_mip; // texture not found
				out->flags = 0;
			}
		}
	}
}

//==========================================================================
//
//	CalcSurfaceExtents
//
//	Fills in s->texturemins[] and s->extents[]
//
//==========================================================================

static void CalcSurfaceExtents(mbrush29_surface_t* s)
{
	float mins[2], maxs[2];
	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	mbrush29_texinfo_t* tex = s->texinfo;

	for (int i = 0; i < s->numedges; i++)
	{
		int e = loadmodel->brush29_surfedges[s->firstedge + i];
		mbrush29_vertex_t* v;
		if (e >= 0)
		{
			v = &loadmodel->brush29_vertexes[loadmodel->brush29_edges[e].v[0]];
		}
		else
		{
			v = &loadmodel->brush29_vertexes[loadmodel->brush29_edges[-e].v[1]];
		}

		for (int j = 0; j < 2; j++)
		{
			float val = v->position[0] * tex->vecs[j][0] + 
				v->position[1] * tex->vecs[j][1] +
				v->position[2] * tex->vecs[j][2] +
				tex->vecs[j][3];
			if (val < mins[j])
			{
				mins[j] = val;
			}
			if (val > maxs[j])
			{
				maxs[j] = val;
			}
		}
	}

	int bmins[2], bmaxs[2];
	for (int i = 0; i < 2; i++)
	{	
		bmins[i] = floor(mins[i] / 16);
		bmaxs[i] = ceil(maxs[i] / 16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;
		if (!(tex->flags & BSP29TEX_SPECIAL) && s->extents[i] > 512)
		{
			throw QException("Bad surface extents");
		}
	}
}

//==========================================================================
//
//	Mod_LoadFaces
//
//==========================================================================

static void Mod_LoadFaces(bsp29_lump_t* l)
{
	bsp29_dface_t* in = (bsp29_dface_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QException(va("MOD_LoadBmodel: funny lump size in %s", loadmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	mbrush29_surface_t* out = new mbrush29_surface_t[count];
	Com_Memset(out, 0, sizeof(mbrush29_surface_t) * count);

	loadmodel->brush29_surfaces = out;
	loadmodel->brush29_numsurfaces = count;

	for (int surfnum = 0; surfnum < count; surfnum++, in++, out++)
	{
		out->firstedge = LittleLong(in->firstedge);
		out->numedges = LittleShort(in->numedges);		
		out->flags = 0;

		int planenum = LittleShort(in->planenum);
		int side = LittleShort(in->side);
		if (side)
		{
			out->flags |= BRUSH29_SURF_PLANEBACK;
		}

		out->plane = loadmodel->brush29_planes + planenum;

		out->texinfo = loadmodel->brush29_texinfo + LittleShort(in->texinfo);

		CalcSurfaceExtents(out);

		// lighting info

		for (int i = 0; i < BSP29_MAXLIGHTMAPS; i++)
		{
			out->styles[i] = in->styles[i];
		}
		int lightofs = LittleLong(in->lightofs);
		if (lightofs == -1)
		{
			out->samples = NULL;
		}
		else
		{
			out->samples = loadmodel->brush29_lightdata + lightofs;
		}

		// set the drawing flags flag

		if (!QStr::NCmp(out->texinfo->texture->name, "sky", 3))	// sky
		{
			out->flags |= (BRUSH29_SURF_DRAWSKY | BRUSH29_SURF_DRAWTILED);
			GL_SubdivideSurface(out);	// cut up polygon for warps
			continue;
		}
		
		if (out->texinfo->texture->name[0] == '*')		// turbulent
		{
			out->flags |= (BRUSH29_SURF_DRAWTURB | BRUSH29_SURF_DRAWTILED);
			for (int i = 0; i < 2; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
			GL_SubdivideSurface(out);	// cut up polygon for warps

			if ((GGameType & GAME_Hexen2) &&
				(!QStr::NICmp(out->texinfo->texture->name, "*rtex078", 8) ||
				!QStr::NICmp(out->texinfo->texture->name, "*lowlight", 9)))
			{
				out->flags |= BRUSH29_SURF_TRANSLUCENT;
			}

			continue;
		}
	}
}

//==========================================================================
//
//	Mod_SetParent
//
//==========================================================================

static void Mod_SetParent(mbrush29_node_t* node, mbrush29_node_t* parent)
{
	node->parent = parent;
	if (node->contents < 0)
	{
		return;
	}
	Mod_SetParent(node->children[0], node);
	Mod_SetParent(node->children[1], node);
}

//==========================================================================
//
//	Mod_LoadNodes
//
//==========================================================================

static void Mod_LoadNodes(bsp29_lump_t* l)
{
	bsp29_dnode_t* in = (bsp29_dnode_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QException(va("MOD_LoadBmodel: funny lump size in %s", loadmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	mbrush29_node_t* out = new mbrush29_node_t[count];
	Com_Memset(out, 0, sizeof(mbrush29_node_t) * count);

	loadmodel->brush29_nodes = out;
	loadmodel->brush29_numnodes = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		for (int j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleShort(in->mins[j]);
			out->minmaxs[3+j] = LittleShort(in->maxs[j]);
		}

		int p = LittleLong(in->planenum);
		out->plane = loadmodel->brush29_planes + p;

		out->firstsurface = LittleShort(in->firstface);
		out->numsurfaces = LittleShort(in->numfaces);
		
		for (int j = 0; j < 2; j++)
		{
			p = LittleShort(in->children[j]);
			if (p >= 0)
			{
				out->children[j] = loadmodel->brush29_nodes + p;
			}
			else
			{
				out->children[j] = (mbrush29_node_t *)(loadmodel->brush29_leafs + (-1 - p));
			}
		}
	}

	Mod_SetParent(loadmodel->brush29_nodes, NULL);	// sets nodes and leafs
}

//==========================================================================
//
//	Mod_LoadLeafs
//
//==========================================================================

static void Mod_LoadLeafs(bsp29_lump_t* l)
{
	bsp29_dleaf_t* in = (bsp29_dleaf_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QException(va("MOD_LoadBmodel: funny lump size in %s", loadmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	mbrush29_leaf_t* out = new mbrush29_leaf_t[count];
	Com_Memset(out, 0, sizeof(mbrush29_leaf_t) * count);

	loadmodel->brush29_leafs = out;
	loadmodel->brush29_numleafs = count;

	//char s[80];
	//sprintf(s, "maps/%s.bsp", Info_ValueForKey(cl.serverinfo,"map"));
	bool isnotmap = true;
	//if (!QStr::Cmp(s, loadmodel->name))
	{
		isnotmap = false;
	}

	for (int i = 0; i < count; i++, in++, out++)
	{
		for (int j = 0; j < 3; j++)
		{
			out->minmaxs[j] = LittleShort(in->mins[j]);
			out->minmaxs[3 + j] = LittleShort(in->maxs[j]);
		}

		int p = LittleLong(in->contents);
		out->contents = p;

		out->firstmarksurface = loadmodel->brush29_marksurfaces +
			(quint16)LittleShort(in->firstmarksurface);
		out->nummarksurfaces = LittleShort(in->nummarksurfaces);
		
		p = LittleLong(in->visofs);
		if (p == -1)
		{
			out->compressed_vis = NULL;
		}
		else
		{
			out->compressed_vis = loadmodel->brush29_visdata + p;
		}
		
		for (int j = 0; j < 4; j++)
		{
			out->ambient_sound_level[j] = in->ambient_level[j];
		}

		// gl underwater warp
		if (out->contents != BSP29CONTENTS_EMPTY)
		{
			for (int j = 0; j < out->nummarksurfaces; j++)
			{
				out->firstmarksurface[j]->flags |= BRUSH29_SURF_UNDERWATER;
			}
		}
		if (isnotmap)
		{
			for (int j = 0; j < out->nummarksurfaces; j++)
			{
				out->firstmarksurface[j]->flags |= BRUSH29_SURF_DONTWARP;
			}
		}
	}	
}

//==========================================================================
//
//	Mod_LoadClipnodes
//
//==========================================================================

static void Mod_LoadClipnodes(bsp29_lump_t* l)
{
	bsp29_dclipnode_t* in = (bsp29_dclipnode_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QException(va("MOD_LoadBmodel: funny lump size in %s", loadmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	bsp29_dclipnode_t* out = new bsp29_dclipnode_t[count];

	loadmodel->brush29_clipnodes = out;
	loadmodel->brush29_numclipnodes = count;

	mbrush29_hull_t* hull = &loadmodel->brush29_hulls[1];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->brush29_planes;
	hull->clip_mins[0] = -16;
	hull->clip_mins[1] = -16;
	hull->clip_mins[2] = -24;
	hull->clip_maxs[0] = 16;
	hull->clip_maxs[1] = 16;
	hull->clip_maxs[2] = 32;

	if (GGameType & GAME_Quake)
	{
		hull = &loadmodel->brush29_hulls[2];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->brush29_planes;
		hull->clip_mins[0] = -32;
		hull->clip_mins[1] = -32;
		hull->clip_mins[2] = -24;
		hull->clip_maxs[0] = 32;
		hull->clip_maxs[1] = 32;
		hull->clip_maxs[2] = 64;
	}
	else
	{
		hull = &loadmodel->brush29_hulls[2];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->brush29_planes;
		hull->clip_mins[0] = -24;
		hull->clip_mins[1] = -24;
		hull->clip_mins[2] = -20;
		hull->clip_maxs[0] = 24;
		hull->clip_maxs[1] = 24;
		hull->clip_maxs[2] = 20;

		hull = &loadmodel->brush29_hulls[3];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->brush29_planes;
		hull->clip_mins[0] = -16;
		hull->clip_mins[1] = -16;
		hull->clip_mins[2] = -12;
		hull->clip_maxs[0] = 16;
		hull->clip_maxs[1] = 16;
		hull->clip_maxs[2] = 16;

		hull = &loadmodel->brush29_hulls[4];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->brush29_planes;
		hull->clip_mins[0] = -40;
		hull->clip_mins[1] = -40;
		hull->clip_mins[2] = -42;
		hull->clip_maxs[0] = 40;
		hull->clip_maxs[1] = 40;
		hull->clip_maxs[2] = 42;

		hull = &loadmodel->brush29_hulls[5];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->brush29_planes;
		hull->clip_mins[0] = -48;
		hull->clip_mins[1] = -48;
		hull->clip_mins[2] = -50;
		hull->clip_maxs[0] = 48;
		hull->clip_maxs[1] = 48;
		hull->clip_maxs[2] = 50;
	}

	for (int i = 0; i < count; i++, out++, in++)
	{
		out->planenum = LittleLong(in->planenum);
		out->children[0] = LittleShort(in->children[0]);
		out->children[1] = LittleShort(in->children[1]);
	}
}

//==========================================================================
//
//	Mod_MakeHull0
//
//	Deplicate the drawing hull structure as a clipping hull
//
//==========================================================================

static void Mod_MakeHull0()
{
	mbrush29_hull_t* hull = &loadmodel->brush29_hulls[0];	

	mbrush29_node_t* in = loadmodel->brush29_nodes;
	int count = loadmodel->brush29_numnodes;
	bsp29_dclipnode_t* out = new bsp29_dclipnode_t[count];

	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->brush29_planes;

	for (int i = 0; i < count; i++, out++, in++)
	{
		out->planenum = in->plane - loadmodel->brush29_planes;
		for (int j = 0; j < 2; j++)
		{
			mbrush29_node_t* child = in->children[j];
			if (child->contents < 0)
			{
				out->children[j] = child->contents;
			}
			else
			{
				out->children[j] = child - loadmodel->brush29_nodes;
			}
		}
	}
}

//==========================================================================
//
//	Mod_LoadMarksurfaces
//
//==========================================================================

static void Mod_LoadMarksurfaces(bsp29_lump_t* l)
{	
	short* in = (short*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QException(va("MOD_LoadBmodel: funny lump size in %s", loadmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	mbrush29_surface_t** out = new mbrush29_surface_t*[count];

	loadmodel->brush29_marksurfaces = out;
	loadmodel->brush29_nummarksurfaces = count;

	for (int i = 0; i < count; i++)
	{
		int j = (quint16)LittleShort(in[i]);
		if (j >= loadmodel->brush29_numsurfaces)
		{
			throw QException("Mod_ParseMarksurfaces: bad surface number");
		}
		out[i] = loadmodel->brush29_surfaces + j;
	}
}

//==========================================================================
//
//	Mod_LoadSurfedges
//
//==========================================================================

static void Mod_LoadSurfedges(bsp29_lump_t* l)
{
	int* in = (int*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QException(va("MOD_LoadBmodel: funny lump size in %s", loadmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	int* out = new int[count];

	loadmodel->brush29_surfedges = out;
	loadmodel->brush29_numsurfedges = count;

	for (int i = 0; i < count; i++)
	{
		out[i] = LittleLong(in[i]);
	}
}

//==========================================================================
//
//	Mod_FreeBsp29
//
//==========================================================================

static void Mod_LoadPlanes(bsp29_lump_t* l)
{
	bsp29_dplane_t* in = (bsp29_dplane_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QException(va("MOD_LoadBmodel: funny lump size in %s", loadmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	cplane_t* out = new cplane_t[count * 2];
	Com_Memset(out, 0, sizeof(cplane_t) * (count * 2));

	loadmodel->brush29_planes = out;
	loadmodel->brush29_numplanes = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		for (int j = 0; j < 3; j++)
		{
			out->normal[j] = LittleFloat(in->normal[j]);
		}
		out->dist = LittleFloat(in->dist);
		out->type = LittleLong(in->type);

		SetPlaneSignbits(out);
	}
}

//==========================================================================
//
//	Mod_LoadSubmodelsQ1
//
//==========================================================================

static void Mod_LoadSubmodelsQ1(bsp29_lump_t* l)
{
	bsp29_dmodel_q1_t* in = (bsp29_dmodel_q1_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QException(va("MOD_LoadBmodel: funny lump size in %s", loadmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	bsp29_dmodel_q1_t* out = new bsp29_dmodel_q1_t[count];
	Com_Memset(out, 0, sizeof(bsp29_dmodel_q1_t) * count);

	loadmodel->brush29_submodels_q1 = out;
	loadmodel->brush29_numsubmodels = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		for (int j = 0; j < 3; j++)
		{
			// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat(in->mins[j]) - 1;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
			out->origin[j] = LittleFloat(in->origin[j]);
		}
		for (int j = 0; j < BSP29_MAX_MAP_HULLS_Q1; j++)
		{
			out->headnode[j] = LittleLong(in->headnode[j]);
		}
		out->visleafs = LittleLong(in->visleafs);
		out->firstface = LittleLong(in->firstface);
		out->numfaces = LittleLong(in->numfaces);
	}
}

//==========================================================================
//
//	Mod_LoadSubmodelsH2
//
//==========================================================================

static void Mod_LoadSubmodelsH2(bsp29_lump_t* l)
{
	bsp29_dmodel_h2_t* in = (bsp29_dmodel_h2_t*)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		throw QException(va("MOD_LoadBmodel: funny lump size in %s", loadmodel->name));
	}
	int count = l->filelen / sizeof(*in);
	bsp29_dmodel_h2_t* out = new bsp29_dmodel_h2_t[count];
	Com_Memset(out, 0, sizeof(bsp29_dmodel_h2_t) * count);

	loadmodel->brush29_submodels_h2 = out;
	loadmodel->brush29_numsubmodels = count;

	for (int i = 0; i < count; i++, in++, out++)
	{
		for (int j = 0; j < 3; j++)
		{
			// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat(in->mins[j]) - 1;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
			out->origin[j] = LittleFloat(in->origin[j]);
		}
		for (int j = 0; j < BSP29_MAX_MAP_HULLS_H2; j++)
		{
			out->headnode[j] = LittleLong(in->headnode[j]);
		}
		out->visleafs = LittleLong(in->visleafs);
		out->firstface = LittleLong(in->firstface);
		out->numfaces = LittleLong(in->numfaces);
	}
}

//==========================================================================
//
//	Mod_FreeBsp29
//
//==========================================================================

void Mod_LoadBrush29Model(model_t* mod, void* buffer)
{
	loadmodel->type = MOD_BRUSH29;

	bsp29_dheader_t* header = (bsp29_dheader_t *)buffer;

	int version = LittleLong(header->version);
	if (version != BSP29_VERSION)
	{
		throw QException(va("Mod_LoadBrush29Model: %s has wrong version number (%i should be %i)", mod->name, version, BSP29_VERSION));
	}

	// swap all the lumps
	mod_base = (byte *)header;

	for (int i = 0; i < (int)sizeof(bsp29_dheader_t) / 4; i++)
	{
		((int*)header)[i] = LittleLong(((int*)header)[i]);
	}

	// load into heap

	Mod_LoadVertexes(&header->lumps[BSP29LUMP_VERTEXES]);
	Mod_LoadEdges(&header->lumps[BSP29LUMP_EDGES]);
	Mod_LoadSurfedges(&header->lumps[BSP29LUMP_SURFEDGES]);
	Mod_LoadTextures(&header->lumps[BSP29LUMP_TEXTURES]);
	Mod_LoadLighting(&header->lumps[BSP29LUMP_LIGHTING]);
	Mod_LoadPlanes(&header->lumps[BSP29LUMP_PLANES]);
	Mod_LoadTexinfo(&header->lumps[BSP29LUMP_TEXINFO]);
	Mod_LoadFaces(&header->lumps[BSP29LUMP_FACES]);
	Mod_LoadMarksurfaces(&header->lumps[BSP29LUMP_MARKSURFACES]);
	Mod_LoadVisibility(&header->lumps[BSP29LUMP_VISIBILITY]);
	Mod_LoadLeafs(&header->lumps[BSP29LUMP_LEAFS]);
	Mod_LoadNodes(&header->lumps[BSP29LUMP_NODES]);
	Mod_LoadClipnodes(&header->lumps[BSP29LUMP_CLIPNODES]);
	Mod_LoadEntities(&header->lumps[BSP29LUMP_ENTITIES]);
	if (GGameType & GAME_Hexen2)
	{
		Mod_LoadSubmodelsH2(&header->lumps[BSP29LUMP_MODELS]);
	}
	else
	{
		Mod_LoadSubmodelsQ1(&header->lumps[BSP29LUMP_MODELS]);
	}

	Mod_MakeHull0();

	mod->q1_numframes = 2;		// regular and alternate animation

	//
	// set up the submodels (FIXME: this is confusing)
	//
	if (GGameType & GAME_Hexen2)
	{
		for (int i = 0; i < mod->brush29_numsubmodels; i++)
		{
			bsp29_dmodel_h2_t* bm = &mod->brush29_submodels_h2[i];

			mod->brush29_hulls[0].firstclipnode = bm->headnode[0];
			for (int j = 1; j < BSP29_MAX_MAP_HULLS_H2; j++)
			{
				mod->brush29_hulls[j].firstclipnode = bm->headnode[j];
				mod->brush29_hulls[j].lastclipnode = mod->brush29_numclipnodes-1;
			}
			
			mod->brush29_firstmodelsurface = bm->firstface;
			mod->brush29_nummodelsurfaces = bm->numfaces;
			
			VectorCopy (bm->maxs, mod->q1_maxs);
			VectorCopy (bm->mins, mod->q1_mins);

			mod->q1_radius = RadiusFromBounds (mod->q1_mins, mod->q1_maxs);

			mod->brush29_numleafs = bm->visleafs;

			if (i < mod->brush29_numsubmodels-1)
			{
				// duplicate the basic information
				char	name[10];

				sprintf (name, "*%i", i+1);
				loadmodel = Mod_FindName(name);
				*loadmodel = *mod;
				QStr::Cpy(loadmodel->name, name);
				mod = loadmodel;
			}
		}
	}
	else
	{
		for (int i = 0; i < mod->brush29_numsubmodels; i++)
		{
			bsp29_dmodel_q1_t* bm = &mod->brush29_submodels_q1[i];

			mod->brush29_hulls[0].firstclipnode = bm->headnode[0];
			for (int j = 1; j < BSP29_MAX_MAP_HULLS_Q1; j++)
			{
				mod->brush29_hulls[j].firstclipnode = bm->headnode[j];
				mod->brush29_hulls[j].lastclipnode = mod->brush29_numclipnodes-1;
			}
			
			mod->brush29_firstmodelsurface = bm->firstface;
			mod->brush29_nummodelsurfaces = bm->numfaces;
			
			VectorCopy (bm->maxs, mod->q1_maxs);
			VectorCopy (bm->mins, mod->q1_mins);

			mod->q1_radius = RadiusFromBounds (mod->q1_mins, mod->q1_maxs);

			mod->brush29_numleafs = bm->visleafs;

			if (i < mod->brush29_numsubmodels-1)
			{
				// duplicate the basic information
				char	name[10];

				sprintf (name, "*%i", i+1);
				loadmodel = Mod_FindName (name);
				*loadmodel = *mod;
				QStr::Cpy(loadmodel->name, name);
				mod = loadmodel;
			}
		}
	}
}

//==========================================================================
//
//	Mod_FreeBsp29
//
//==========================================================================

void Mod_FreeBsp29(model_t* mod)
{
	if (mod->brush29_textures)
	{
		for (int i = 0; i < mod->brush29_numtextures; i++)
		{
			if (mod->brush29_textures[i])
			{
				Mem_Free(mod->brush29_textures[i]);
			}
		}
		delete[] mod->brush29_textures;
	}
	if (mod->brush29_lightdata)
	{
		delete[] mod->brush29_lightdata;
	}
	if (mod->brush29_visdata)
	{
		delete[] mod->brush29_visdata;
	}
	if (mod->brush29_entities)
	{
		delete[] mod->brush29_entities;
	}
	delete[] mod->brush29_vertexes;
	delete[] mod->brush29_edges;
	delete[] mod->brush29_texinfo;
	delete[] mod->brush29_surfaces;
	delete[] mod->brush29_nodes;
	delete[] mod->brush29_leafs;
	delete[] mod->brush29_clipnodes;
	delete[] mod->brush29_hulls[0].clipnodes;
	delete[] mod->brush29_marksurfaces;
	delete[] mod->brush29_surfedges;
	delete[] mod->brush29_planes;
	if (mod->brush29_submodels_q1)
	{
		delete[] mod->brush29_submodels_q1;
	}
	if (mod->brush29_submodels_h2)
	{
		delete[] mod->brush29_submodels_h2;
	}
}
