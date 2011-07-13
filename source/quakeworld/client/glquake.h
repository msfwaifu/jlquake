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
#include "../../client/render_local.h"
#include "gl_model.h"

#include <GL/glu.h>

void GL_EndRendering (void);


// r_local.h -- private refresh defs

#define TILE_SIZE		128		// size of textures generated by R_GenTiledSurf

#define SKYSHIFT		7
#define	SKYSIZE			(1 << SKYSHIFT)
#define SKYMASK			(SKYSIZE - 1)

void R_TimeRefresh_f (void);
void R_ReadPointFile_f (void);

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


//
// screen size info
//
extern	int		d_lightstylevalue[256];	// 8.8 fraction of base light value

extern	qboolean	envmap;
extern	image_t*	particletexture;
extern	image_t*	playertextures[MAX_CLIENTS];

extern	QCvar*	r_norefresh;
extern	QCvar*	r_drawentities;
extern	QCvar*	r_drawviewmodel;
extern	QCvar*	r_netgraph;

extern	QCvar*	gl_cull;
extern	QCvar*	gl_polyblend;
extern	QCvar*	gl_nocolors;
extern	QCvar*	gl_reporttjunctions;

//
// gl_rlight.c
//
void R_AnimateLight (void);

//
// gl_ngraph.c
//
void R_NetGraph (void);
