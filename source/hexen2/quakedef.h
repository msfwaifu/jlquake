// quakedef.h -- primary header for client

/*
 * $Header: /H2 Mission Pack/Quakedef.h 8     3/19/98 12:53p Jmonroe $
 */

#include "../client/client.h"
#include "../client/game/hexen2/local.h"

//#define MISSIONPACK

#define HEXEN2_VERSION      1.12

//define	PARANOID			// speed sapping error checking

#define GAMENAME    "data1"		// directory to look in by default

#include <setjmp.h>

#if id386
#define UNALIGNED_OK    1	// set to 0 if unaligned accesses are not supported
#else
#define UNALIGNED_OK    0
#endif

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define CACHE_SIZE  32		// used to align key data structures

#define UNUSED(x)   (x = x)	// for pesky compiler / lint warnings

#define MINIMUM_MEMORY          0x550000
#define MINIMUM_MEMORY_LEVELPAK (MINIMUM_MEMORY + 0x100000)


#define ON_EPSILON      0.1			// point on plane side epsilon

#define SAVEGAME_COMMENT_LENGTH 39

//
// stats are integers communicated to the client by the server
//
//#define	STAT_HEALTH			0
#define STAT_FRAGS          1
#define STAT_WEAPON         2
//#define	STAT_AMMO			3
#define STAT_ARMOR          4
#define STAT_WEAPONFRAME    5
//#define	STAT_SHELLS			6
//#define	STAT_NAILS			7
//#define	STAT_ROCKETS		8
//#define	STAT_CELLS			9
//#define	STAT_ACTIVEWEAPON	10
#define STAT_TOTALSECRETS   11
#define STAT_TOTALMONSTERS  12
#define STAT_SECRETS        13		// bumped on client side by h2svc_foundsecret
#define STAT_MONSTERS       14		// bumped by h2svc_killedmonster
//#define	STAT_BLUEMANA			15
//#define	STAT_GREENMANA			16
//#define	STAT_EXPERIENCE		17


// stock defines

#define IT_SHOTGUN              1
#define IT_SUPER_SHOTGUN        2
#define IT_NAILGUN              4
#define IT_SUPER_NAILGUN        8
#define IT_GRENADE_LAUNCHER     16
#define IT_ROCKET_LAUNCHER      32
#define IT_LIGHTNING            64
#define IT_SUPER_LIGHTNING      128
#define IT_SHELLS               256
#define IT_NAILS                512
#define IT_ROCKETS              1024
#define IT_CELLS                2048
#define IT_AXE                  4096
#define IT_ARMOR1               8192
#define IT_ARMOR2               16384
#define IT_ARMOR3               32768
#define IT_SUPERHEALTH          65536
#define IT_KEY1                 131072
#define IT_KEY2                 262144
#define IT_INVISIBILITY         524288
#define IT_INVULNERABILITY      1048576
#define IT_SUIT                 2097152
#define IT_QUAD                 4194304
#define IT_SIGIL1               (1 << 28)
#define IT_SIGIL2               (1 << 29)
#define IT_SIGIL3               (1 << 30)
#define IT_SIGIL4               (1 << 31)

//===========================================

#ifdef MISSIONPACK
#define NUM_CLASSES                 NUM_CLASSES_H2MP
#else
#define NUM_CLASSES                 NUM_CLASSES_H2
#endif
#define ABILITIES_STR_INDEX         400

#define SOUND_CHANNELS      8

// This makes anyone on id's net privileged
// Use for multiplayer testing only - VERY dangerous!!!
// #define IDGODS

#include "common.h"
#include "vid.h"
#include "sys.h"
#include "zone.h"

//#define BASE_ENT_ON		1
//#define BASE_ENT_SENT	2

#define CLEAR_LIMIT 2

#define ENT_STATE_ON        1
#define ENT_CLEARED         2


#include "draw.h"
#include "screen.h"
#include "net.h"
#include "protocol.h"
#include "cmd.h"
#include "sbar.h"
#include "render.h"
#include "progs.h"
#include "client.h"
#include "server.h"
#include "world.h"
#include "keys.h"
#include "console.h"
#include "view.h"
#include "menu.h"

//=============================================================================

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use

typedef struct
{
	const char* basedir;
	int argc;
	char** argv;
	void* membase;
	int memsize;
} quakeparms_t;


//=============================================================================



//
// host
//
extern quakeparms_t host_parms;

extern Cvar* sys_ticrate;

extern qboolean host_initialized;			// true if into command execution
extern double host_frametime;
extern int host_framecount;				// incremented every frame, never reset
extern double realtime;					// not bounded in any way, changed at
										// start of every frame, never reset

void Host_ClearMemory(void);
void Host_ServerFrame(void);
void Host_InitCommands(void);
void Host_Init(quakeparms_t* parms);
void Host_Shutdown(void);
void Host_Error(const char* error, ...);
void Host_EndGame(const char* message, ...);
void Host_Frame(float time);
void Host_Quit_f(void);
void Host_ClientCommands(const char* fmt, ...);
void Host_ShutdownServer(qboolean crash);

extern qboolean msg_suppress_1;			// suppresses resolution and cache size console output
										//  an fullscreen DIB focus gain/loss
extern int current_skill;				// skill level for currently loaded level (in case
										//  the user changes the cvar while the level is
										//  running, this reflects the level actually in use)

extern qboolean isDedicated;

extern int minimum_memory;

extern int sv_kingofhill;
extern qboolean skip_start;
extern int num_intro_msg;
extern qboolean check_bottom;
