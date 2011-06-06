/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifdef _WIN32
// disable data conversion warnings

#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA
#endif

#include "../client/render_local.h"
#include "gl_model.h"

#include <GL/glu.h>

void GL_BeginRendering (int *x, int *y, int *width, int *height);
void GL_EndRendering (void);


extern	float	gldepthmin, gldepthmax;

extern	int glx, gly, glwidth, glheight;

// r_local.h -- private refresh defs

#define TILE_SIZE		128		// size of textures generated by R_GenTiledSurf

#define SKYSHIFT		7
#define	SKYSIZE			(1 << SKYSHIFT)
#define SKYMASK			(SKYSIZE - 1)

#define BACKFACE_EPSILON	0.01


void R_TimeRefresh_f (void);
void R_ReadPointFile_f (void);
mbrush29_texture_t *R_TextureAnimation (mbrush29_texture_t *base);

typedef enum {
	pt_static, pt_grav, pt_slowgrav, pt_fire, pt_explode, pt_explode2, pt_blob, pt_blob2
} ptype_t;

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
typedef struct particle_s
{
// driver-usable fields
	vec3_t		org;
	float		color;
// drivers never touch the following fields
	struct particle_s	*next;
	vec3_t		vel;
	float		ramp;
	float		die;
	ptype_t		type;
} cparticle_t;


//====================================================


extern	qboolean	r_cache_thrash;		// compatability
extern	vec3_t		modelorg, r_entorigin;
extern	cplane_t	frustum[4];
extern	int		c_brush_polys, c_alias_polys;


//
// screen size info
//
extern	mbrush29_leaf_t		*r_viewleaf, *r_oldviewleaf;
extern	int		d_lightstylevalue[256];	// 8.8 fraction of base light value

extern	qboolean	envmap;
extern	image_t*	particletexture;
extern	image_t*	playertextures[16];

extern	int	skytexturenum;		// index in cl.loadmodel, not gl texture object

#define			MAX_VISEDICTS	256
extern	int				cl_numvisedicts;
extern trRefEntity_t	cl_visedicts[MAX_VISEDICTS];

extern	QCvar*	r_norefresh;
extern	QCvar*	r_drawentities;
extern	QCvar*	r_drawviewmodel;
extern	QCvar*	r_speeds;
extern	QCvar*	r_shadows;
extern	QCvar*	r_mirroralpha;
extern	QCvar*	r_dynamic;
extern	QCvar*	r_novis;

extern	QCvar*	gl_clear;
extern	QCvar*	gl_cull;
extern	QCvar*	gl_texsort;
extern	QCvar*	gl_smoothmodels;
extern	QCvar*	gl_affinemodels;
extern	QCvar*	gl_polyblend;
extern	QCvar*	gl_nocolors;
extern	QCvar*	gl_keeptjunctions;
extern	QCvar*	gl_reporttjunctions;

extern	int			mirrortexturenum;	// quake texturenum, not gltexturenum
extern	qboolean	mirror;
extern	cplane_t	*mirror_plane;

extern	float	r_world_matrix[16];

void GL_DisableMultitexture(void);
void GL_EnableMultitexture(void);

void GL_Set2D (void);
qboolean R_CullBox (vec3_t mins, vec3_t maxs);
void R_MarkLights (cdlight_t *light, int bit, mbrush29_node_t *node);
void R_DrawSkyChain (mbrush29_surface_t *s);
void EmitBothSkyLayers (mbrush29_surface_t *fa);
void EmitWaterPolys (mbrush29_surface_t *fa);
void R_RotateForEntity (trRefEntity_t *e);
void EmitSkyPolys (mbrush29_surface_t *fa);
void R_InitParticles (void);
void R_ClearParticles (void);
void GL_BuildLightmaps (void);
int R_LightPoint (vec3_t p);
void R_DrawBrushModel (trRefEntity_t *e);
void R_AnimateLight (void);
void R_DrawWorld (void);
void R_DrawParticles (void);
void V_CalcBlend (void);
void R_DrawWaterSurfaces (void);
void R_RenderBrushPoly (mbrush29_surface_t *fa);
void R_InitSky (mbrush29_texture_t *mt);	// called at level load
