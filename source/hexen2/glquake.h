/*
 * $Header: /H2 Mission Pack/glquake.h 10    3/09/98 11:24p Mgummelt $
 */

// disable data conversion warnings

#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA
  
#ifdef _WIN32
#include <windows.h>
#else
#define PROC void*
#endif

#include <GL/gl.h>
#include <GL/glu.h>

void GL_BeginRendering (int *x, int *y, int *width, int *height);
void GL_EndRendering (void);


// Function prototypes for the Texture Object Extension routines
typedef GLboolean (APIENTRY *ARETEXRESFUNCPTR)(GLsizei, const GLuint *,
                    const GLboolean *);
typedef void (APIENTRY *BINDTEXFUNCPTR)(GLenum, GLuint);
typedef void (APIENTRY *DELTEXFUNCPTR)(GLsizei, const GLuint *);
typedef void (APIENTRY *GENTEXFUNCPTR)(GLsizei, GLuint *);
typedef GLboolean (APIENTRY *ISTEXFUNCPTR)(GLuint);
typedef void (APIENTRY *PRIORTEXFUNCPTR)(GLsizei, const GLuint *,
                    const GLclampf *);
typedef void (APIENTRY *TEXSUBIMAGEPTR)(int, int, int, int, int, int, int, int, void *);
typedef int  (APIENTRY *FX_DISPLAY_MODE_EXT)(int);
typedef void (APIENTRY *FX_SET_PALETTE_EXT)( unsigned long * );
typedef void (APIENTRY *FX_MARK_PAL_TEXTURE_EXT)( void );

extern	DELTEXFUNCPTR delTexFunc;
extern	TEXSUBIMAGEPTR TexSubImage2DFunc;
extern  FX_DISPLAY_MODE_EXT fxDisplayModeExtension;
extern  FX_SET_PALETTE_EXT fxSetPaletteExtension;
extern  FX_MARK_PAL_TEXTURE_EXT fxMarkPalTextureExtension;

#define INVERSE_PAL_R_BITS 6
#define INVERSE_PAL_G_BITS 6
#define INVERSE_PAL_B_BITS 6
#define INVERSE_PAL_TOTAL_BITS \
	( INVERSE_PAL_R_BITS + INVERSE_PAL_G_BITS + INVERSE_PAL_B_BITS )

extern unsigned char inverse_pal[(1<<INVERSE_PAL_TOTAL_BITS)+1];

extern	int texture_extension_number;
extern	int		texture_mode;

extern	float	gldepthmin, gldepthmax;

#define MAX_EXTRA_TEXTURES 156   // 255-100+1
extern int			gl_extra_textures[MAX_EXTRA_TEXTURES];   // generic textures for models

void GL_Upload32 (unsigned *data, int width, int height,  qboolean mipmap, qboolean alpha);
void GL_Upload8 (byte *data, int width, int height,  qboolean mipmap, qboolean alpha, int mode);
int GL_LoadTexture (char *identifier, int width, int height, byte *data, qboolean mipmap, qboolean alpha, int mode);
int GL_LoadTransTexture (char *identifier, int width, int height, byte *data, qboolean mipmap, byte Alpha);
int GL_FindTexture (char *identifier);

typedef struct
{
	float	x, y, z;
	float	s, t;
	float	r, g, b;
} glvert_t;

extern glvert_t glv;

typedef struct
{
	int		texnum;
	float	sl, tl, sh, th;
} glpic_t;

extern	int glx, gly, glwidth, glheight;

extern	PROC glArrayElementEXT;
extern	PROC glColorPointerEXT;
extern	PROC glTexturePointerEXT;
extern	PROC glVertexPointerEXT;


// r_local.h -- private refresh defs

#define ALIAS_BASE_SIZE_RATIO		(1.0 / 11.0)
					// normalizing factor so player model works out to about
					//  1 pixel per triangle
#define MAX_SKIN_HEIGHT 480

#define TILE_SIZE		128		// size of textures generated by R_GenTiledSurf

#define SKYSHIFT		7
#define	SKYSIZE			(1 << SKYSHIFT)
#define SKYMASK			(SKYSIZE - 1)

#define BACKFACE_EPSILON	0.01


void R_TimeRefresh_f (void);
void R_ReadPointFile_f (void);
texture_t *R_TextureAnimation (texture_t *base);

typedef struct surfcache_s
{
	struct surfcache_s	*next;
	struct surfcache_s 	**owner;		// NULL is an empty chunk of memory
	int					lightadj[MAXLIGHTMAPS]; // checked for strobe flush
	int					dlight;
	int					size;		// including header
	unsigned			width;
	unsigned			height;		// DEBUG only needed for debug
	float				mipscale;
	struct texture_s	*texture;	// checked for animating textures
	byte				data[4];	// width*height elements
} surfcache_t;


typedef struct
{
	pixel_t		*surfdat;	// destination for generated surface
	int			rowbytes;	// destination logical width in bytes
	msurface_t	*surf;		// description for surface to generate
	fixed8_t	lightadj[MAXLIGHTMAPS];
							// adjust for lightmap levels for dynamic lighting
	texture_t	*texture;	// corrected for animating textures
	int			surfmip;	// mipmapped ratio of surface texels / world pixels
	int			surfwidth;	// in mipmapped texels
	int			surfheight;	// in mipmapped texels
} drawsurf_t;


// Changes to ptype_t must also be made in d_iface.h
typedef enum {
	pt_static,
	pt_grav,
	pt_fastgrav,
	pt_slowgrav,
	pt_fire,
	pt_explode,
	pt_explode2,
	pt_blob,
	pt_blob2,
	pt_rain,
	pt_c_explode,
	pt_c_explode2,
	pt_spit,
	pt_fireball,
	pt_ice,
	pt_spell,
	pt_test,
	pt_quake,
	pt_rd,			// rider's death
	pt_vorpal,
	pt_setstaff,
	pt_magicmissile,
	pt_boneshard,
	pt_scarab,
	pt_acidball,
	pt_darken,
	pt_snow,
	pt_gravwell,
	pt_redfire
} ptype_t;

// Changes to rtype_t must also be made in glquake.h
typedef enum
{
   rt_rocket_trail = 0,
	rt_smoke,
	rt_blood,
	rt_tracer,
	rt_slight_blood,
	rt_tracer2,
	rt_voor_trail,
	rt_fireball,
	rt_ice,
	rt_spit,
	rt_spell,
	rt_vorpal,
	rt_setstaff,
	rt_magicmissile,
	rt_boneshard,
	rt_scarab,
	rt_acidball,
	rt_bloodshot,
} rt_type_t;

// !!! if this is changed, it must be changed in d_iface.h too !!!
typedef struct particle_s
{
// driver-usable fields
	vec3_t		org;
	float		color;
// drivers never touch the following fields
	struct particle_s	*next;
	vec3_t		vel;
	vec3_t		min_org;
	vec3_t		max_org;
	float		ramp;
	float		die;
	byte		type;
	byte		flags;
	byte		count;
} particle_t;


//====================================================


extern	entity_t	r_worldentity;
extern	qboolean	r_cache_thrash;		// compatability
extern	vec3_t		modelorg, r_entorigin;
extern	entity_t	*currententity;
extern	int			r_visframecount;	// ??? what difs?
extern	int			r_framecount;
extern	cplane_t	frustum[4];
extern	int		c_brush_polys, c_alias_polys, c_sky_polys;


//
// view origin
//
extern "C"
{
extern	vec3_t	vup;
extern	vec3_t	vpn;
extern	vec3_t	vright;
}
extern	vec3_t	r_origin;

//
// screen size info
//
extern	refdef_t	r_refdef;
extern	mleaf_t		*r_viewleaf, *r_oldviewleaf;
extern	texture_t	*r_notexture_mip;
extern	int		d_lightstylevalue[256];	// 8.8 fraction of base light value

extern	qboolean	envmap;
extern	int	currenttexture;
extern	int	particletexture;
extern	int	playertextures;

extern	int	skytexturenum;		// index in cl.loadmodel, not gl texture object

extern	QCvar*	r_norefresh;
extern	QCvar*	r_drawentities;
extern	QCvar*	r_drawviewmodel;
extern	QCvar*	r_speeds;
extern	QCvar*	r_fullbright;
extern	QCvar*	r_lightmap;
extern	QCvar*	r_shadows;
extern	QCvar*	r_mirroralpha;
extern	QCvar*	r_wateralpha;
extern	QCvar*	r_dynamic;
extern	QCvar*	r_novis;
extern	QCvar*	r_wholeframe;

extern	QCvar*	gl_clear;
extern	QCvar*	gl_cull;
extern	QCvar*	gl_texsort;
extern	QCvar*	gl_smoothmodels;
extern	QCvar*	gl_affinemodels;
extern	QCvar*	gl_polyblend;
extern	QCvar*	gl_flashblend;
extern	QCvar*	gl_playermip;
extern	QCvar*	gl_nocolors;
extern	QCvar*	gl_keeptjunctions;
extern	QCvar*	gl_reporttjunctions;

extern	int		gl_lightmap_format;
extern	int		gl_solid_format;
extern	int		gl_alpha_format;

extern	QCvar*	gl_max_size;

extern	int			mirrortexturenum;	// quake texturenum, not gltexturenum
extern	qboolean	mirror;
extern	cplane_t	*mirror_plane;

extern	float	r_world_matrix[16];

extern	const char *gl_vendor;
extern	const char *gl_renderer;
extern	const char *gl_version;
extern	const char *gl_extensions;

void R_TranslatePlayerSkin (int playernum);
void GL_Bind (int texnum);

extern byte *playerTranslation;

// Multitexture
#define    TEXTURE0_SGIS				0x835E
#define    TEXTURE1_SGIS				0x835F

#ifndef _WIN32
#define APIENTRY /* */
#endif

typedef void (APIENTRY *lpMTexFUNC) (GLenum, GLfloat, GLfloat);
typedef void (APIENTRY *lpSelTexFUNC) (GLenum);
extern lpMTexFUNC qglMTexCoord2fSGIS;
extern lpSelTexFUNC qglSelectTextureSGIS;

extern qboolean gl_mtexable;

void GL_DisableMultitexture(void);
void GL_EnableMultitexture(void);

void D_ShowLoadingSize(void);
model_t *Mod_FindName (char *name);
void GL_SubdivideSurface (msurface_t *fa);
void GL_MakeAliasModelDisplayLists (model_t *m, aliashdr_t *hdr);
int R_LightPoint (vec3_t p);
void R_DrawBrushModel (entity_t *e, qboolean Translucent);
void R_AnimateLight(void);
void V_CalcBlend (void);
void R_DrawWorld (void);
void R_RenderDlights (void);
void R_DrawParticles (void);
void R_DrawWaterSurfaces (void);
void R_RenderBrushPoly (msurface_t *fa, qboolean override);
void R_InitParticles (void);
void GL_BuildLightmaps (void);
void EmitWaterPolys (msurface_t *fa);
void EmitSkyPolys (msurface_t *fa, qboolean save);
void EmitBothSkyLayers (msurface_t *fa);
void R_DrawSkyChain (msurface_t *s);
qboolean R_CullBox (vec3_t mins, vec3_t maxs);
void R_MarkLights (dlight_t *light, int bit, mnode_t *node);
void R_RotateForEntity (entity_t *e);
void R_StoreEfrags (efrag_t **ppefrag);
void GL_Set2D (void);
void SCR_DrawLoading (void);
int M_DrawBigCharacter (int x, int y, int num, int numNext);
