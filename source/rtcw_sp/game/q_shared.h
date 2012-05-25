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

#ifndef __Q_SHARED_H
#define __Q_SHARED_H

// q_shared.h -- included first by ALL program modules.
// A user mod should never modify this file

#include "../../common/qcommon.h"

#define Q3_VERSION      "Wolf 1.41"
// ver 1.0.0	- release
// ver 1.0.1	- post-release work
// ver 1.1.0	- patch 1 (12/12/01)
// ver 1.1b		- TTimo SP linux release (+ MP update)
// ver 1.2.b5	- Mac code merge in
// ver 1.3		- patch 2 (02/13/02)

#define NEW_ANIMS
#define MAX_TEAMNAME    32

#if defined(ppc) || defined(__ppc) || defined(__ppc__) || defined(__POWERPC__)
#define idppc 1
#endif

/**********************************************************************
  VM Considerations

  The VM can not use the standard system headers because we aren't really
  using the compiler they were meant for.  We use bg_lib.h which contains
  prototypes for the functions we define for our own use in bg_lib.c.

  When writing mods, please add needed headers HERE, do not start including
  stuff like <stdio.h> in the various .c files that make up each of the VMs
  since you will be including system headers files can will have issues.

  Remember, if you use a C library function that is not defined in bg_lib.c,
  you will have to add your own version for support in the VM.

 **********************************************************************/

#ifdef Q3_VM

#include "bg_lib.h"

#else

#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>

#endif

#ifdef _WIN32

//#pragma intrinsic( memset, memcpy )

#endif


// for windows fastcall option

#define QDECL

//======================= WIN32 DEFINES =================================

#ifdef WIN32

#define MAC_STATIC

#undef QDECL
#define QDECL   __cdecl

// buildstring will be incorporated into the version string
#ifdef NDEBUG
#ifdef _M_IX86
#define CPUSTRING   "win-x86"
#elif defined _M_X64
#define CPUSTRING   "win-x86_64"
#elif defined _M_ALPHA
#define CPUSTRING   "win-AXP"
#endif
#else
#ifdef _M_IX86
#define CPUSTRING   "win-x86-debug"
#elif defined _M_X64
#define CPUSTRING   "win-x86_64-debug"
#elif defined _M_ALPHA
#define CPUSTRING   "win-AXP-debug"
#endif
#endif


#endif

//======================= MAC OS X SERVER DEFINES =====================

#if defined(__MACH__) && defined(__APPLE__)

#define MAC_STATIC

#ifdef __ppc__
#define CPUSTRING   "MacOSXS-ppc"
#elif defined __i386__
#define CPUSTRING   "MacOSXS-i386"
#else
#define CPUSTRING   "MacOSXS-other"
#endif

#define GAME_HARD_LINKED
#define CGAME_HARD_LINKED
#define UI_HARD_LINKED
#define BOTLIB_HARD_LINKED

#endif

//======================= MAC DEFINES =================================

#ifdef __MACOS__

#include <MacTypes.h>
//DAJ #define	MAC_STATIC	static
#define MAC_STATIC

#define CPUSTRING   "MacOS-PPC"

#define GAME_HARD_LINKED
#define CGAME_HARD_LINKED
#define UI_HARD_LINKED
#define BOTLIB_HARD_LINKED

void Sys_PumpEvents(void);

#endif

//======================= LINUX DEFINES =================================

// the mac compiler can't handle >32k of locals, so we
// just waste space and make big arrays static...
#ifdef __linux__

#define MAC_STATIC

#ifdef __i386__
#define CPUSTRING   "linux-i386"
#elif defined __axp__
#define CPUSTRING   "linux-alpha"
#else
#define CPUSTRING   "linux-other"
#endif

#endif

//=============================================================


enum {qfalse, qtrue};

#ifndef ID_INLINE
#ifdef _WIN32
#define ID_INLINE __inline
#else
#define ID_INLINE inline
#endif
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

#define MAX_QINT            0x7fffffff
#define MIN_QINT            (-MAX_QINT - 1)


#ifndef max
#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif

// RF, this is just here so different elements of the engine can be aware of this setting as it changes
#define MAX_SP_CLIENTS      64		// increasing this will increase memory usage significantly

#define MAX_SAY_TEXT        150

// print levels from renderer (FIXME: set up for game / cgame?)
typedef enum {
	PRINT_ALL,
	PRINT_DEVELOPER,		// only print when "developer 1"
	PRINT_WARNING,
	PRINT_ERROR
} printParm_t;

#ifdef  ERR_FATAL
#undef  ERR_FATAL				// this is be defined in malloc.h
#endif

// parameters to the main Error routine
typedef enum {
	ERR_FATAL,					// exit the entire game with a popup window
	ERR_DROP,					// print to console and disconnect from game
	ERR_SERVERDISCONNECT,		// don't kill server
	ERR_DISCONNECT,				// client disconnected from the server
	ERR_NEED_CD,				// pop up the need-cd dialog
	ERR_ENDGAME					// not an error.  just clean up properly, exit to the menu, and start up the "endgame" menu  //----(SA)	added
} errorParm_t;

#if defined(_DEBUG)
	#define HUNK_DEBUG
#endif

typedef enum {
	h_high,
	h_low,
	h_dontcare
} ha_pref;

#ifdef HUNK_DEBUG
#define Hunk_Alloc(size, preference)              Hunk_AllocDebug(size, preference, # size, __FILE__, __LINE__)
void* Hunk_AllocDebug(int size, ha_pref preference, char* label, char* file, int line);
#else
void* Hunk_Alloc(int size, ha_pref preference);
#endif

/*
==============================================================

MATHLIB

==============================================================
*/


// all drawing is done to a 640*480 virtual screen size
// and will be automatically scaled to the real resolution
#define SCREEN_WIDTH        640
#define SCREEN_HEIGHT       480

extern vec4_t colorBlack;
extern vec4_t colorRed;
extern vec4_t colorGreen;
extern vec4_t colorBlue;
extern vec4_t colorYellow;
extern vec4_t colorMagenta;
extern vec4_t colorCyan;
extern vec4_t colorWhite;
extern vec4_t colorLtGrey;
extern vec4_t colorMdGrey;
extern vec4_t colorDkGrey;

#define MAKERGB(v, r, g, b) v[0] = r; v[1] = g; v[2] = b
#define MAKERGBA(v, r, g, b, a) v[0] = r; v[1] = g; v[2] = b; v[3] = a

unsigned ColorBytes3(float r, float g, float b);

float NormalizeColor(const vec3_t in, vec3_t out);

int     Q_rand(int* seed);
float   Q_random(int* seed);
float   Q_crandom(int* seed);

void vectoangles(const vec3_t value1, vec3_t angles);
// TTimo: const vec_t ** would require explicit casts for ANSI C conformance
// see unix/const-arg.c
void AxisToAngles(/*const*/ vec3_t axis[3], vec3_t angles);

// Ridah
void ProjectPointOntoVector(vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj);
// done.

//=============================================

float Com_Clamp(float min, float max, float value);

// TTimo
qboolean COM_BitCheck(const int array[], int bitNum);
void COM_BitSet(int array[], int bitNum);
void COM_BitClear(int array[], int bitNum);

//=============================================

#ifdef _WIN32
#define Q_putenv _putenv
#else
#define Q_putenv putenv
#endif

// removes color sequences from string
char* Q_CleanStr(char* string);
//=============================================

// 64-bit integers for global rankings interface
// implemented as a struct for qvm compatibility
typedef struct
{
	byte b0;
	byte b1;
	byte b2;
	byte b3;
	byte b4;
	byte b5;
	byte b6;
	byte b7;
} qint64;

//=============================================

float* tv(float x, float y, float z);

//=============================================

// this is only here so the functions in q_shared.c and bg_*.c can link
void QDECL Com_Error(int level, const char* error, ...);
void QDECL Com_Printf(const char* msg, ...);


/*
==============================================================

SAVE

    12 -
    13 - (SA) added 'episode' tracking to savegame
    14 - RF added 'skill'
    15 - (SA) moved time info above the main game reading
    16 - (SA) added fog
    17 - (SA) rats, changed fog.
  18 - TTimo targetdeath fix
       show_bug.cgi?id=434

==============================================================
*/

#define SAVE_VERSION    18
#define SAVE_INFOSTRING_LENGTH  256



/*
==========================================================

  RELOAD STATES

==========================================================
*/

#define RELOAD_SAVEGAME         0x01
#define RELOAD_NEXTMAP          0x02
#define RELOAD_NEXTMAP_WAITING  0x04
#define RELOAD_FAILED           0x08
#define RELOAD_ENDGAME          0x10

/*
========================================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

========================================================================
*/
#define SNAPFLAG_RATE_DELAYED   1
#define SNAPFLAG_NOT_ACTIVE     2	// snapshot used during connection and for zombies
#define SNAPFLAG_SERVERCOUNT    4	// toggled every map_restart so transitions can be detected

//
// per-level limits
//
#define MAX_LOCATIONS       64

#define MAX_SOUNDS          256		// so they cannot be blindly increased


#define MAX_PARTICLES_AREAS     128

#define MAX_MULTI_SPAWNTARGETS  16	// JPW NERVE

#define MAX_DLIGHT_CONFIGSTRINGS    128
#define MAX_CLIPBOARD_CONFIGSTRINGS 64
#define MAX_SPLINE_CONFIGSTRINGS    64

#define PARTICLE_SNOW128    1
#define PARTICLE_SNOW64     2
#define PARTICLE_SNOW32     3
#define PARTICLE_SNOW256    0

#define PARTICLE_BUBBLE8    4
#define PARTICLE_BUBBLE16   5
#define PARTICLE_BUBBLE32   6
#define PARTICLE_BUBBLE64   7

#define RESERVED_CONFIGSTRINGS  2	// game can't modify below this, only the system can

//=========================================================
// shared by AI and animation scripting
//
typedef enum
{
	AISTATE_RELAXED,
	AISTATE_QUERY,
	AISTATE_ALERT,
	AISTATE_COMBAT,

	MAX_AISTATES
} aistateEnum_t;

//=========================================================


// weapon grouping
#define MAX_WEAP_BANKS      12
#define MAX_WEAPS_IN_BANK   3
// JPW NERVE
#define MAX_WEAPS_IN_BANK_MP    8
#define MAX_WEAP_BANKS_MP   7
// jpw
#define MAX_WEAP_ALTS       WP_DYNAMITE


//====================================================================

//----(SA) wolf buttons
#define WBUTTON_LEANLEFT    16
#define WBUTTON_LEANRIGHT   32

//===================================================================

// RF, put this here so we have a central means of defining a Zombie (kind of a hack, but this is to minimize bandwidth usage)
#define SET_FLAMING_ZOMBIE(x,y) (x.frame = y)
#define IS_FLAMING_ZOMBIE(x)    (x.frame == 1)

// real time
//=============================================


typedef struct qtime_s
{
	int tm_sec;		/* seconds after the minute - [0,59] */
	int tm_min;		/* minutes after the hour - [0,59] */
	int tm_hour;	/* hours since midnight - [0,23] */
	int tm_mday;	/* day of the month - [1,31] */
	int tm_mon;		/* months since January - [0,11] */
	int tm_year;	/* years since 1900 */
	int tm_wday;	/* days since Sunday - [0,6] */
	int tm_yday;	/* days since January 1 - [0,365] */
	int tm_isdst;	/* daylight savings time flag */
} qtime_t;


// server browser sources
#define AS_LOCAL            0
#define AS_MPLAYER      1
#define AS_GLOBAL           2
#define AS_FAVORITES    3

typedef enum _flag_status {
	FLAG_ATBASE = 0,
	FLAG_TAKEN,			// CTF
	FLAG_TAKEN_RED,		// One Flag CTF
	FLAG_TAKEN_BLUE,	// One Flag CTF
	FLAG_DROPPED
} flagStatus_t;



#define MAX_PINGREQUESTS            16
#define MAX_SERVERSTATUSREQUESTS    16

#define SAY_ALL     0
#define SAY_TEAM    1
#define SAY_TELL    2

#define CDKEY_LEN 16
#define CDCHKSUM_LEN 2

#endif	// __Q_SHARED_H
