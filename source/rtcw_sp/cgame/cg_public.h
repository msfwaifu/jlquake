/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

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

#define MAX_ENTITIES_IN_SNAPSHOT    256

// snapshots are a view of the server at a given time

// Snapshots are generated at regular time intervals by the server,
// but they may not be sent if a client's rate level is exceeded, or
// they may be dropped by the network.
typedef struct
{
	int snapFlags;						// SNAPFLAG_RATE_DELAYED, etc
	int ping;

	int serverTime;					// server time the message is valid for (in msec)

	byte areamask[MAX_MAP_AREA_BYTES];					// portalarea visibility bits

	wsplayerState_t ps;							// complete information about the current player at this time

	int numEntities;						// all of the entities that need to be presented
	wsentityState_t entities[MAX_ENTITIES_IN_SNAPSHOT];		// at the time of this snapshot

	int numServerCommands;					// text based server commands to execute when this
	int serverCommandSequence;				// snapshot becomes current
} snapshot_t;

enum {
	CGAME_EVENT_NONE,
	CGAME_EVENT_TEAMMENU,
	CGAME_EVENT_SCOREBOARD,
	CGAME_EVENT_EDITHUD
};

//	Overlaps with RF_WRAP_FRAMES
#define WSRF_BLINK            (1 << 9)		// eyes in 'blink' state

struct wsrefEntity_t
{
	refEntityType_t reType;
	int renderfx;

	qhandle_t hModel;				// opaque type outside refresh

	// most recent data
	vec3_t lightingOrigin;			// so multi-part models can be lit identically (RF_LIGHTING_ORIGIN)
	float shadowPlane;				// projection shadows go here, stencils go slightly lower

	vec3_t axis[3];					// rotation vectors
	vec3_t torsoAxis[3];			// rotation vectors for torso section of skeletal animation
	qboolean nonNormalizedAxes;		// axis are not normalized, i.e. they have scale
	float origin[3];				// also used as MODEL_BEAM's "from"
	int frame;						// also used as MODEL_BEAM's diameter
	int torsoFrame;					// skeletal torso can have frame independant of legs frame

	vec3_t scale;		//----(SA)	added

	// previous data for frame interpolation
	float oldorigin[3];				// also used as MODEL_BEAM's "to"
	int oldframe;
	int oldTorsoFrame;
	float backlerp;					// 0.0 = current, 1.0 = old
	float torsoBacklerp;

	// texturing
	int skinNum;					// inline skin index
	qhandle_t customSkin;			// NULL for default skin
	qhandle_t customShader;			// use one image for the entire thing

	// misc
	byte shaderRGBA[4];				// colors used by rgbgen entity shaders
	float shaderTexCoord[2];		// texture coordinates used by tcMod entity modifiers
	float shaderTime;				// subtracted from refdef time to control effect start times

	// extra sprite information
	float radius;
	float rotation;

	// Ridah
	vec3_t fireRiseDir;

	// Ridah, entity fading (gibs, debris, etc)
	int fadeStartTime, fadeEndTime;

	float hilightIntensity;			//----(SA)	added

	int reFlags;

	int entityNum;					// currentState.number, so we can attach rendering effects to specific entities (Zombie)
};

struct wsglfog_t
{
	int mode;					// GL_LINEAR, GL_EXP
	int hint;					// GL_DONT_CARE
	int startTime;				// in ms
	int finishTime;				// in ms
	float color[4];
	float start;				// near
	float end;					// far
	qboolean useEndForClip;		// use the 'far' value for the far clipping plane
	float density;				// 0.0-1.0
	qboolean registered;		// has this fog been set up?
	qboolean drawsky;			// draw skybox
	qboolean clearscreen;		// clear the GL color buffer

	int dirty;
};

//	Overlaps with RDF_UNDERWATER
#define WSRDF_DRAWSKYBOX    16

struct wsrefdef_t
{
	int x, y, width, height;
	float fov_x, fov_y;
	vec3_t vieworg;
	vec3_t viewaxis[3];				// transformation matrix

	int time;			// time in milliseconds for shader effects and other time dependent rendering issues
	int rdflags;					// RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte areamask[MAX_MAP_AREA_BYTES];

	// text messages for deform text shaders
	char text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];

	//	added (needed to pass fog infos into the portal sky scene)
	wsglfog_t glfog;
};

struct wsglconfig_t
{
	char renderer_string[MAX_STRING_CHARS];
	char vendor_string[MAX_STRING_CHARS];
	char version_string[MAX_STRING_CHARS];
	char extensions_string[4 * MAX_STRING_CHARS];						// this is actually too short for many current cards/drivers  // (SA) doubled from 2x to 4x MAX_STRING_CHARS

	int maxTextureSize;								// queried from GL
	int maxActiveTextures;							// multitexture ability

	int colorBits, depthBits, stencilBits;

	int driverType;
	int hardwareType;

	qboolean deviceSupportsGamma;
	int textureCompression;
	qboolean textureEnvAddAvailable;
	qboolean anisotropicAvailable;					//----(SA)	added
	float maxAnisotropy;							//----(SA)	added

	// vendor-specific support
	// NVidia
	qboolean NVFogAvailable;					//----(SA)	added
	int NVFogMode;									//----(SA)	added
	// ATI
	int ATIMaxTruformTess;							// for truform support
	int ATINormalMode;							// for truform support
	int ATIPointMode;							// for truform support


	int vidWidth, vidHeight;
	// aspect is the screen's physical width / height, which may be different
	// than scrWidth / scrHeight if the pixels are non-square
	// normal screens should be 4/3, but wide aspect monitors may be 16/9
	float windowAspect;

	int displayFrequency;

	// synonymous with "does rendering consume the entire screen?", therefore
	// a Voodoo or Voodoo2 will have this set to TRUE, as will a Win32 ICD that
	// used CDS.
	qboolean isFullscreen;
	qboolean stereoEnabled;
	qboolean smpActive;						// dual processor

	qboolean textureFilterAnisotropicAvailable;					//DAJ
};

/*
==================================================================

functions imported from the main executable

==================================================================
*/

#define CGAME_IMPORT_API_VERSION    3

typedef enum {
	CG_PRINT,
	CG_ERROR,
	CG_MILLISECONDS,
	CG_CVAR_REGISTER,
	CG_CVAR_UPDATE,
	CG_CVAR_SET,
	CG_CVAR_VARIABLESTRINGBUFFER,
	CG_ARGC,
	CG_ARGV,
	CG_ARGS,
	CG_FS_FOPENFILE,
	CG_FS_READ,
	CG_FS_WRITE,
	CG_FS_FCLOSEFILE,
	CG_SENDCONSOLECOMMAND,
	CG_ADDCOMMAND,
	CG_SENDCLIENTCOMMAND,
	CG_UPDATESCREEN,
	CG_CM_LOADMAP,
	CG_CM_NUMINLINEMODELS,
	CG_CM_INLINEMODEL,
	CG_CM_LOADMODEL,
	CG_CM_TEMPBOXMODEL,
	CG_CM_POINTCONTENTS,
	CG_CM_TRANSFORMEDPOINTCONTENTS,
	CG_CM_BOXTRACE,
	CG_CM_TRANSFORMEDBOXTRACE,
// MrE:
	CG_CM_CAPSULETRACE,
	CG_CM_TRANSFORMEDCAPSULETRACE,
	CG_CM_TEMPCAPSULEMODEL,
// done.
	CG_CM_MARKFRAGMENTS,
	CG_S_STARTSOUND,
	CG_S_STARTSOUNDEX,	//----(SA)	added
	CG_S_STARTLOCALSOUND,
	CG_S_CLEARLOOPINGSOUNDS,
	CG_S_ADDLOOPINGSOUND,
	CG_S_UPDATEENTITYPOSITION,
// Ridah, talking animations
	CG_S_GETVOICEAMPLITUDE,
// done.
	CG_S_RESPATIALIZE,
	CG_S_REGISTERSOUND,
	CG_S_STARTBACKGROUNDTRACK,
	CG_S_FADESTREAMINGSOUND,	//----(SA)	modified
	CG_S_FADEALLSOUNDS,			//----(SA)	added for fading out everything
	CG_S_STARTSTREAMINGSOUND,
	CG_R_LOADWORLDMAP,
	CG_R_REGISTERMODEL,
	CG_R_REGISTERSKIN,
	CG_R_REGISTERSHADER,

	CG_R_GETSKINMODEL,		// client allowed to view what the .skin loaded so they can set their model appropriately
	CG_R_GETMODELSHADER,	// client allowed the shader handle for given model/surface (for things like debris inheriting shader from explosive)

	CG_R_REGISTERFONT,
	CG_R_CLEARSCENE,
	CG_R_ADDREFENTITYTOSCENE,
	CG_GET_ENTITY_TOKEN,
	CG_R_ADDPOLYTOSCENE,
// Ridah
	CG_R_ADDPOLYSTOSCENE,
	CG_RB_ZOMBIEFXADDNEWHIT,
// done.
	CG_R_ADDLIGHTTOSCENE,

	CG_R_ADDCORONATOSCENE,
	CG_R_SETFOG,

	CG_R_RENDERSCENE,
	CG_R_SETCOLOR,
	CG_R_DRAWSTRETCHPIC,
	CG_R_DRAWSTRETCHPIC_GRADIENT,	//----(SA)	added
	CG_R_MODELBOUNDS,
	CG_R_LERPTAG,
	CG_GETGLCONFIG,
	CG_GETGAMESTATE,
	CG_GETCURRENTSNAPSHOTNUMBER,
	CG_GETSNAPSHOT,
	CG_GETSERVERCOMMAND,
	CG_GETCURRENTCMDNUMBER,
	CG_GETUSERCMD,
	CG_SETUSERCMDVALUE,
	CG_R_REGISTERSHADERNOMIP,
	CG_MEMORY_REMAINING,

	CG_KEY_ISDOWN,
	CG_KEY_GETCATCHER,
	CG_KEY_SETCATCHER,
	CG_KEY_GETKEY,

	CG_PC_ADD_GLOBAL_DEFINE,
	CG_PC_LOAD_SOURCE,
	CG_PC_FREE_SOURCE,
	CG_PC_READ_TOKEN,
	CG_PC_SOURCE_FILE_AND_LINE,
	CG_S_STOPBACKGROUNDTRACK,
	CG_REAL_TIME,
	CG_SNAPVECTOR,
	CG_REMOVECOMMAND,
//	CG_R_LIGHTFORPOINT,	// not currently used (sorry, trying to keep CG_MEMSET @ 100)

	CG_SENDMOVESPEEDSTOGAME,

	CG_CIN_PLAYCINEMATIC,
	CG_CIN_STOPCINEMATIC,
	CG_CIN_RUNCINEMATIC,
	CG_CIN_DRAWCINEMATIC,
	CG_CIN_SETEXTENTS,
	CG_R_REMAP_SHADER,
//	CG_S_ADDREALLOOPINGSOUND,	// not currently used (sorry, trying to keep CG_MEMSET @ 100)
	CG_S_STOPLOOPINGSOUND,
	CG_S_STOPSTREAMINGSOUND,	//----(SA)	added

	CG_LOADCAMERA,
	CG_STARTCAMERA,
	CG_STOPCAMERA,	//----(SA)	added
	CG_GETCAMERAINFO,

	CG_MEMSET = 110,
	CG_MEMCPY,
	CG_STRNCPY,
	CG_SIN,
	CG_COS,
	CG_ATAN2,
	CG_SQRT,
	CG_FLOOR,
	CG_CEIL,

	CG_TESTPRINTINT,
	CG_TESTPRINTFLOAT,
	CG_ACOS,

	CG_INGAME_POPUP,		//----(SA)	added
	CG_INGAME_CLOSEPOPUP,	// NERVE - SMF
	CG_LIMBOCHAT,			// NERVE - SMF

	CG_GETMODELINFO
} cgameImport_t;


/*
==================================================================

functions exported to the main executable

==================================================================
*/

typedef enum {
	CG_INIT,
//	void CG_Init( int serverMessageNum, int serverCommandSequence )
	// called when the level loads or when the renderer is restarted
	// all media should be registered at this time
	// cgame will display loading status by calling SCR_Update, which
	// will call CG_DrawInformation during the loading process
	// reliableCommandSequence will be 0 on fresh loads, but higher for
	// demos, tourney restarts, or vid_restarts

	CG_SHUTDOWN,
//	void (*CG_Shutdown)( void );
	// oportunity to flush and close any open files

	CG_CONSOLE_COMMAND,
//	qboolean (*CG_ConsoleCommand)( void );
	// a console command has been issued locally that is not recognized by the
	// main game system.
	// use Cmd_Argc() / Cmd_Argv() to read the command, return qfalse if the
	// command is not known to the game

	CG_DRAW_ACTIVE_FRAME,
//	void (*CG_DrawActiveFrame)( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback );
	// Generates and draws a game scene and status information at the given time.
	// If demoPlayback is set, local movement prediction will not be enabled

	CG_CROSSHAIR_PLAYER,
//	int (*CG_CrosshairPlayer)( void );

	CG_LAST_ATTACKER,
//	int (*CG_LastAttacker)( void );

	CG_KEY_EVENT,
//	void	(*CG_KeyEvent)( int key, qboolean down );

	CG_MOUSE_EVENT,
//	void	(*CG_MouseEvent)( int dx, int dy );
	CG_EVENT_HANDLING,
//	void (*CG_EventHandling)(int type);

	CG_GET_TAG,
//	qboolean CG_GetTag( int clientNum, char *tagname, orientation_t *or );

	MAX_CGAME_EXPORT

} cgameExport_t;

//----------------------------------------------
