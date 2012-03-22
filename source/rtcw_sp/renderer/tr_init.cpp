/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// tr_init.c -- functions that are not called every frame

#include "tr_local.h"

void AssertCvarRange(Cvar* cv, float minVal, float maxVal, bool shouldBeIntegral);
void R_Register_();

glstate_t glState;

static void GfxInfo_f( void );

Cvar  *r_zfar;

Cvar  *r_inGameVideo;
Cvar  *r_dlightBacks;
Cvar  *r_dlightScale; //----(SA)	added

Cvar  *r_waterFogColor;   //----(SA)	added
Cvar  *r_mapFogColor;
Cvar  *r_savegameFogColor;    //----(SA)	added

Cvar  *r_allowExtensions;

//----(SA)	added
Cvar  *r_ext_texture_filter_anisotropic;

Cvar  *r_ext_NV_fog_dist;
Cvar  *r_nv_fogdist_mode;

Cvar  *r_ext_ATI_pntriangles;
Cvar  *r_ati_truform_tess;        //
Cvar  *r_ati_truform_normalmode;  // linear/quadratic
Cvar  *r_ati_truform_pointmode;   // linear/cubic
//----(SA)	end

Cvar  *r_ati_fsaa_samples;        //DAJ valids are 1, 2, 4

Cvar  *r_glDriver;
Cvar  *r_portalsky;   //----(SA)	added
Cvar  *r_lowMemTextureSize;
Cvar  *r_lowMemTextureThreshold;
Cvar  *r_picmip2;

extern Cvar  *r_customwidth;
extern Cvar  *r_customheight;
extern Cvar  *r_customaspect;

// Ridah
Cvar  *r_cache;
Cvar  *r_cacheShaders;
Cvar  *r_cacheModels;
Cvar  *r_compressModels;
Cvar  *r_exportCompressedModels;

Cvar  *r_cacheGathering;

Cvar  *r_buildScript;

Cvar  *r_bonesDebug;
// done.

// Rafael - wolf fog
Cvar  *r_wolffog;
// done

Cvar  *r_highQualityVideo;
Cvar  *r_rmse;

int max_polys;
int max_polyverts;

//----(SA)	added
void ( APIENTRY * qglPNTrianglesiATI )( GLenum pname, GLint param );
void ( APIENTRY * qglPNTrianglesfATI )( GLenum pname, GLfloat param );
/*
The tessellation level and normal generation mode are specified with:

	void qglPNTriangles{if}ATI(enum pname, T param)

	If <pname> is:
		GL_PN_TRIANGLES_NORMAL_MODE_ATI -
			<param> must be one of the symbolic constants:
				- GL_PN_TRIANGLES_NORMAL_MODE_LINEAR_ATI or
				- GL_PN_TRIANGLES_NORMAL_MODE_QUADRATIC_ATI
			which will select linear or quadratic normal interpolation respectively.
		GL_PN_TRIANGLES_POINT_MODE_ATI -
			<param> must be one of the symbolic  constants:
				- GL_PN_TRIANGLES_POINT_MODE_LINEAR_ATI or
				- GL_PN_TRIANGLES_POINT_MODE_CUBIC_ATI
			which will select linear or cubic interpolation respectively.
		GL_PN_TRIANGLES_TESSELATION_LEVEL_ATI -
			<param> should be a value specifying the number of evaluation points on each edge.  This value must be
			greater than 0 and less than or equal to the value given by GL_MAX_PN_TRIANGLES_TESSELATION_LEVEL_ATI.

	An INVALID_VALUE error will be generated if the value for <param> is less than zero or greater than the max value.

Associated 'gets':
Get Value                               Get Command Type     Minimum Value								Attribute
---------                               ----------- ----     ------------								---------
PN_TRIANGLES_ATI						IsEnabled   B		False                                       PN Triangles/enable
PN_TRIANGLES_NORMAL_MODE_ATI			GetIntegerv Z2		PN_TRIANGLES_NORMAL_MODE_QUADRATIC_ATI		PN Triangles
PN_TRIANGLES_POINT_MODE_ATI				GetIntegerv Z2		PN_TRIANGLES_POINT_MODE_CUBIC_ATI			PN Triangles
PN_TRIANGLES_TESSELATION_LEVEL_ATI		GetIntegerv Z+		1											PN Triangles
MAX_PN_TRIANGLES_TESSELATION_LEVEL_ATI	GetIntegerv Z+		1											-




*/
//----(SA)	end


/*
** InitOpenGL
**
** This function is responsible for initializing a valid OpenGL subsystem.  This
** is done by calling GLimp_Init (which gives us a working OGL subsystem) then
** setting variables, checking GL constants, and reporting the gfx system config
** to the user.
*/
static void InitOpenGL( void ) {
	char renderer_buffer[1024];

	//
	// initialize OS specific portions of the renderer
	//
	// GLimp_Init directly or indirectly references the following cvars:
	//		- r_fullscreen
	//		- r_glDriver
	//		- r_mode
	//		- r_(color|depth|stencil)bits
	//		- r_ignorehwgamma
	//		- r_gamma
	//

	if ( glConfig.vidWidth == 0 ) {
		GLint temp;

		GLimp_Init();

		String::Cpy( renderer_buffer, glConfig.renderer_string );
		String::ToLower( renderer_buffer );

		// OpenGL driver constants
		qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &temp );
		glConfig.maxTextureSize = temp;

		// stubbed or broken drivers may have reported 0...
		if ( glConfig.maxTextureSize <= 0 ) {
			glConfig.maxTextureSize = 0;
		}
	}

	// init command buffers and SMP
	R_InitCommandBuffers();

	// print info
	GfxInfo_f();

	// set default state
	GL_SetDefaultState();
}

/*
==================
GL_CheckErrors
==================
*/
void GL_CheckErrors( void ) {
	int err;
	char s[64];

	err = qglGetError();
	if ( err == GL_NO_ERROR ) {
		return;
	}
	if ( r_ignoreGLErrors->integer ) {
		return;
	}
	switch ( err ) {
	case GL_INVALID_ENUM:
		String::Cpy( s, "GL_INVALID_ENUM" );
		break;
	case GL_INVALID_VALUE:
		String::Cpy( s, "GL_INVALID_VALUE" );
		break;
	case GL_INVALID_OPERATION:
		String::Cpy( s, "GL_INVALID_OPERATION" );
		break;
	case GL_STACK_OVERFLOW:
		String::Cpy( s, "GL_STACK_OVERFLOW" );
		break;
	case GL_STACK_UNDERFLOW:
		String::Cpy( s, "GL_STACK_UNDERFLOW" );
		break;
	case GL_OUT_OF_MEMORY:
		String::Cpy( s, "GL_OUT_OF_MEMORY" );
		break;
	default:
		String::Sprintf( s, sizeof( s ), "%i", err );
		break;
	}

	ri.Error( ERR_FATAL, "GL_CheckErrors: %s", s );
}


/*
** R_GetModeInfo
*/
typedef struct vidmode_s
{
	const char *description;
	int width, height;
	float pixelAspect;              // pixel width / height
} vidmode_t;

vidmode_t r_vidModes[] =
{
	{ "Mode  0: 320x240",        320,    240,    1 },
	{ "Mode  1: 400x300",        400,    300,    1 },
	{ "Mode  2: 512x384",        512,    384,    1 },
	{ "Mode  3: 640x480",        640,    480,    1 },
	{ "Mode  4: 800x600",        800,    600,    1 },
	{ "Mode  5: 960x720",        960,    720,    1 },
	{ "Mode  6: 1024x768",       1024,   768,    1 },
	{ "Mode  7: 1152x864",       1152,   864,    1 },
	{ "Mode  8: 1280x1024",      1280,   1024,   1 },
	{ "Mode  9: 1600x1200",      1600,   1200,   1 },
	{ "Mode 10: 2048x1536",      2048,   1536,   1 },
	{ "Mode 11: 856x480 (wide)",856, 480,    1 },
	{ "Mode 12: 1920x1200 (wide)",1920,  1200,   1 }     //----(SA)	added
};
static int s_numVidModes = ( sizeof( r_vidModes ) / sizeof( r_vidModes[0] ) );

qboolean R_GetModeInfo( int *width, int *height, float *windowAspect, int mode ) {
	vidmode_t   *vm;

	if ( mode < -1 ) {
		return qfalse;
	}
	if ( mode >= s_numVidModes ) {
		return qfalse;
	}

	if ( mode == -1 ) {
		*width = r_customwidth->integer;
		*height = r_customheight->integer;
		*windowAspect = r_customaspect->value;
		return qtrue;
	}

	vm = &r_vidModes[mode];

	*width  = vm->width;
	*height = vm->height;
	*windowAspect = (float)vm->width / ( vm->height * vm->pixelAspect );

	return qtrue;
}

/*
** R_ModeList_f
*/
static void R_ModeList_f( void ) {
	int i;

	ri.Printf( PRINT_ALL, "\n" );
	for ( i = 0; i < s_numVidModes; i++ )
	{
		ri.Printf( PRINT_ALL, "%s\n", r_vidModes[i].description );
	}
	ri.Printf( PRINT_ALL, "\n" );
}


/*
==============================================================================

						SCREEN SHOTS

==============================================================================
*/

/*
==================
R_TakeScreenshot
==================
*/
void R_TakeScreenshot( int x, int y, int width, int height, char *fileName ) {
	byte        *buffer;
	int i, c, temp;

	buffer = (byte*)ri.Hunk_AllocateTempMemory( glConfig.vidWidth * glConfig.vidHeight * 3 + 18 );

	memset( buffer, 0, 18 );
	buffer[2] = 2;      // uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24;    // pixel size

	qglReadPixels( x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer + 18 );

	// swap rgb to bgr
	c = 18 + width * height * 3;
	for ( i = 18 ; i < c ; i += 3 ) {
		temp = buffer[i];
		buffer[i] = buffer[i + 2];
		buffer[i + 2] = temp;
	}

	// gamma correct
	if ( ( tr.overbrightBits > 0 ) && glConfig.deviceSupportsGamma ) {
		R_GammaCorrect( buffer + 18, glConfig.vidWidth * glConfig.vidHeight * 3 );
	}

	ri.FS_WriteFile( fileName, buffer, c );

	ri.Hunk_FreeTempMemory( buffer );
}

/*
==============
R_TakeScreenshotJPEG
==============
*/
void R_TakeScreenshotJPEG( int x, int y, int width, int height, char *fileName ) {
	byte        *buffer;

	buffer = (byte*)ri.Hunk_AllocateTempMemory( glConfig.vidWidth * glConfig.vidHeight * 4 );

	qglReadPixels( x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer );

	// gamma correct
	if ( ( tr.overbrightBits > 0 ) && glConfig.deviceSupportsGamma ) {
		R_GammaCorrect( buffer, glConfig.vidWidth * glConfig.vidHeight * 4 );
	}

	ri.FS_WriteFile( fileName, buffer, 1 );     // create path
	SaveJPG( fileName, 95, glConfig.vidWidth, glConfig.vidHeight, buffer );

	ri.Hunk_FreeTempMemory( buffer );
}

/*
==================
R_ScreenshotFilename
==================
*/
void R_ScreenshotFilename( int lastNumber, char *fileName ) {
	int a,b,c,d;

	if ( lastNumber < 0 || lastNumber > 9999 ) {
		String::Sprintf( fileName, MAX_OSPATH, "screenshots/shot9999.tga" );
		return;
	}

	a = lastNumber / 1000;
	lastNumber -= a * 1000;
	b = lastNumber / 100;
	lastNumber -= b * 100;
	c = lastNumber / 10;
	lastNumber -= c * 10;
	d = lastNumber;

	String::Sprintf( fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.tga"
				 , a, b, c, d );
}

/*
==============
R_ScreenshotFilenameJPEG
==============
*/
void R_ScreenshotFilenameJPEG( int lastNumber, char *fileName ) {
	int a,b,c,d;

	if ( lastNumber < 0 || lastNumber > 9999 ) {
		String::Sprintf( fileName, MAX_OSPATH, "screenshots/shot9999.jpg" );
		return;
	}

	a = lastNumber / 1000;
	lastNumber -= a * 1000;
	b = lastNumber / 100;
	lastNumber -= b * 100;
	c = lastNumber / 10;
	lastNumber -= c * 10;
	d = lastNumber;

	String::Sprintf( fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.jpg"
				 , a, b, c, d );
}

/*
====================
R_LevelShot

levelshots are specialized 128*128 thumbnails for
the menu system, sampled down from full screen distorted images
====================
*/
void R_LevelShot( void ) {
	char checkname[MAX_OSPATH];
	byte        *buffer;
	byte        *source;
	byte        *src, *dst;
	int x, y;
	int r, g, b;
	float xScale, yScale;
	int xx, yy;

	sprintf( checkname, "levelshots/%s.tga", tr.world->baseName );

	source = (byte*)ri.Hunk_AllocateTempMemory( glConfig.vidWidth * glConfig.vidHeight * 3 );

	buffer = (byte*)ri.Hunk_AllocateTempMemory( 128 * 128 * 3 + 18 );
	memset( buffer, 0, 18 );
	buffer[2] = 2;      // uncompressed type
	buffer[12] = 128;
	buffer[14] = 128;
	buffer[16] = 24;    // pixel size

	qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGB, GL_UNSIGNED_BYTE, source );

	// resample from source
	xScale = glConfig.vidWidth / 512.0f;
	yScale = glConfig.vidHeight / 384.0f;
	for ( y = 0 ; y < 128 ; y++ ) {
		for ( x = 0 ; x < 128 ; x++ ) {
			r = g = b = 0;
			for ( yy = 0 ; yy < 3 ; yy++ ) {
				for ( xx = 0 ; xx < 4 ; xx++ ) {
					src = source + 3 * ( glConfig.vidWidth * (int)( ( y * 3 + yy ) * yScale ) + (int)( ( x * 4 + xx ) * xScale ) );
					r += src[0];
					g += src[1];
					b += src[2];
				}
			}
			dst = buffer + 18 + 3 * ( y * 128 + x );
			dst[0] = b / 12;
			dst[1] = g / 12;
			dst[2] = r / 12;
		}
	}

	// gamma correct
	if ( ( tr.overbrightBits > 0 ) && glConfig.deviceSupportsGamma ) {
		R_GammaCorrect( buffer + 18, 128 * 128 * 3 );
	}

	ri.FS_WriteFile( checkname, buffer, 128 * 128 * 3 + 18 );

	ri.Hunk_FreeTempMemory( buffer );
	ri.Hunk_FreeTempMemory( source );

	ri.Printf( PRINT_ALL, "Wrote %s\n", checkname );
}

/*
==================
R_ScreenShot_f

screenshot
screenshot [silent]
screenshot [levelshot]
screenshot [filename]

Doesn't print the pacifier message if there is a second arg
==================
*/
void R_ScreenShot_f( void ) {
	char checkname[MAX_OSPATH];
	int len;
	static int lastNumber = -1;
	qboolean silent;

	if ( !String::Cmp( ri.Cmd_Argv( 1 ), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !String::Cmp( ri.Cmd_Argv( 1 ), "silent" ) ) {
		silent = qtrue;
	} else {
		silent = qfalse;
	}

	if ( ri.Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		String::Sprintf( checkname, MAX_OSPATH, "screenshots/%s.tga", ri.Cmd_Argv( 1 ) );
	} else {
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if ( lastNumber == -1 ) {
			lastNumber = 0;
		}
		// scan for a free number
		for ( ; lastNumber <= 9999 ; lastNumber++ ) {
			R_ScreenshotFilename( lastNumber, checkname );

			len = ri.FS_ReadFile( checkname, NULL );
			if ( len <= 0 ) {
				break;  // file doesn't exist
			}
		}

		if ( lastNumber >= 9999 ) {
			ri.Printf( PRINT_ALL, "ScreenShot: Couldn't create a file\n" );
			return;
		}

		lastNumber++;
	}


	R_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname );

	if ( !silent ) {
		ri.Printf( PRINT_ALL, "Wrote %s\n", checkname );
	}
}

void R_ScreenShotJPEG_f( void ) {
	char checkname[MAX_OSPATH];
	int len;
	static int lastNumber = -1;
	qboolean silent;

	if ( !String::Cmp( ri.Cmd_Argv( 1 ), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !String::Cmp( ri.Cmd_Argv( 1 ), "silent" ) ) {
		silent = qtrue;
	} else {
		silent = qfalse;
	}

	if ( ri.Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		String::Sprintf( checkname, MAX_OSPATH, "screenshots/%s.jpg", ri.Cmd_Argv( 1 ) );
	} else {
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if ( lastNumber == -1 ) {
			lastNumber = 0;
		}
		// scan for a free number
		for ( ; lastNumber <= 9999 ; lastNumber++ ) {
			R_ScreenshotFilenameJPEG( lastNumber, checkname );

			len = ri.FS_ReadFile( checkname, NULL );
			if ( len <= 0 ) {
				break;  // file doesn't exist
			}
		}

		if ( lastNumber == 10000 ) {
			ri.Printf( PRINT_ALL, "ScreenShot: Couldn't create a file\n" );
			return;
		}

		lastNumber++;
	}


	R_TakeScreenshotJPEG( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname );

	if ( !silent ) {
		ri.Printf( PRINT_ALL, "Wrote %s\n", checkname );
	}
}

//============================================================================

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState( void ) {
	qglClearDepth( 1.0f );

	qglCullFace( GL_FRONT );

	qglColor4f( 1,1,1,1 );

	// initialize downstream texture unit if we're running
	// in a multitexture environment
	if ( qglActiveTextureARB ) {
		GL_SelectTexture( 1 );
		GL_TextureMode( r_textureMode->string );
		GL_TexEnv( GL_MODULATE );
		qglDisable( GL_TEXTURE_2D );
		GL_SelectTexture( 0 );
	}

	qglEnable( GL_TEXTURE_2D );
	GL_TextureMode( r_textureMode->string );
	GL_TexEnv( GL_MODULATE );

	qglShadeModel( GL_SMOOTH );
	qglDepthFunc( GL_LEQUAL );

	// the vertex array is always enabled, but the color and texture
	// arrays are enabled and disabled around the compiled vertex array call
	qglEnableClientState( GL_VERTEX_ARRAY );

	//
	// make sure our GL state vector is set correctly
	//
	glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;

	qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	qglDepthMask( GL_TRUE );
	qglDisable( GL_DEPTH_TEST );
	qglEnable( GL_SCISSOR_TEST );
	qglDisable( GL_CULL_FACE );
	qglDisable( GL_BLEND );

//----(SA)	added.
	// ATI pn_triangles
	if ( qglPNTrianglesiATI ) {
		int maxtess;
		// get max supported tesselation
		qglGetIntegerv( GL_MAX_PN_TRIANGLES_TESSELATION_LEVEL_ATI, (GLint*)&maxtess );
#ifdef __MACOS__
		glConfig.ATIMaxTruformTess = 7;
#else
		glConfig.ATIMaxTruformTess = maxtess;
#endif
		// cap if necessary
		if ( r_ati_truform_tess->value > maxtess ) {
			ri.Cvar_Set( "r_ati_truform_tess", va( "%d", maxtess ) );
		}

		// set Wolf defaults
		qglPNTrianglesiATI( GL_PN_TRIANGLES_TESSELATION_LEVEL_ATI, r_ati_truform_tess->value );
	}

	if ( glConfig.anisotropicAvailable ) {
		float maxAnisotropy;

		qglGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy );
		glConfig.maxAnisotropy = maxAnisotropy;

		// set when rendering
//	   qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, glConfig.maxAnisotropy);
	}

//----(SA)	end
}


/*
================
GfxInfo_f
================
*/
void GfxInfo_f( void ) {
	Cvar *sys_cpustring = ri.Cvar_Get( "sys_cpustring", "", 0 );
	const char *enablestrings[] =
	{
		"disabled",
		"enabled"
	};
	const char *fsstrings[] =
	{
		"windowed",
		"fullscreen"
	};

	ri.Printf( PRINT_ALL, "\nGL_VENDOR: %s\n", glConfig.vendor_string );
	ri.Printf( PRINT_ALL, "GL_RENDERER: %s\n", glConfig.renderer_string );
	ri.Printf( PRINT_ALL, "GL_VERSION: %s\n", glConfig.version_string );
	ri.Printf( PRINT_ALL, "GL_EXTENSIONS: %s\n", glConfig.extensions_string );
	ri.Printf( PRINT_ALL, "GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize );
	ri.Printf( PRINT_ALL, "GL_MAX_ACTIVE_TEXTURES_ARB: %d\n", glConfig.maxActiveTextures );
	ri.Printf( PRINT_ALL, "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
	ri.Printf( PRINT_ALL, "MODE: %d, %d x %d %s hz:", r_mode->integer, glConfig.vidWidth, glConfig.vidHeight, fsstrings[r_fullscreen->integer == 1] );
	if ( glConfig.displayFrequency ) {
		ri.Printf( PRINT_ALL, "%d\n", glConfig.displayFrequency );
	} else
	{
		ri.Printf( PRINT_ALL, "N/A\n" );
	}
	if ( glConfig.deviceSupportsGamma ) {
		ri.Printf( PRINT_ALL, "GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits );
	} else
	{
		ri.Printf( PRINT_ALL, "GAMMA: software w/ %d overbright bits\n", tr.overbrightBits );
	}
	ri.Printf( PRINT_ALL, "CPU: %s\n", sys_cpustring->string );

	// rendering primitives
	{
		int primitives;

		// default is to use triangles if compiled vertex arrays are present
		ri.Printf( PRINT_ALL, "rendering primitives: " );
		primitives = r_primitives->integer;
		if ( primitives == 0 ) {
			if ( qglLockArraysEXT ) {
				primitives = 2;
			} else {
				primitives = 1;
			}
		}
		if ( primitives == -1 ) {
			ri.Printf( PRINT_ALL, "none\n" );
		} else if ( primitives == 2 ) {
			ri.Printf( PRINT_ALL, "single glDrawElements\n" );
		} else if ( primitives == 1 ) {
			ri.Printf( PRINT_ALL, "multiple glArrayElement\n" );
		} else if ( primitives == 3 ) {
			ri.Printf( PRINT_ALL, "multiple glColor4ubv + glTexCoord2fv + glVertex3fv\n" );
		}
	}

	ri.Printf( PRINT_ALL, "texturemode: %s\n", r_textureMode->string );
	ri.Printf( PRINT_ALL, "picmip: %d\n", r_picmip->integer );
	ri.Printf( PRINT_ALL, "picmip2: %d\n", r_picmip2->integer );
	ri.Printf( PRINT_ALL, "texture bits: %d\n", r_texturebits->integer );
	ri.Printf( PRINT_ALL, "multitexture: %s\n", enablestrings[qglActiveTextureARB != 0] );
	ri.Printf( PRINT_ALL, "compiled vertex arrays: %s\n", enablestrings[qglLockArraysEXT != 0 ] );
	ri.Printf( PRINT_ALL, "texenv add: %s\n", enablestrings[glConfig.textureEnvAddAvailable != 0] );
	ri.Printf( PRINT_ALL, "compressed textures: %s\n", enablestrings[glConfig.textureCompression != TC_NONE] );

	ri.Printf( PRINT_ALL, "ATI truform: %s\n", enablestrings[qglPNTrianglesiATI != 0] );
	if ( qglPNTrianglesiATI ) {
//DAJ bogus at this point		ri.Printf( PRINT_ALL, "MAX_PN_TRIANGLES_TESSELATION_LEVEL_ATI: %d\n", glConfig.ATIMaxTruformTess );
		ri.Printf( PRINT_ALL, "Truform Tess: %d\n", r_ati_truform_tess->integer );
		ri.Printf( PRINT_ALL, "Truform Point Mode: %s\n", r_ati_truform_pointmode->string );
		ri.Printf( PRINT_ALL, "Truform Normal Mode: %s\n", r_ati_truform_normalmode->string );
	}

	ri.Printf( PRINT_ALL, "NV distance fog: %s\n", enablestrings[glConfig.NVFogAvailable != 0] );
	if ( glConfig.NVFogAvailable ) {
		ri.Printf( PRINT_ALL, "Fog Mode: %s\n", r_nv_fogdist_mode->string );
	}

	if ( r_vertexLight->integer ) {
		ri.Printf( PRINT_ALL, "HACK: using vertex lightmap approximation\n" );
	}
	if ( glConfig.smpActive ) {
		ri.Printf( PRINT_ALL, "Using dual processor acceleration\n" );
	}
	if ( r_finish->integer ) {
		ri.Printf( PRINT_ALL, "Forcing glFinish\n" );
	}
}

// RF
extern void R_CropImages_f( void );

/*
===============
R_Register
===============
*/
void R_Register( void ) {
	R_Register_();
	//
	// latched and archived variables
	//
	r_glDriver = ri.Cvar_Get( "r_glDriver", OPENGL_DRIVER_NAME, CVAR_ARCHIVE | CVAR_LATCH2 );
	r_allowExtensions = ri.Cvar_Get( "r_allowExtensions", "1", CVAR_ARCHIVE | CVAR_LATCH2 );

//----(SA)	added
	r_ext_ATI_pntriangles           = ri.Cvar_Get( "r_ext_ATI_pntriangles", "0", CVAR_ARCHIVE | CVAR_LATCH2 );   //----(SA)	default to '0'
	r_ati_truform_tess              = ri.Cvar_Get( "r_ati_truform_tess", "1", CVAR_ARCHIVE );
// GR - Change default mode -- linear doesn't do much...
	r_ati_truform_normalmode        = ri.Cvar_Get( "r_ati_truform_normalmode", "QUADRATIC", CVAR_ARCHIVE );
	r_ati_truform_pointmode         = ri.Cvar_Get( "r_ati_truform_pointmode", "CUBIC", CVAR_ARCHIVE );

	r_ati_fsaa_samples              = ri.Cvar_Get( "r_ati_fsaa_samples", "1", CVAR_ARCHIVE );       //DAJ valids are 1, 2, 4

	r_ext_texture_filter_anisotropic    = ri.Cvar_Get( "r_ext_texture_filter_anisotropic", "0", CVAR_ARCHIVE );

	r_ext_NV_fog_dist                   = ri.Cvar_Get( "r_ext_NV_fog_dist", "1", CVAR_ARCHIVE | CVAR_LATCH2 );
	r_nv_fogdist_mode                   = ri.Cvar_Get( "r_nv_fogdist_mode", "GL_EYE_RADIAL_NV", CVAR_ARCHIVE );    // default to 'looking good'
//----(SA)	end

	r_picmip2 = ri.Cvar_Get( "r_picmip2", "2", CVAR_ARCHIVE | CVAR_LATCH2 );   // used for character skins picmipping at a different level from the rest of the game
	r_lowMemTextureSize = ri.Cvar_Get( "r_lowMemTextureSize", "0", CVAR_ARCHIVE | CVAR_LATCH2 );
	r_lowMemTextureThreshold = ri.Cvar_Get( "r_lowMemTextureThreshold", "15.0", CVAR_ARCHIVE | CVAR_LATCH2 );
	r_rmse = ri.Cvar_Get( "r_rmse", "0.0", CVAR_ARCHIVE | CVAR_LATCH2 );
	AssertCvarRange( r_picmip2, 0, 16, qtrue );

//----(SA)	added
	r_zfar = ri.Cvar_Get( "r_zfar", "0", CVAR_CHEAT );
//----(SA)	end
	r_inGameVideo = ri.Cvar_Get( "r_inGameVideo", "1", CVAR_ARCHIVE );
	r_dlightScale = ri.Cvar_Get( "r_dlightScale", "1.0", CVAR_ARCHIVE );   //----(SA)	added
	r_dlightBacks = ri.Cvar_Get( "r_dlightBacks", "1", CVAR_ARCHIVE );

	r_waterFogColor = ri.Cvar_Get( "r_waterFogColor", "0", CVAR_ROM );  //----(SA)	added
	r_mapFogColor = ri.Cvar_Get( "r_mapFogColor", "0", CVAR_ROM );  //----(SA)	added
	r_savegameFogColor = ri.Cvar_Get( "r_savegameFogColor", "0", CVAR_ROM );    //----(SA)	added

	// Ridah
	// show_bug.cgi?id=440
	// NOTE TTimo: r_cache is disabled by default in SP
	ri.Cvar_Set( "r_cache", "0" );
	r_cache = ri.Cvar_Get( "r_cache", "1", CVAR_LATCH2 );  // leaving it as this for backwards compability. but it caches models and shaders also
// (SA) disabling cacheshaders
	ri.Cvar_Set( "r_cacheShaders", "0" );
	r_cacheShaders = ri.Cvar_Get( "r_cacheShaders", "0", CVAR_LATCH2 );
//----(SA)	end

	r_cacheModels = ri.Cvar_Get( "r_cacheModels", "1", CVAR_LATCH2 );
	r_compressModels = ri.Cvar_Get( "r_compressModels", "0", 0 );     // converts MD3 -> MDC at run-time
	r_exportCompressedModels = ri.Cvar_Get( "r_exportCompressedModels", "0", 0 ); // saves compressed models
	r_cacheGathering = ri.Cvar_Get( "cl_cacheGathering", "0", 0 );
	r_buildScript = ri.Cvar_Get( "com_buildscript", "0", 0 );
	r_bonesDebug = ri.Cvar_Get( "r_bonesDebug", "0", CVAR_CHEAT );
	// done.

	// Rafael - wolf fog
	r_wolffog = ri.Cvar_Get( "r_wolffog", "1", 0 );
	// done

	r_portalsky = ri.Cvar_Get( "cg_skybox", "1", 0 );

	r_maxpolys = ri.Cvar_Get( "r_maxpolys", va( "%d", MAX_POLYS ), 0 );
	r_maxpolyverts = ri.Cvar_Get( "r_maxpolyverts", va( "%d", MAX_POLYVERTS ), 0 );

	r_highQualityVideo = ri.Cvar_Get( "r_highQualityVideo", "1", CVAR_ARCHIVE );
	// make sure all the commands added here are also
	// removed in R_Shutdown
	ri.Cmd_AddCommand( "imagelist", R_ImageList_f );
	ri.Cmd_AddCommand( "shaderlist", R_ShaderList_f );
	ri.Cmd_AddCommand( "skinlist", R_SkinList_f );
	ri.Cmd_AddCommand( "modellist", R_Modellist_f );
	ri.Cmd_AddCommand( "modelist", R_ModeList_f );
	ri.Cmd_AddCommand( "screenshot", R_ScreenShot_f );
	ri.Cmd_AddCommand( "screenshotJPEG", R_ScreenShotJPEG_f );
	ri.Cmd_AddCommand( "gfxinfo", GfxInfo_f );
	ri.Cmd_AddCommand( "taginfo", R_TagInfo_f );

	// Ridah
	ri.Cmd_AddCommand( "cropimages", R_CropImages_f );
	// done.
}

/*
===============
R_Init
===============
*/
void R_Init( void ) {
	int err;
	int i;

	ri.Printf( PRINT_ALL, "----- R_Init -----\n" );

	// clear all our internal state
	memset( &tr, 0, sizeof( tr ) );
	memset( &backEnd, 0, sizeof( backEnd ) );
	memset( &tess, 0, sizeof( tess ) );

	if ( (qintptr)tess.xyz & 15 ) {
		Com_Printf( "WARNING: tess.xyz not 16 byte aligned\n" );
	}
	memset( tess.constantColor255, 255, sizeof( tess.constantColor255 ) );

	//
	// init function tables
	//
	for ( i = 0; i < FUNCTABLE_SIZE; i++ )
	{
		tr.sinTable[i]      = sin( DEG2RAD( i * 360.0f / ( ( float ) ( FUNCTABLE_SIZE - 1 ) ) ) );
		tr.squareTable[i]   = ( i < FUNCTABLE_SIZE / 2 ) ? 1.0f : -1.0f;
		tr.sawToothTable[i] = (float)i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

		if ( i < FUNCTABLE_SIZE / 2 ) {
			if ( i < FUNCTABLE_SIZE / 4 ) {
				tr.triangleTable[i] = ( float ) i / ( FUNCTABLE_SIZE / 4 );
			} else
			{
				tr.triangleTable[i] = 1.0f - tr.triangleTable[i - FUNCTABLE_SIZE / 4];
			}
		} else
		{
			tr.triangleTable[i] = -tr.triangleTable[i - FUNCTABLE_SIZE / 2];
		}
	}

	R_InitFogTable();

	R_NoiseInit();

	R_Register();

	// Ridah, init the virtual memory
	R_Hunk_Begin();

	max_polys = r_maxpolys->integer;
	if ( max_polys < MAX_POLYS ) {
		max_polys = MAX_POLYS;
	}

	max_polyverts = r_maxpolyverts->integer;
	if ( max_polyverts < MAX_POLYVERTS ) {
		max_polyverts = MAX_POLYVERTS;
	}

//	backEndData[0] = ri.Hunk_Alloc( sizeof( *backEndData[0] ), h_low );
	backEndData[0] = (backEndData_t*)ri.Hunk_Alloc( sizeof( *backEndData[0] ) + sizeof( srfPoly_t ) * max_polys + sizeof( polyVert_t ) * max_polyverts, h_low );

	if ( r_smp->integer ) {
//		backEndData[1] = ri.Hunk_Alloc( sizeof( *backEndData[1] ), h_low );
		backEndData[1] = (backEndData_t*)ri.Hunk_Alloc( sizeof( *backEndData[1] ) + sizeof( srfPoly_t ) * max_polys + sizeof( polyVert_t ) * max_polyverts, h_low );
	} else {
		backEndData[1] = NULL;
	}
	R_ToggleSmpFrame();

	InitOpenGL();

	R_InitImages();



	R_InitShaders();

	R_InitSkins();

	R_ModelInit();

	R_InitFreeType();

	RB_ZombieFXInit();

	err = qglGetError();
	if ( err != GL_NO_ERROR ) {
		ri.Printf( PRINT_ALL, "glGetError() = 0x%x\n", err );
	}

	ri.Printf( PRINT_ALL, "----- finished R_Init -----\n" );
}

/*
===============
RE_Shutdown
===============
*/
void RE_Shutdown( qboolean destroyWindow ) {

	ri.Printf( PRINT_ALL, "RE_Shutdown( %i )\n", destroyWindow );

	ri.Cmd_RemoveCommand( "modellist" );
	ri.Cmd_RemoveCommand( "screenshotJPEG" );
	ri.Cmd_RemoveCommand( "screenshot" );
	ri.Cmd_RemoveCommand( "imagelist" );
	ri.Cmd_RemoveCommand( "shaderlist" );
	ri.Cmd_RemoveCommand( "skinlist" );
	ri.Cmd_RemoveCommand( "gfxinfo" );
	ri.Cmd_RemoveCommand( "modelist" );
	ri.Cmd_RemoveCommand( "shaderstate" );
	ri.Cmd_RemoveCommand( "taginfo" );

	// Ridah
	ri.Cmd_RemoveCommand( "cropimages" );
	// done.

	R_ShutdownCommandBuffers();

	// Ridah, keep a backup of the current images if possible
	// clean out any remaining unused media from the last backup
	R_PurgeShaders( 9999999 );
	R_PurgeBackupImages( 9999999 );
	R_PurgeModels( 9999999 );

	if ( r_cache->integer ) {
		if ( tr.registered ) {
			if ( destroyWindow ) {
				R_SyncRenderThread();
				R_ShutdownCommandBuffers();
				R_DeleteTextures();
			} else {
				// backup the current media
				R_ShutdownCommandBuffers();

				R_BackupModels();
				R_BackupShaders();
				R_BackupImages();
			}
		}
	} else if ( tr.registered ) {
		R_SyncRenderThread();
		R_ShutdownCommandBuffers();
		R_DeleteTextures();
	}

	R_DoneFreeType();

	// shut down platform specific OpenGL stuff
	if ( destroyWindow ) {
		GLimp_Shutdown();

		// Ridah, release the virtual memory
		R_Hunk_End();
		R_FreeImageBuffer();
		//ri.Tag_Free();	// wipe all render alloc'd zone memory
	}

	tr.registered = qfalse;
}


/*
=============
RE_EndRegistration

Touch all images to make sure they are resident
=============
*/
void RE_EndRegistration( void ) {
	R_SyncRenderThread();
	if ( !Sys_LowPhysicalMemory() ) {
		RB_ShowImages();
	}
}


/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
refexport_t *GetRefAPI( int apiVersion, refimport_t *rimp ) {
	static refexport_t re;

	ri = *rimp;

	memset( &re, 0, sizeof( re ) );

	if ( apiVersion != REF_API_VERSION ) {
		ri.Printf( PRINT_ALL, "Mismatched REF_API_VERSION: expected %i, got %i\n",
				   REF_API_VERSION, apiVersion );
		return NULL;
	}

	// the RE_ functions are Renderer Entry points

	re.Shutdown = RE_Shutdown;

	re.BeginRegistration = RE_BeginRegistration;
	re.RegisterModel    = RE_RegisterModel;
	re.RegisterSkin     = RE_RegisterSkin;
//----(SA) added
	re.GetSkinModel         = RE_GetSkinModel;
	re.GetShaderFromModel   = RE_GetShaderFromModel;
//----(SA) end
	re.RegisterShader   = RE_RegisterShader;
	re.RegisterShaderNoMip = RE_RegisterShaderNoMip;
	re.LoadWorld        = RE_LoadWorldMap;
	re.SetWorldVisData  = RE_SetWorldVisData;
	re.EndRegistration  = RE_EndRegistration;

	re.BeginFrame       = RE_BeginFrame;
	re.EndFrame         = RE_EndFrame;

	re.MarkFragments    = R_MarkFragments;
	re.LerpTag          = R_LerpTag;
	re.ModelBounds      = R_ModelBounds;

	re.ClearScene       = RE_ClearScene;
	re.AddRefEntityToScene = RE_AddRefEntityToScene;
	re.AddPolyToScene   = RE_AddPolyToScene;
	// Ridah
	re.AddPolysToScene  = RE_AddPolysToScene;
	// done.
	re.AddLightToScene  = RE_AddLightToScene;
//----(SA)
	re.AddCoronaToScene = RE_AddCoronaToScene;
	re.SetFog           = R_SetFog;
//----(SA)
	re.RenderScene      = RE_RenderScene;

	re.SetColor         = RE_SetColor;
	re.DrawStretchPic   = RE_StretchPic;
	re.DrawStretchPicGradient   = RE_StretchPicGradient;
	re.DrawStretchRaw   = RE_StretchRaw;
	re.UploadCinematic  = RE_UploadCinematic;
	re.RegisterFont     = RE_RegisterFont;
	re.RemapShader      = R_RemapShader;
	re.GetEntityToken   = R_GetEntityToken;

	// RF
	re.ZombieFXAddNewHit = RB_ZombieFXAddNewHit;

	return &re;
}

