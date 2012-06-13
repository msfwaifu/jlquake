/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __Q_SHARED_H
#define __Q_SHARED_H

// q_shared.h -- included first by ALL program modules.
// A user mod should never modify this file

#include "../../common/qcommon.h"

//#define PRE_RELEASE_DEMO

#ifndef PRE_RELEASE_DEMO
#define Q3_VERSION      "ET 2.60d"
#else
#define Q3_VERSION      "ET 2.32"
#endif	// PRE_RELEASE_DEMO
// 2.60d: Mac OSX universal binaries
// 2.60c: Mac OSX universal binaries
// 2.60b: CVE-2006-2082 fix
// 2.6x: Enemy Territory - ETPro team maintenance release
// 2.5x: Enemy Territory FINAL
// 2.4x: Enemy Territory RC's
// 2.3x: Enemy Territory TEST
// 2.2+: post SP removal
// 2.1+: post Enemy Territory moved standalone
// 2.x: post Enemy Territory
// 1.x: pre Enemy Territory
////
// 1.3-MP : final for release
// 1.1b - TTimo SP linux release (+ MP updates)
// 1.1b5 - Mac update merge in

#define CONFIG_NAME     "etconfig.cfg"

//#define LOCALIZATION_SUPPORT

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
#include <sys/stat.h>	// rain
#include <float.h>
#include <stdint.h>

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

#if defined(MACOS_X)

#error WTF

#define MAC_STATIC

#define CPUSTRING   "MacOS_X"

// Vanilla PPC code, but since PPC has a reciprocal square root estimate instruction,
// runs *much* faster than calling sqrt(). We'll use two Newton-Raphson
// refinement steps to get bunch more precision in the 1/sqrt() value for very little cost.
// We'll then multiply 1/sqrt times the original value to get the sqrt.
// This is about 12.4 times faster than sqrt() and according to my testing (not exhaustive)
// it returns fairly accurate results (error below 1.0e-5 up to 100000.0 in 0.1 increments).

static inline float idSqrt(float x)
{
	const float half = 0.5;
	const float one = 1.0;
	float B, y0, y1;

	// This'll NaN if it hits frsqrte. Handle both +0.0 and -0.0
	if (Q_fabs(x) == 0.0)
	{
		return x;
	}
	B = x;

#ifdef __GNUC__
	asm ("frsqrte %0,%1" : "=f" (y0) : "f" (B));
#else
	y0 = __frsqrte(B);
#endif
	/* First refinement step */

	y1 = y0 + half * y0 * (one - B * y0 * y0);

	/* Second refinement step -- copy the output of the last step to the input of this step */

	y0 = y1;
	y1 = y0 + half * y0 * (one - B * y0 * y0);

	/* Get sqrt(x) from x * 1/sqrt(x) */
	return x * y1;
}
#define sqrt idSqrt


#endif

//======================= MAC DEFINES =================================

#ifdef __MACOS__

#define MAC_STATIC

#define CPUSTRING   "OSX-universal"

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


#ifndef NULL
#define NULL ((void*)0)
#endif

#define MAX_QINT            0x7fffffff
#define MIN_QINT            (-MAX_QINT - 1)

// TTimo gcc: was missing, added from Q3 source
#ifndef max
#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif

// RF, this is just here so different elements of the engine can be aware of this setting as it changes
#define MAX_SP_CLIENTS      64		// increasing this will increase memory usage significantly

#define MAX_SAY_TEXT        150

typedef enum {
	MESSAGE_EMPTY = 0,
	MESSAGE_WAITING,		// rate/packet limited
	MESSAGE_WAITING_OVERFLOW,	// packet too large with message
} messageStatus_t;

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
	ERR_VID_FATAL,				// exit the entire game with a popup window and doesn't delete profile.pid
	ERR_DROP,					// print to console and disconnect from game
	ERR_SERVERDISCONNECT,		// don't kill server
	ERR_DISCONNECT,				// client disconnected from the server
	ERR_NEED_CD,				// pop up the need-cd dialog
	ERR_AUTOUPDATE
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

#define GAME_INIT_FRAMES    6
#define FRAMETIME           100					// msec

#define MAKERGB(v, r, g, b) v[0] = r; v[1] = g; v[2] = b
#define MAKERGBA(v, r, g, b, a) v[0] = r; v[1] = g; v[2] = b; v[3] = a

int     Q_rand(int* seed);
float   Q_random(int* seed);

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

// this is only here so the functions in q_shared.c and bg_*.c can link
void QDECL Com_Error(int level, const char* error, ...) id_attribute((format(printf,2,3)));
void QDECL Com_Printf(const char* msg, ...) id_attribute((format(printf,1,2)));

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
#define ANIM_BITS       10

#define SNAPFLAG_RATE_DELAYED   1
#define SNAPFLAG_NOT_ACTIVE     2	// snapshot used during connection and for zombies
#define SNAPFLAG_SERVERCOUNT    4	// toggled every map_restart so transitions can be detected

//
// per-level limits
//
#define MAX_SOUNDS          256		// so they cannot be blindly increased
#define MAX_CS_SKINS        64
#define MAX_CSSTRINGS       32

#define MAX_CS_SHADERS      32

#define MAX_MULTI_SPAWNTARGETS  16	// JPW NERVE

#define MAX_DLIGHT_CONFIGSTRINGS    16
#define MAX_SPLINE_CONFIGSTRINGS    8

#define PARTICLE_SNOW128    1
#define PARTICLE_SNOW64     2
#define PARTICLE_SNOW32     3
#define PARTICLE_SNOW256    0

#define PARTICLE_BUBBLE8    4
#define PARTICLE_BUBBLE16   5
#define PARTICLE_BUBBLE32   6
#define PARTICLE_BUBBLE64   7

#define RESERVED_CONFIGSTRINGS  2	// game can't modify below this, only the system can

// xkan, 1/10/2003 - adapted from original SP
typedef enum
{
	AISTATE_RELAXED,
	AISTATE_QUERY,
	AISTATE_ALERT,
	AISTATE_COMBAT,

	MAX_AISTATES
} aistateEnum_t;

//====================================================================

//----(SA) wolf buttons
#define WBUTTON_LEANLEFT    16
#define WBUTTON_LEANRIGHT   32

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
#define AS_LOCAL        0
#define AS_GLOBAL       1			// NERVE - SMF - modified
#define AS_FAVORITES    2

typedef enum _flag_status {
	FLAG_ATBASE = 0,
	FLAG_TAKEN,			// CTF
	FLAG_TAKEN_RED,		// One Flag CTF
	FLAG_TAKEN_BLUE,	// One Flag CTF
	FLAG_DROPPED
} flagStatus_t;



#define MAX_PINGREQUESTS            16
#define MAX_SERVERSTATUSREQUESTS    16

#define CDKEY_LEN 16
#define CDCHKSUM_LEN 2

// NERVE - SMF - wolf server/game states
typedef enum {
	GS_INITIALIZE = -1,
	GS_PLAYING,
	GS_WARMUP_COUNTDOWN,
	GS_WARMUP,
	GS_INTERMISSION,
	GS_WAITING_FOR_PLAYERS,
	GS_RESET
} gamestate_t;

#endif	// __Q_SHARED_H
