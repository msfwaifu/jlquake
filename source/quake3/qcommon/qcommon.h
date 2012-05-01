/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// qcommon.h -- definitions common between client and server, but not game.or ref modules
#ifndef _QCOMMON_H_
#define _QCOMMON_H_

//#define	PRE_RELEASE_DEMO

/*
==============================================================

NET

==============================================================
*/

#define MAX_PACKET_USERCMDS     32		// max number of q3usercmd_t in a packet

void        NET_Init(void);
void        NET_Shutdown(void);
void        NET_Restart(void);

void        NET_SendPacket(netsrc_t sock, int length, const void* data, netadr_t to);
void        NET_OutOfBandPrint(netsrc_t net_socket, netadr_t adr, const char* format, ...);
void        NET_OutOfBandData(netsrc_t sock, netadr_t adr, byte* format, int len);

qboolean    NET_GetLoopPacket(netsrc_t sock, netadr_t* net_from, QMsg* net_message);
void        NET_Sleep(int msec);

qboolean    Sys_GetPacket(netadr_t* net_from, QMsg* net_message);


#define MAX_DOWNLOAD_WINDOW         8		// max of eight download frames
#define MAX_DOWNLOAD_BLKSIZE        2048	// 2048 byte block chunks


/*
Netchan handles packet fragmentation and out of order / duplicate suppression
*/

void Netchan_Init(int qport);
void Netchan_Setup(netsrc_t sock, netchan_t* chan, netadr_t adr, int qport);

void Netchan_Transmit(netchan_t* chan, int length, const byte* data);
void Netchan_TransmitNextFragment(netchan_t* chan);

qboolean Netchan_Process(netchan_t* chan, QMsg* msg);


/*
==============================================================

PROTOCOL

==============================================================
*/

#define PROTOCOL_VERSION    68
// 1.31 - 67

// maintain a list of compatible protocols for demo playing
// NOTE: that stuff only works with two digits protocols
extern int demo_protocols[];

#define UPDATE_SERVER_NAME  "update.quake3arena.com"
// override on command line, config files etc.
#ifndef MASTER_SERVER_NAME
#define MASTER_SERVER_NAME  "master.quake3arena.com"
#endif
#ifndef AUTHORIZE_SERVER_NAME
#define AUTHORIZE_SERVER_NAME   "authorize.quake3arena.com"
#endif

#define PORT_MASTER         27950
#define PORT_UPDATE         27951
#ifndef PORT_AUTHORIZE
#define PORT_AUTHORIZE      27952
#endif
#define PORT_SERVER         27960
#define NUM_SERVER_PORTS    4		// broadcast scan this many ports after
									// PORT_SERVER so a single machine can
									// run multiple servers

/*
==============================================================

VIRTUAL MACHINE

==============================================================
*/

struct vm_t;

typedef enum {
	VMI_NATIVE,
	VMI_BYTECODE,
	VMI_COMPILED
} vmInterpret_t;

typedef enum {
	TRAP_MEMSET = 100,
	TRAP_MEMCPY,
	TRAP_STRNCPY,
	TRAP_SIN,
	TRAP_COS,
	TRAP_ATAN2,
	TRAP_SQRT,
	TRAP_MATRIXMULTIPLY,
	TRAP_ANGLEVECTORS,
	TRAP_PERPENDICULARVECTOR,
	TRAP_FLOOR,
	TRAP_CEIL,

	TRAP_TESTPRINTINT,
	TRAP_TESTPRINTFLOAT
} sharedTraps_t;

void    VM_Init(void);
vm_t* VM_Create(const char* module, qintptr (* systemCalls)(qintptr*),
	vmInterpret_t interpret);
// module should be bare: "cgame", not "cgame.dll" or "vm/cgame.qvm"

void    VM_Free(vm_t* vm);
void    VM_Clear(void);
vm_t* VM_Restart(vm_t* vm);

qintptr VM_Call(vm_t* vm, int callNum, ...);

//===========================================================================

void    Cmd_Init(void);

/*
==============================================================

FILESYSTEM

No stdio calls should be used by any part of the game, because
we need to deal with all sorts of directory and seperator char
issues.
==============================================================
*/

#define BASEGAME "baseq3"

void FS_InitFilesystem();

qboolean    FS_ConditionalRestart(int checksumFeed);
void    FS_Restart(int checksumFeed);
// shutdown and restart the filesystem so changes to fs_gamedir can take effect

int     FS_LoadStack();

/*
==============================================================

MISC

==============================================================
*/

// centralizing the declarations for cl_cdkey
// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=470
extern char cl_cdkey[34];

// returnbed by Sys_GetProcessorId
#define CPUID_GENERIC           0			// any unrecognized processor

#define CPUID_AXP               0x10

#define CPUID_INTEL_UNSUPPORTED 0x20			// Intel 386/486
#define CPUID_INTEL_PENTIUM     0x21			// Intel Pentium or PPro
#define CPUID_INTEL_MMX         0x22			// Intel Pentium/MMX or P2/MMX
#define CPUID_INTEL_KATMAI      0x23			// Intel Katmai

#define CPUID_AMD_3DNOW         0x30			// AMD K6 3DNOW!

char* CopyString(const char* in);

void        Com_BeginRedirect(char* buffer, int buffersize, void (* flush)(char*));
void        Com_EndRedirect(void);
void        Com_Printf(const char* fmt, ...);
void        Com_DPrintf(const char* fmt, ...);
void        Com_Error(int code, const char* fmt, ...);
void        Com_Quit_f(void);
int         Com_EventLoop(void);
int         Com_Milliseconds(void);		// will be journaled properly
int         Com_RealTime(qtime_t* qtime);
qboolean    Com_SafeMode(void);

void        Com_StartupVariable(const char* match);
// checks for and removes command line "+set var arg" constructs
// if match is NULL, all set commands will be executed, otherwise
// only a set with the exact name.  Only used during startup.


extern Cvar* com_speeds;
extern Cvar* com_sv_running;
extern Cvar* com_cl_running;
extern Cvar* com_version;
extern Cvar* com_blood;
extern Cvar* com_buildScript;			// for building release pak files
extern Cvar* com_cameraMode;

// both client and server must agree to pause
extern Cvar* cl_paused;
extern Cvar* sv_paused;

// com_speeds times
extern int time_game;
extern int time_frontend;
extern int time_backend;			// renderer backend time

extern qboolean com_errorEntered;

typedef enum {
	TAG_FREE,
	TAG_GENERAL,
	TAG_BOTLIB,
	TAG_RENDERER,
	TAG_SMALL,
	TAG_STATIC
} memtag_t;

/*

--- low memory ----
server vm
server clipmap
---mark---
renderer initialization (shaders, etc)
UI vm
cgame vm
renderer map
renderer models

---free---

temp file loading
--- high memory ---

*/

#if defined(_DEBUG)
	#define ZONE_DEBUG
#endif

#ifdef ZONE_DEBUG
#define Z_TagMalloc(size, tag)          Z_TagMallocDebug(size, tag, # size, __FILE__, __LINE__)
#define Z_Malloc(size)                  Z_MallocDebug(size, # size, __FILE__, __LINE__)
#define S_Malloc(size)                  S_MallocDebug(size, # size, __FILE__, __LINE__)
void* Z_TagMallocDebug(int size, int tag, char* label, char* file, int line);	// NOT 0 filled memory
void* Z_MallocDebug(int size, char* label, char* file, int line);			// returns 0 filled memory
void* S_MallocDebug(int size, char* label, char* file, int line);			// returns 0 filled memory
#else
void* Z_TagMalloc(int size, int tag);	// NOT 0 filled memory
void* Z_Malloc(int size);			// returns 0 filled memory
void* S_Malloc(int size);			// NOT 0 filled memory only for small allocations
#endif
void Z_Free(void* ptr);
void Z_FreeTags(int tag);
int Z_AvailableMemory(void);
void Z_LogHeap(void);

void Hunk_Clear(void);
void Hunk_ClearToMark(void);
void Hunk_SetMark(void);
qboolean Hunk_CheckMark(void);
void Hunk_ClearTempMemory(void);
void* Hunk_AllocateTempMemory(int size);
void Hunk_FreeTempMemory(void* buf);
int Hunk_MemoryRemaining(void);
void Hunk_Log(void);
void Hunk_Trash(void);

void Com_TouchMemory(void);

// commandLine should not include the executable name (argv[0])
void Com_Init(char* commandLine);
void Com_Frame(void);
void Com_Shutdown(void);


/*
==============================================================

CLIENT / SERVER SYSTEMS

==============================================================
*/

//
// client interface
//
void CL_InitKeyCommands(void);
// the keyboard binding interface must be setup before execing
// config files, but the rest of client startup will happen later

void CL_Init(void);
void CL_Disconnect(qboolean showMainMenu);
void CL_Shutdown(void);
void CL_Frame(int msec);
qboolean CL_GameCommand(void);
void CL_KeyEvent(int key, qboolean down, unsigned time);

void CL_CharEvent(int key);
// char events are for field typing, not game control

void CL_MouseEvent(int dx, int dy, int time);

void CL_PacketEvent(netadr_t from, QMsg* msg);

void CL_ConsolePrint(char* text);

void CL_MapLoading(void);
// do a screen update before starting to load a map
// when the server is going to load a new map, the entire hunk
// will be cleared, so the client must shutdown cgame, ui, and
// the renderer

void    CL_ForwardCommandToServer(const char* string);
// adds the current command line as a q3clc_clientCommand to the client message.
// things like godmode, noclip, etc, are commands directed to the server,
// so when they are typed in at the console, they will need to be forwarded.

void CL_CDDialog(void);
// bring up the "need a cd to play" dialog

void CL_ShutdownAll(void);
// shutdown all the client stuff

void CL_FlushMemory(void);
// dump all memory on an error

void CL_StartHunkUsers(void);
// start all the client stuff using the hunk

void Key_WriteBindings(fileHandle_t f);
// for writing the config files

void SCR_DebugGraph(float value, int color);	// FIXME: move logging to common?


//
// server interface
//
void SV_Init(void);
void SV_Shutdown(const char* finalmsg);
void SV_Frame(int msec);
void SV_PacketEvent(netadr_t from, QMsg* msg);
qboolean SV_GameCommand(void);


//
// UI interface
//
qboolean UI_GameCommand(void);
qboolean UI_usesUniqueCDKey();

/*
==============================================================

NON-PORTABLE SYSTEM SERVICES

==============================================================
*/

sysEvent_t  Sys_GetEvent(void);

void    Sys_Init(void);

void    Sys_UnloadGame(void);
void* Sys_GetGameAPI(void* parms);

void    Sys_UnloadCGame(void);
void* Sys_GetCGameAPI(void);

void    Sys_UnloadUI(void);
void* Sys_GetUIAPI(void);

//bot libraries
void    Sys_UnloadBotLib(void);
void* Sys_GetBotLibAPI(void* parms);

const char* Sys_GetCurrentUser(void);

void    Sys_Error(const char* error, ...);
void    Sys_Quit(void);

extern "C" void Sys_SnapVector(float* v);

// the system console is shown when a dedicated server is running
void    Sys_DisplaySystemConsole(qboolean show);

int     Sys_GetProcessorId(void);

void    Sys_BeginProfiling(void);
void    Sys_EndProfiling(void);

unsigned int Sys_ProcessorCount();

extern huffman_t clientHuffTables;

#define SV_ENCODE_START     4
#define SV_DECODE_START     12
#define CL_ENCODE_START     12
#define CL_DECODE_START     4

#endif	// _QCOMMON_H_
