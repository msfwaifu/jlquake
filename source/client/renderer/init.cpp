//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "../client.h"
#include "local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

struct vidmode_t
{
	const char*	description;
	int         width;
	int			height;
	float		pixelAspect;		// pixel width / height
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

Cvar*		r_logFile;

Cvar*		r_mode;
Cvar*		r_fullscreen;
Cvar*		r_customwidth;
Cvar*		r_customheight;
Cvar*		r_customaspect;

Cvar*		r_allowSoftwareGL;		// don't abort out if the pixelformat claims software
Cvar*		r_stencilbits;
Cvar*		r_depthbits;
Cvar*		r_colorbits;
Cvar*		r_stereo;
Cvar*		r_displayRefresh;
Cvar*		r_swapInterval;
Cvar*		r_drawBuffer;

Cvar*		r_verbose;

Cvar*		r_ext_compressed_textures;
Cvar*		r_ext_multitexture;
Cvar*		r_ext_texture_env_add;
Cvar*		r_ext_gamma_control;
Cvar*		r_ext_compiled_vertex_array;
Cvar*		r_ext_point_parameters;

Cvar*		r_gamma;
Cvar*		r_ignorehwgamma;
Cvar*		r_intensity;
Cvar*		r_overBrightBits;

Cvar*		r_wateralpha;
Cvar*		r_roundImagesDown;
Cvar*		r_picmip;
Cvar*		r_texturebits;
Cvar*		r_colorMipLevels;
Cvar*		r_simpleMipMaps;
Cvar*		r_textureMode;

Cvar*		r_ignoreGLErrors;

Cvar*		r_nobind;

Cvar*		r_mapOverBrightBits;
Cvar*		r_vertexLight;
Cvar*		r_lightmap;
Cvar*		r_fullbright;
Cvar*		r_singleShader;

Cvar*		r_subdivisions;

Cvar*		r_ignoreFastPath;
Cvar*		r_detailTextures;
Cvar*		r_uiFullScreen;
Cvar*		r_printShaders;

Cvar*		r_saveFontData;

Cvar*		r_smp;
Cvar*		r_maxpolys;
Cvar*		r_maxpolyverts;

Cvar*		r_dynamiclight;
Cvar*		r_znear;

Cvar*		r_nocull;

Cvar*		r_primitives;
Cvar*		r_vertex_arrays;
Cvar*		r_lerpmodels;
Cvar*		r_shadows;
Cvar*		r_debugSort;
Cvar*		r_showtris;
Cvar*		r_shownormals;
Cvar*		r_offsetFactor;
Cvar*		r_offsetUnits;
Cvar*		r_clear;

Cvar*		r_modulate;
Cvar*		r_ambientScale;
Cvar*		r_directedScale;
Cvar*		r_debugLight;

Cvar*		r_lightlevel;	// FIXME: This is a HACK to get the client's light level

Cvar*		r_lodbias;
Cvar*		r_lodscale;

Cvar*		r_skymip;
Cvar*		r_fastsky;
Cvar*		r_showsky;
Cvar*		r_drawSun;

Cvar*		r_lodCurveError;

Cvar*		r_ignore;

Cvar*		r_keeptjunctions;
Cvar*		r_texsort;
Cvar*		r_dynamic;
Cvar*		r_saturatelighting;

Cvar*		r_nocurves;
Cvar*		r_facePlaneCull;
Cvar*		r_novis;
Cvar*		r_lockpvs;
Cvar*		r_showcluster;
Cvar*		r_drawworld;
Cvar*		r_measureOverdraw;
Cvar*		r_finish;
Cvar*		r_showImages;
Cvar*		r_speeds;
Cvar*		r_showSmp;
Cvar*		r_skipBackEnd;
Cvar*		r_drawentities;
Cvar*		r_debugSurface;
Cvar*		r_norefresh;

Cvar*		r_railWidth;
Cvar*		r_railCoreWidth;
Cvar*		r_railSegmentLength;

Cvar*		r_flares;
Cvar*		r_flareSize;
Cvar*		r_flareFade;

Cvar*		r_particle_size;
Cvar*		r_particle_min_size;
Cvar*		r_particle_max_size;
Cvar*		r_particle_att_a;
Cvar*		r_particle_att_b;
Cvar*		r_particle_att_c;

Cvar*		r_noportals;
Cvar*		r_portalOnly;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static vidmode_t r_vidModes[] =
{
	{ "Mode  0: 320x240",		320,	240,	1 },
	{ "Mode  1: 400x300",		400,	300,	1 },
	{ "Mode  2: 512x384",		512,	384,	1 },
	{ "Mode  3: 640x480",		640,	480,	1 },
	{ "Mode  4: 800x600",		800,	600,	1 },
	{ "Mode  5: 960x720",		960,	720,	1 },
	{ "Mode  6: 1024x768",		1024,	768,	1 },
	{ "Mode  7: 1152x864",		1152,	864,	1 },
	{ "Mode  8: 1280x1024",		1280,	1024,	1 },
	{ "Mode  9: 1600x1200",		1600,	1200,	1 },
	{ "Mode 10: 2048x1536",		2048,	1536,	1 },
	{ "Mode 11: 856x480 (wide)",856,	480,	1 }
};
static int		s_numVidModes = sizeof(r_vidModes) / sizeof(r_vidModes[0]);

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	R_GetModeInfo
//
//==========================================================================

bool R_GetModeInfo(int* width, int* height, float* windowAspect, int mode)
{
    if (mode < -1)
	{
        return false;
	}
	if (mode >= s_numVidModes)
	{
		return false;
	}

	if (mode == -1)
	{
		*width = r_customwidth->integer;
		*height = r_customheight->integer;
		*windowAspect = r_customaspect->value;
		return true;
	}

	vidmode_t* vm = &r_vidModes[mode];

	*width  = vm->width;
	*height = vm->height;
	*windowAspect = (float)vm->width / (vm->height * vm->pixelAspect);

	return true;
}

//==========================================================================
//
//	R_ModeList_f
//
//==========================================================================

static void R_ModeList_f()
{
	Log::write("\n");
	for (int i = 0; i < s_numVidModes; i++ )
	{
		Log::write("%s\n", r_vidModes[i].description);
	}
	Log::write("\n");
}

//==========================================================================
//
//	AssertCvarRange
//
//==========================================================================

static void AssertCvarRange(Cvar* cv, float minVal, float maxVal, bool shouldBeIntegral)
{
	if (shouldBeIntegral)
	{
		if ((int)cv->value != cv->integer)
		{
			Log::write(S_COLOR_YELLOW "WARNING: cvar '%s' must be integral (%f)\n", cv->name, cv->value);
			Cvar_Set(cv->name, va("%d", cv->integer));
		}
	}

	if (cv->value < minVal)
	{
		Log::write(S_COLOR_YELLOW "WARNING: cvar '%s' out of range (%f < %f)\n", cv->name, cv->value, minVal);
		Cvar_Set(cv->name, va("%f", minVal));
	}
	else if (cv->value > maxVal)
	{
		Log::write(S_COLOR_YELLOW "WARNING: cvar '%s' out of range (%f > %f)\n", cv->name, cv->value, maxVal);
		Cvar_Set(cv->name, va("%f", maxVal));
	}
}

//==========================================================================
//
//	GfxInfo_f
//
//==========================================================================

static void GfxInfo_f()
{
	const char* fsstrings[] =
	{
		"windowed",
		"fullscreen"
	};
	const char* enablestrings[] =
	{
		"disabled",
		"enabled"
	};

	Log::write("\nGL_VENDOR: %s\n", glConfig.vendor_string);
	Log::write("GL_RENDERER: %s\n", glConfig.renderer_string);
	Log::write("GL_VERSION: %s\n", glConfig.version_string);

	Log::writeLine("GL_EXTENSIONS:");
	Array<String> Exts;
	String(glConfig.extensions_string).Split(' ', Exts);
	for (int i = 0; i < Exts.Num(); i++)
	{
		Log::writeLine(" %s", *Exts[i]);
	}

	Log::write("GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize);
	Log::write("GL_MAX_ACTIVE_TEXTURES: %d\n", glConfig.maxActiveTextures);
	Log::write("\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits);
	Log::write("MODE: %d, %d x %d %s hz:", r_mode->integer, glConfig.vidWidth, glConfig.vidHeight, fsstrings[r_fullscreen->integer == 1]);
	if (glConfig.displayFrequency)
	{
		Log::write("%d\n", glConfig.displayFrequency);
	}
	else
	{
		Log::write("N/A\n");
	}
	if (glConfig.deviceSupportsGamma)
	{
		Log::write("GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits);
	}
	else
	{
		Log::write("GAMMA: software w/ %d overbright bits\n", tr.overbrightBits);
	}

	// rendering primitives
	// default is to use triangles if compiled vertex arrays are present
	Log::write("rendering primitives: ");
	int primitives = r_primitives->integer;
	if (primitives == 0)
	{
		if (qglLockArraysEXT)
		{
			primitives = 2;
		}
		else
		{
			primitives = 1;
		}
	}
	if (primitives == -1)
	{
		Log::write("none\n");
	}
	else if (primitives == 2)
	{
		Log::write("single glDrawElements\n");
	}
	else if (primitives == 1)
	{
		Log::write("multiple glArrayElement\n");
	}
	else if (primitives == 3)
	{
		Log::write("multiple glColor4ubv + glTexCoord2fv + glVertex3fv\n");
	}

	Log::write("texturemode: %s\n", r_textureMode->string);
	Log::write("picmip: %d\n", r_picmip->integer);
	Log::write("texture bits: %d\n", r_texturebits->integer);
	Log::write("multitexture: %s\n", enablestrings[qglActiveTextureARB != 0]);
	Log::write("compiled vertex arrays: %s\n", enablestrings[qglLockArraysEXT != 0 ]);
	Log::write("texenv add: %s\n", enablestrings[glConfig.textureEnvAddAvailable != 0]);
	Log::write("compressed textures: %s\n", enablestrings[glConfig.textureCompression != TC_NONE]);
	if (r_vertexLight->integer)
	{
		Log::write("HACK: using vertex lightmap approximation\n");
	}
	if (glConfig.smpActive)
	{
		Log::write("Using dual processor acceleration\n");
	}
	if (r_finish->integer)
	{
		Log::write("Forcing glFinish\n");
	}
}

//==========================================================================
//
//	R_Register
//
//==========================================================================

static void R_Register() 
{
	//
	// latched and archived variables
	//
	r_mode = Cvar_Get("r_mode", "3", CVAR_ARCHIVE | CVAR_LATCH2);
	r_fullscreen = Cvar_Get("r_fullscreen", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_customwidth = Cvar_Get("r_customwidth", "1600", CVAR_ARCHIVE | CVAR_LATCH2);
	r_customheight = Cvar_Get("r_customheight", "1024", CVAR_ARCHIVE | CVAR_LATCH2);
	r_customaspect = Cvar_Get("r_customaspect", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_colorbits = Cvar_Get("r_colorbits", "0", CVAR_ARCHIVE | CVAR_LATCH2);
	r_stencilbits = Cvar_Get("r_stencilbits", "8", CVAR_ARCHIVE | CVAR_LATCH2);
	r_depthbits = Cvar_Get("r_depthbits", "0", CVAR_ARCHIVE | CVAR_LATCH2);
	r_stereo = Cvar_Get("r_stereo", "0", CVAR_ARCHIVE | CVAR_LATCH2);
	r_ext_compressed_textures = Cvar_Get("r_ext_compressed_textures", "0", CVAR_ARCHIVE | CVAR_LATCH2);
	r_ext_multitexture = Cvar_Get("r_ext_multitexture", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_ext_texture_env_add = Cvar_Get("r_ext_texture_env_add", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_ext_gamma_control = Cvar_Get("r_ext_gamma_control", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_ext_compiled_vertex_array = Cvar_Get("r_ext_compiled_vertex_array", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_ext_point_parameters = Cvar_Get("r_ext_point_parameters", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_ignorehwgamma = Cvar_Get("r_ignorehwgamma", "0", CVAR_ARCHIVE | CVAR_LATCH2);
	r_overBrightBits = Cvar_Get("r_overBrightBits", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_roundImagesDown = Cvar_Get("r_roundImagesDown", "0", CVAR_ARCHIVE | CVAR_LATCH2);
	r_picmip = Cvar_Get("r_picmip", (GGameType & GAME_Quake3) ? "1" : "0", CVAR_ARCHIVE | CVAR_LATCH2);
	AssertCvarRange(r_picmip, 0, 16, true);
	r_texturebits = Cvar_Get("r_texturebits", "0", CVAR_ARCHIVE | CVAR_LATCH2);
	r_simpleMipMaps = Cvar_Get("r_simpleMipMaps", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_vertexLight = Cvar_Get("r_vertexLight", "0", CVAR_ARCHIVE | CVAR_LATCH2);
	r_subdivisions = Cvar_Get("r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH2);
	r_ignoreFastPath = Cvar_Get("r_ignoreFastPath", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	r_detailTextures = Cvar_Get("r_detailtextures", "1", CVAR_ARCHIVE | CVAR_LATCH2);
	//	Disabled until I fix it on Linux.
	r_smp = Cvar_Get("r_smp", "0", CVAR_ARCHIVE | CVAR_LATCH2);

	//
	// temporary latched variables that can only change over a restart
	//
	r_allowSoftwareGL = Cvar_Get("r_allowSoftwareGL", "0", CVAR_LATCH2);
	r_displayRefresh = Cvar_Get("r_displayRefresh", "0", CVAR_LATCH2);
	AssertCvarRange(r_displayRefresh, 0, 200, true);
	r_intensity = Cvar_Get("r_intensity", "1", CVAR_LATCH2);
	r_colorMipLevels = Cvar_Get("r_colorMipLevels", "0", CVAR_LATCH2);
	r_mapOverBrightBits = Cvar_Get("r_mapOverBrightBits", "2", CVAR_LATCH2);
	r_fullbright = Cvar_Get("r_fullbright", "0", CVAR_LATCH2 | CVAR_CHEAT);
	r_singleShader = Cvar_Get("r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH2);

	//
	// archived variables that can change at any time
	//
	r_swapInterval = Cvar_Get("r_swapInterval", "0", CVAR_ARCHIVE);
	r_gamma = Cvar_Get("r_gamma", "1", CVAR_ARCHIVE);
	r_wateralpha = Cvar_Get("r_wateralpha","0.4", CVAR_ARCHIVE);
	r_ignoreGLErrors = Cvar_Get("r_ignoreGLErrors", "1", CVAR_ARCHIVE);
	r_textureMode = Cvar_Get("r_textureMode", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE);
	r_dynamiclight = Cvar_Get("r_dynamiclight", "1", CVAR_ARCHIVE);
	r_primitives = Cvar_Get("r_primitives", "0", CVAR_ARCHIVE);
	r_vertex_arrays = Cvar_Get("r_vertex_arrays", "0", CVAR_ARCHIVE);
	r_modulate = Cvar_Get("r_modulate", "1", CVAR_ARCHIVE);
	if (GGameType & GAME_Quake3)
	{
		r_shadows = Cvar_Get("cg_shadows", "1", 0);
	}
	else
	{
		r_shadows = Cvar_Get("r_shadows", "0", CVAR_ARCHIVE);
	}
	r_lodbias = Cvar_Get("r_lodbias", "0", CVAR_ARCHIVE);
	r_fastsky = Cvar_Get("r_fastsky", "0", CVAR_ARCHIVE);
	r_drawSun = Cvar_Get("r_drawSun", "0", CVAR_ARCHIVE);
	r_lodCurveError = Cvar_Get("r_lodCurveError", "250", CVAR_ARCHIVE | CVAR_CHEAT);
	r_keeptjunctions = Cvar_Get("r_keeptjunctions", "1", CVAR_ARCHIVE);
	r_facePlaneCull = Cvar_Get("r_facePlaneCull", "1", CVAR_ARCHIVE);
	r_railWidth = Cvar_Get("r_railWidth", "16", CVAR_ARCHIVE);
	r_railCoreWidth = Cvar_Get("r_railCoreWidth", "6", CVAR_ARCHIVE);
	r_railSegmentLength = Cvar_Get("r_railSegmentLength", "32", CVAR_ARCHIVE);
	r_flares = Cvar_Get("r_flares", "0", CVAR_ARCHIVE);
	r_finish = Cvar_Get("r_finish", "0", CVAR_ARCHIVE);
	r_particle_size = Cvar_Get("r_particle_size", "40", CVAR_ARCHIVE);
	r_particle_min_size = Cvar_Get("r_particle_min_size", "2", CVAR_ARCHIVE);
	r_particle_max_size = Cvar_Get("r_particle_max_size", "40", CVAR_ARCHIVE);
	r_particle_att_a = Cvar_Get("r_particle_att_a", "0.01", CVAR_ARCHIVE);
	r_particle_att_b = Cvar_Get("r_particle_att_b", "0.0", CVAR_ARCHIVE);
	r_particle_att_c = Cvar_Get("r_particle_att_c", "0.01", CVAR_ARCHIVE);

	//
	// temporary variables that can change at any time
	//
	r_logFile = Cvar_Get("r_logFile", "0", CVAR_CHEAT);
	r_verbose = Cvar_Get("r_verbose", "0", CVAR_CHEAT);
	r_nobind = Cvar_Get("r_nobind", "0", CVAR_CHEAT);
	r_lightmap = Cvar_Get("r_lightmap", "0", 0);
	r_uiFullScreen = Cvar_Get("r_uifullscreen", "0", 0);
	r_printShaders = Cvar_Get("r_printShaders", "0", 0);
	r_saveFontData = Cvar_Get("r_saveFontData", "0", 0);
	r_maxpolys = Cvar_Get("r_maxpolys", va("%d", MAX_POLYS), 0);
	r_maxpolyverts = Cvar_Get("r_maxpolyverts", va("%d", MAX_POLYVERTS), 0);
	r_znear = Cvar_Get("r_znear", "4", CVAR_CHEAT);
	AssertCvarRange(r_znear, 0.001f, 200, true);
	r_nocull = Cvar_Get("r_nocull", "0", CVAR_CHEAT);
	r_lightlevel = Cvar_Get("r_lightlevel", "0", 0);
	r_lerpmodels = Cvar_Get("r_lerpmodels", "1", 0);
	r_ambientScale = Cvar_Get("r_ambientScale", "0.6", CVAR_CHEAT);
	r_directedScale = Cvar_Get("r_directedScale", "1", CVAR_CHEAT);
	r_debugLight = Cvar_Get("r_debuglight", "0", CVAR_TEMP);
	r_debugSort = Cvar_Get("r_debugSort", "0", CVAR_CHEAT);
	r_showtris = Cvar_Get("r_showtris", "0", CVAR_CHEAT);
	r_shownormals = Cvar_Get("r_shownormals", "0", CVAR_CHEAT);
	r_offsetFactor = Cvar_Get("r_offsetfactor", "-1", CVAR_CHEAT);
	r_offsetUnits = Cvar_Get("r_offsetunits", "-2", CVAR_CHEAT);
	r_lodscale = Cvar_Get("r_lodscale", "5", CVAR_CHEAT);
	r_clear = Cvar_Get("r_clear", "0", CVAR_CHEAT);
	r_skymip = Cvar_Get("r_skymip", "0", 0);
	r_showsky = Cvar_Get("r_showsky", "0", CVAR_CHEAT);
	r_ignore = Cvar_Get("r_ignore", "1", CVAR_CHEAT);
	r_texsort = Cvar_Get("r_texsort", "1", 0);
	r_dynamic = Cvar_Get("r_dynamic", "1", 0);
	r_saturatelighting = Cvar_Get("r_saturatelighting", "0", 0);
	r_nocurves = Cvar_Get("r_nocurves", "0", CVAR_CHEAT);
	r_novis = Cvar_Get("r_novis", "0", CVAR_CHEAT);
	r_lockpvs = Cvar_Get("r_lockpvs", "0", CVAR_CHEAT);
	r_showcluster = Cvar_Get("r_showcluster", "0", CVAR_CHEAT);
	r_drawworld = Cvar_Get("r_drawworld", "1", CVAR_CHEAT);
	r_flareSize = Cvar_Get("r_flareSize", "40", CVAR_CHEAT);
	r_flareFade = Cvar_Get("r_flareFade", "7", CVAR_CHEAT);
	r_measureOverdraw = Cvar_Get("r_measureOverdraw", "0", CVAR_CHEAT);
	r_showImages = Cvar_Get("r_showImages", "0", CVAR_TEMP);
	r_drawBuffer = Cvar_Get("r_drawBuffer", "GL_BACK", CVAR_CHEAT);
	r_speeds = Cvar_Get("r_speeds", "0", CVAR_CHEAT);
	r_showSmp = Cvar_Get("r_showSmp", "0", CVAR_CHEAT);
	r_skipBackEnd = Cvar_Get("r_skipBackEnd", "0", CVAR_CHEAT);
	r_drawentities = Cvar_Get("r_drawentities", "1", CVAR_CHEAT);
	r_noportals = Cvar_Get("r_noportals", "0", CVAR_CHEAT);
	r_portalOnly = Cvar_Get("r_portalOnly", "0", CVAR_CHEAT);
	r_debugSurface = Cvar_Get("r_debugSurface", "0", CVAR_CHEAT);
	r_norefresh = Cvar_Get("r_norefresh", "0", CVAR_CHEAT);

	// make sure all the commands added here are also
	// removed in R_Shutdown
	Cmd_AddCommand("modelist", R_ModeList_f);
	Cmd_AddCommand("imagelist", R_ImageList_f);
	Cmd_AddCommand("shaderlist", R_ShaderList_f);
	Cmd_AddCommand("skinlist", R_SkinList_f);
	Cmd_AddCommand("screenshot", R_ScreenShot_f);
	Cmd_AddCommand("screenshotJPEG", R_ScreenShotJPEG_f);
	Cmd_AddCommand("gfxinfo", GfxInfo_f);
	Cmd_AddCommand("modellist", R_Modellist_f);
}

//==========================================================================
//
//	R_GetTitleForWindow
//
//==========================================================================

const char* R_GetTitleForWindow()
{
	if (GGameType & GAME_QuakeWorld)
	{
		return "QuakeWorld";
	}
	if (GGameType & GAME_Quake)
	{
		return "Quake";
	}
	if (GGameType & GAME_HexenWorld)
	{
		return "HexenWorld";
	}
	if (GGameType & GAME_Hexen2)
	{
		return "Hexen II";
	}
	if (GGameType & GAME_Quake2)
	{
		return "Quake 2";
	}
	if (GGameType & GAME_Quake3)
	{
		return "Quake 3: Arena";
	}
	return "Unknown";
}

//==========================================================================
//
//	R_SetMode
//
//==========================================================================

static void R_SetMode()
{
	rserr_t err = GLimp_SetMode(r_mode->integer, r_colorbits->integer, !!r_fullscreen->integer);
	if (err == RSERR_OK)
	{
		return;
	}

	if (err == RSERR_INVALID_FULLSCREEN)
	{
		Log::write("...WARNING: fullscreen unavailable in this mode\n");

		Cvar_SetValue("r_fullscreen", 0);
		r_fullscreen->modified = false;

		err = GLimp_SetMode(r_mode->integer, r_colorbits->integer, false);
		if (err == RSERR_OK)
		{
			return;
		}
	}

	Log::write("...WARNING: could not set the given mode (%d)\n", r_mode->integer);

	// if we're on a 24/32-bit desktop and we're going fullscreen on an ICD,
	// try it again but with a 16-bit desktop
	if (r_colorbits->integer != 16 || r_fullscreen->integer == 0 || r_mode->integer != 3)
	{
		err = GLimp_SetMode(3, 16, true);
		if (err == RSERR_OK)
		{
			return;
		}
		Log::write("...WARNING: could not set default 16-bit fullscreen mode\n");
	}

	// try setting it back to something safe
	err = GLimp_SetMode(3, r_colorbits->integer, false);
	if (err == RSERR_OK)
	{
		return;
	}

	Log::write("...WARNING: could not revert to safe mode\n");
	throw Exception("R_SetMode() - could not initialise OpenGL subsystem\n" );
}

//==========================================================================
//
//	InitOpenGLSubsystem
//
//	This is the OpenGL initialization function.  It is responsible for
// initialising OpenGL, setting extensions, creating a window of the
// appropriate size, doing fullscreen manipulations, etc.  Its overall
// responsibility is to make sure that a functional OpenGL subsystem is
// operating when it returns.
//
//==========================================================================

static void InitOpenGLSubsystem()
{	
	Log::write("Initializing OpenGL subsystem\n");

	//	Ceate the window and set up the context.
	R_SetMode();

	// 	Initialise our QGL dynamic bindings
	QGL_Init();

	//	Needed for Quake 3 UI vm.
	glConfig.driverType = GLDRV_ICD;
	glConfig.hardwareType = GLHW_GENERIC;

	//	Get our config strings.
	String::NCpyZ(glConfig.vendor_string, (char*)qglGetString(GL_VENDOR), sizeof(glConfig.vendor_string));
	String::NCpyZ(glConfig.renderer_string, (char*)qglGetString(GL_RENDERER), sizeof(glConfig.renderer_string));
	String::NCpyZ(glConfig.version_string, (char*)qglGetString(GL_VERSION), sizeof(glConfig.version_string));
	String::NCpyZ(glConfig.extensions_string, (char*)qglGetString(GL_EXTENSIONS), sizeof(glConfig.extensions_string));

	// OpenGL driver constants
	GLint temp;
	qglGetIntegerv(GL_MAX_TEXTURE_SIZE, &temp);
	glConfig.maxTextureSize = temp;

	//	Load palette used by 8-bit graphics files.
	if (GGameType & GAME_QuakeHexen)
	{
		R_InitQ1Palette();
	}
	else if (GGameType & GAME_Quake2)
	{
		R_InitQ2Palette();
	}

	if (qglActiveTextureARB)
	{
		Cvar_SetValue("r_texsort", 0.0);
	}
}

//==========================================================================
//
//	GL_SetDefaultState
//
//==========================================================================

static void GL_SetDefaultState()
{
	qglClearDepth(1.0f);

	qglColor4f(1, 1, 1, 1);

	// initialize downstream texture unit if we're running
	// in a multitexture environment
	if (qglActiveTextureARB)
	{
		GL_SelectTexture(1);
		GL_TextureMode(r_textureMode->string);
		GL_TexEnv(GL_MODULATE);
		qglDisable(GL_TEXTURE_2D);
		GL_SelectTexture(0);
	}

	qglEnable(GL_TEXTURE_2D);
	GL_TextureMode(r_textureMode->string);
	GL_TexEnv(GL_MODULATE);

	qglShadeModel(GL_SMOOTH);
	qglDepthFunc(GL_LEQUAL);

	// the vertex array is always enabled, but the color and texture
	// arrays are enabled and disabled around the compiled vertex array call
	qglEnableClientState(GL_VERTEX_ARRAY);

	//
	// make sure our GL state vector is set correctly
	//
	glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;
	glState.faceCulling = CT_TWO_SIDED;

	qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	qglDepthMask(GL_TRUE);
	qglDisable(GL_DEPTH_TEST);
	qglEnable(GL_SCISSOR_TEST);
	qglDisable(GL_CULL_FACE);
	qglDisable(GL_BLEND);

	if (qglPointParameterfEXT)
	{
		float attenuations[3];

		attenuations[0] = r_particle_att_a->value;
		attenuations[1] = r_particle_att_b->value;
		attenuations[2] = r_particle_att_c->value;

		qglEnable(GL_POINT_SMOOTH);
		qglPointParameterfEXT(GL_POINT_SIZE_MIN_EXT, r_particle_min_size->value);
		qglPointParameterfEXT(GL_POINT_SIZE_MAX_EXT, r_particle_max_size->value);
		qglPointParameterfvEXT(GL_DISTANCE_ATTENUATION_EXT, attenuations);
	}
}

//==========================================================================
//
//	InitOpenGL
//
//	This function is responsible for initializing a valid OpenGL subsystem.
// This is done by calling InitOpenGLSubsystem (which gives us a working OGL
// subsystem) then setting variables, checking GL constants, and reporting
// the gfx system config to the user.
//
//==========================================================================

static void InitOpenGL()
{	
	//
	// initialize OS specific portions of the renderer
	//
	// InitOpenGLSubsystem directly or indirectly references the following cvars:
	//		- r_fullscreen
	//		- r_mode
	//		- r_(color|depth|stencil)bits
	//		- r_ignorehwgamma
	//		- r_gamma
	//
	
	if (glConfig.vidWidth == 0)
	{
		InitOpenGLSubsystem();
	}

	// init command buffers and SMP
	R_InitCommandBuffers();

	// print info
	GfxInfo_f();

	// set default state
	GL_SetDefaultState();
}

//==========================================================================
//
//	R_InitFunctionTables
//
//==========================================================================

static void R_InitFunctionTables()
{
	//
	// init function tables
	//
	for (int i = 0; i < FUNCTABLE_SIZE; i++)
	{
		tr.sinTable[i] = sin(DEG2RAD(i * 360.0f / ((float)FUNCTABLE_SIZE)));
		tr.squareTable[i] = (i < FUNCTABLE_SIZE / 2) ? 1.0f : -1.0f;
		tr.sawToothTable[i] = (float)i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

		if (i < FUNCTABLE_SIZE / 2)
		{
			if (i < FUNCTABLE_SIZE / 4)
			{
				tr.triangleTable[i] = (float) i / (FUNCTABLE_SIZE / 4);
			}
			else
			{
				tr.triangleTable[i] = 1.0f - tr.triangleTable[i - FUNCTABLE_SIZE / 4];
			}
		}
		else
		{
			tr.triangleTable[i] = -tr.triangleTable[i - FUNCTABLE_SIZE / 2];
		}
	}
}

//==========================================================================
//
//	R_Init
//
//==========================================================================

static void R_Init()
{
	Log::write("----- R_Init -----\n");

	// clear all our internal state
	Com_Memset(&tr, 0, sizeof(tr));
	Com_Memset(&backEnd, 0, sizeof(backEnd));
	Com_Memset(&tess, 0, sizeof(tess));

	if ((qintptr)tess.xyz & 15)
	{
		Log::write("WARNING: tess.xyz not 16 byte aligned\n");
	}
	Com_Memset(tess.constantColor255, 255, sizeof(tess.constantColor255));

	R_InitFunctionTables();

	R_InitFogTable();

	R_NoiseInit();

	R_Register();

	R_InitBackEndData();

	InitOpenGL();

	R_InitImages();

	R_InitShaders();

	R_InitSkins();

	R_ModelInit();

	R_InitFreeType();

	int err = qglGetError();
	if (err != GL_NO_ERROR)
	{
		Log::write("glGetError() = 0x%x\n", err);
	}

	Log::write("----- finished R_Init -----\n");
}

//==========================================================================
//
//	R_BeginRegistration
//
//==========================================================================

void R_BeginRegistration(glconfig_t *glconfigOut)
{
	R_Init();

	*glconfigOut = glConfig;

	R_SyncRenderThread();

	r_oldviewleaf = NULL;
	r_viewleaf = NULL;
	r_oldviewcluster = -1;
	r_viewcluster = -1;
	tr.viewCluster = -1;		// force markleafs to regenerate
	R_ClearFlares();
	R_ClearScene();

	tr.registered = true;

	if (GGameType & GAME_Quake3)
	{
		// NOTE: this sucks, for some reason the first stretch pic is never drawn
		// without this we'd see a white flash on a level load because the very
		// first time the level shot would not be drawn
		R_StretchPic(0, 0, 0, 0, 0, 0, 1, 1, 0);
	}
}

//==========================================================================
//
//	R_EndRegistration
//
//	Touch all images to make sure they are resident
//
//==========================================================================

void R_EndRegistration()
{
	if (GGameType & GAME_QuakeHexen)
	{
		GL_BuildLightmaps();
	}
	R_SyncRenderThread();
	RB_ShowImages();
}

//==========================================================================
//
//	R_Shutdown
//
//==========================================================================

void R_Shutdown(bool destroyWindow)
{
	Log::write("RE_Shutdown( %i )\n", destroyWindow);

	Cmd_RemoveCommand("modellist");
	Cmd_RemoveCommand("imagelist");
	Cmd_RemoveCommand("shaderlist");
	Cmd_RemoveCommand("skinlist");
	Cmd_RemoveCommand("screenshot");
	Cmd_RemoveCommand("screenshotJPEG");
	Cmd_RemoveCommand("gfxinfo");
	Cmd_RemoveCommand("modelist");

	if (tr.registered)
	{
		R_SyncRenderThread();
		R_ShutdownCommandBuffers();

		R_FreeModels();

		R_FreeShaders();

		R_DeleteTextures();

		R_FreeBackEndData();
	}

	R_DoneFreeType();

	// shut down platform specific OpenGL stuff
	if (destroyWindow)
	{
		GLimp_Shutdown();

		// shutdown QGL subsystem
		QGL_Shutdown();
	}

	tr.registered = false;
}