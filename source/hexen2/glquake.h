/*
 * $Header: /H2 Mission Pack/glquake.h 10    3/09/98 11:24p Mgummelt $
 */

// disable data conversion warnings

#ifdef _WIN32
#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA
#endif
  
#ifndef _WIN32
#define PROC void*
#endif

#include "../client/render_local.h"

#include <GL/glu.h>

#define MAX_EXTRA_TEXTURES 156   // 255-100+1
extern image_t*		gl_extra_textures[MAX_EXTRA_TEXTURES];   // generic textures for models

// r_local.h -- private refresh defs

#define TILE_SIZE		128		// size of textures generated by R_GenTiledSurf

#define SKYSHIFT		7
#define	SKYSIZE			(1 << SKYSHIFT)
#define SKYMASK			(SKYSIZE - 1)

void R_TimeRefresh_f (void);

//====================================================


extern	qboolean	r_cache_thrash;		// compatability

extern	image_t*	playertextures[16];

extern	QCvar*	r_drawviewmodel;

extern	QCvar*	gl_polyblend;
extern	QCvar*	gl_nocolors;
extern	QCvar*	gl_reporttjunctions;

extern byte *playerTranslation;

void R_AnimateLight(void);
void V_CalcBlend (void);
void R_InitParticles (void);
void SCR_DrawLoading (void);

extern qboolean	vid_initialized;
