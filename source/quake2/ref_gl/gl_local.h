/*
Copyright (C) 1997-2001 Id Software, Inc.

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
#include <stdio.h>

#include <math.h>

#include "../../client/client.h"
#include "../client/ref.h"

#include "qgl.h"

#include <GL/glu.h>

#ifndef GL_COLOR_INDEX8_EXT
#define GL_COLOR_INDEX8_EXT GL_COLOR_INDEX
#endif

#define	REF_VERSION	"GL 0.01"

//===================================================================

#include "gl_model.h"

void GL_EndRendering (void);

void GL_UpdateSwapInterval( void );

//====================================================

//
// screen size info
//
extern	refdef_t	r_newrefdef;

extern	QCvar	*r_norefresh;

extern	QCvar	*gl_nosubimage;
extern	QCvar	*gl_cull;
extern	QCvar	*gl_poly;
extern	QCvar	*gl_polyblend;
extern	QCvar	*gl_lightmaptype;
extern  QCvar  *gl_driver;

void R_TranslatePlayerSkin (int playernum);

//====================================================================


void V_AddBlend (float r, float g, float b, float a, float *v_blend);

int 	R_Init();
void	R_Shutdown( void );

void R_RenderView (refdef_t *fd);
void Draw_InitLocal (void);

mbrush38_glpoly_t *WaterWarpPolyVerts (mbrush38_glpoly_t *p);

void COM_StripExtension (char *in, char *out);

void	Draw_GetPicSize (int *w, int *h, const char *name);
void	Draw_Pic (int x, int y, char *name);
void	Draw_StretchPic (int x, int y, int w, int h, const char *name);
void	Draw_Char (int x, int y, int c);
void	Draw_TileClear (int x, int y, int w, int h, const char *name);
void	Draw_Fill (int x, int y, int w, int h, int c);
void	Draw_FadeScreen (void);
void	Draw_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data);

void	R_BeginFrame( float camera_separation );
void	R_SwapBuffers( int );

image_t *R_RegisterSkinQ2 (const char *name);

/*
** GL extension emulation functions
*/
void GL_DrawParticles( int n, const particle_t particles[], const unsigned colortable[768] );

typedef struct
{
	float camera_separation;
} glstate2_t;

extern glstate2_t   gl_state;

/*
====================================================================

IMPORTED FUNCTIONS

====================================================================
*/

extern	refimport_t	ri;

extern QCvar		*vid_ref;
extern qboolean		reflib_active;

refexport_t GetRefAPI (refimport_t rimp);

void VID_Restart_f (void);
void VID_NewWindow ( int width, int height);
void VID_FreeReflib (void);
qboolean VID_LoadRefresh();


/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void		GLimp_BeginFrame( float camera_separation );
void		GLimp_EndFrame( void );
