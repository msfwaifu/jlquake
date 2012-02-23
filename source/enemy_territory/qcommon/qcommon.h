/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

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

// qcommon.h -- definitions common between client and server, but not game.or ref modules
#ifndef _QCOMMON_H_
#define _QCOMMON_H_

#include "../qcommon/cm_public.h"

//#define PRE_RELEASE_DEMO
#ifdef PRE_RELEASE_DEMO
#define PRE_RELEASE_DEMO_NODEVMAP
#endif // PRE_RELEASE_DEMO

//============================================================================

//
// msg.c
//
typedef struct {
	qboolean allowoverflow;     // if false, do a Com_Error
	qboolean overflowed;        // set to true if the buffer size failed (with allowoverflow set)
	qboolean oob;               // set to true if the buffer size failed (with allowoverflow set)
	byte    *data;
	int maxsize;
	int cursize;
	int uncompsize;             // NERVE - SMF - net debugging
	int readcount;
	int bit;                    // for bitwise reads and writes
} msg_t;

void MSG_Init( msg_t *buf, byte *data, int length );
void MSG_InitOOB( msg_t *buf, byte *data, int length );
void MSG_Clear( msg_t *buf );
void *MSG_GetSpace( msg_t *buf, int length );
void MSG_WriteData( msg_t *buf, const void *data, int length );
void MSG_Bitstream( msg_t *buf );
void MSG_Uncompressed( msg_t *buf );

// TTimo
// copy a msg_t in case we need to store it as is for a bit
// (as I needed this to keep an msg_t from a static var for later use)
// sets data buffer as MSG_Init does prior to do the copy
void MSG_Copy( msg_t *buf, byte *data, int length, msg_t *src );

struct usercmd_s;
struct entityState_s;
struct playerState_s;

void MSG_WriteBits( msg_t *msg, int value, int bits );

void MSG_WriteChar( msg_t *sb, int c );
void MSG_WriteByte( msg_t *sb, int c );
void MSG_WriteShort( msg_t *sb, int c );
void MSG_WriteLong( msg_t *sb, int c );
void MSG_WriteFloat( msg_t *sb, float f );
void MSG_WriteString( msg_t *sb, const char *s );
void MSG_WriteBigString( msg_t *sb, const char *s );
void MSG_WriteAngle16( msg_t *sb, float f );

void    MSG_BeginReading( msg_t *sb );
void    MSG_BeginReadingOOB( msg_t *sb );
void    MSG_BeginReadingUncompressed( msg_t *msg );

int     MSG_ReadBits( msg_t *msg, int bits );

int     MSG_ReadChar( msg_t *sb );
int     MSG_ReadByte( msg_t *sb );
int     MSG_ReadShort( msg_t *sb );
int     MSG_ReadLong( msg_t *sb );
float   MSG_ReadFloat( msg_t *sb );
char    *MSG_ReadString( msg_t *sb );
char    *MSG_ReadBigString( msg_t *sb );
char    *MSG_ReadStringLine( msg_t *sb );
float   MSG_ReadAngle16( msg_t *sb );
void    MSG_ReadData( msg_t *sb, void *buffer, int size );


void MSG_WriteDeltaUsercmd( msg_t *msg, struct usercmd_s *from, struct usercmd_s *to );
void MSG_ReadDeltaUsercmd( msg_t *msg, struct usercmd_s *from, struct usercmd_s *to );

void MSG_WriteDeltaUsercmdKey( msg_t *msg, int key, usercmd_t *from, usercmd_t *to );
void MSG_ReadDeltaUsercmdKey( msg_t *msg, int key, usercmd_t *from, usercmd_t *to );

void MSG_WriteDeltaEntity( msg_t *msg, struct entityState_s *from, struct entityState_s *to
						   , qboolean force );
void MSG_ReadDeltaEntity( msg_t *msg, entityState_t *from, entityState_t *to,
						  int number );

void MSG_WriteDeltaPlayerstate( msg_t *msg, struct playerState_s *from, struct playerState_s *to );
void MSG_ReadDeltaPlayerstate( msg_t *msg, struct playerState_s *from, struct playerState_s *to );


void MSG_ReportChangeVectors_f( void );

/*
==============================================================

NET

==============================================================
*/

#define PACKET_BACKUP   32  // number of old messages that must be kept on client and
							// server for delta comrpession and ping estimation
#define PACKET_MASK     ( PACKET_BACKUP - 1 )

#define MAX_PACKET_USERCMDS     32      // max number of usercmd_t in a packet

#define PORT_ANY            -1

// RF, increased this, seems to keep causing problems when set to 64, especially when loading
// a savegame, which is hard to fix on that side, since we can't really spread out a loadgame
// among several frames
//#define	MAX_RELIABLE_COMMANDS	64			// max string commands buffered for restransmit
//#define	MAX_RELIABLE_COMMANDS	128			// max string commands buffered for restransmit
#define MAX_RELIABLE_COMMANDS   256 // bigger!

typedef enum {
	NA_BOT,
	NA_BAD,                 // an address lookup failed
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
	NA_IPX,
	NA_BROADCAST_IPX
} netadrtype_t;

typedef enum {
	NS_CLIENT,
	NS_SERVER
} netsrc_t;

typedef struct {
	netadrtype_t type;

	byte ip[4];
	byte ipx[10];

	unsigned short port;
} netadr_t;

void        NET_Init( void );
void        NET_Shutdown( void );
void        NET_Restart( void );
void        NET_Config( qboolean enableNetworking );

void        NET_SendPacket( netsrc_t sock, int length, const void *data, netadr_t to );
void QDECL NET_OutOfBandPrint( netsrc_t net_socket, netadr_t adr, const char *format, ... );
void QDECL NET_OutOfBandData( netsrc_t sock, netadr_t adr, byte *format, int len );

qboolean    NET_CompareAdr( netadr_t a, netadr_t b );
qboolean    NET_CompareBaseAdr( netadr_t a, netadr_t b );
qboolean    NET_IsLocalAddress( netadr_t adr );
qboolean    NET_IsIPXAddress( const char *buf );
const char  *NET_AdrToString( netadr_t a );
qboolean    NET_StringToAdr( const char *s, netadr_t *a );
qboolean    NET_GetLoopPacket( netsrc_t sock, netadr_t *net_from, msg_t *net_message );
void        NET_Sleep( int msec );


//----(SA)	increased for larger submodel entity counts
#define MAX_MSGLEN              32768       // max length of a message, which may
//#define	MAX_MSGLEN				16384		// max length of a message, which may
// be fragmented into multiple packets
#define MAX_DOWNLOAD_WINDOW         8       // max of eight download frames
#define MAX_DOWNLOAD_BLKSIZE        2048    // 2048 byte block chunks


/*
Netchan handles packet fragmentation and out of order / duplicate suppression
*/

typedef struct {
	netsrc_t sock;

	int dropped;                    // between last packet and previous

	netadr_t remoteAddress;
	int qport;                      // qport value to write when transmitting

	// sequencing variables
	int incomingSequence;
	int outgoingSequence;

	// incoming fragment assembly buffer
	int fragmentSequence;
	int fragmentLength;
	byte fragmentBuffer[MAX_MSGLEN];

	// outgoing fragment buffer
	// we need to space out the sending of large fragmented messages
	qboolean unsentFragments;
	int unsentFragmentStart;
	int unsentLength;
	byte unsentBuffer[MAX_MSGLEN];

} netchan_t;

void Netchan_Init( int qport );
void Netchan_Setup( netsrc_t sock, netchan_t *chan, netadr_t adr, int qport );

void Netchan_Transmit( netchan_t *chan, int length, const byte *data );
void Netchan_TransmitNextFragment( netchan_t *chan );

qboolean Netchan_Process( netchan_t *chan, msg_t *msg );


/*
==============================================================

PROTOCOL

==============================================================
*/

// sent by the server, printed on connection screen, works for all clients
// (restrictions: does not handle \n, no more than 256 chars)
#define PROTOCOL_MISMATCH_ERROR "ERROR: Protocol Mismatch Between Client and Server.\
The server you are attempting to join is running an incompatible version of the game."

// long version used by the client in diagnostic window
#define PROTOCOL_MISMATCH_ERROR_LONG "ERROR: Protocol Mismatch Between Client and Server.\n\n\
The server you attempted to join is running an incompatible version of the game.\n\
You or the server may be running older versions of the game. Press the auto-update\
 button if it appears on the Main Menu screen."

#define GAMENAME_STRING "et"
#ifndef PRE_RELEASE_DEMO
// 2.56 - protocol 83
// 2.4 - protocol 80
// 1.33 - protocol 59
// 1.4 - protocol 60
#define PROTOCOL_VERSION    84
#else
// the demo uses a different protocol version for independant browsing
#define PROTOCOL_VERSION    72
#endif

// NERVE - SMF - wolf multiplayer master servers
#ifndef MASTER_SERVER_NAME
	#define MASTER_SERVER_NAME      "etmaster.idsoftware.com"
#endif
#define MOTD_SERVER_NAME        "etmaster.idsoftware.com"    //"etmotd.idsoftware.com"			// ?.?.?.?

#ifdef AUTHORIZE_SUPPORT
	#define AUTHORIZE_SERVER_NAME   "wolfauthorize.idsoftware.com"
#endif // AUTHORIZE_SUPPORT

// TTimo: override autoupdate server for testing
#ifndef AUTOUPDATE_SERVER_NAME
//	#define AUTOUPDATE_SERVER_NAME "127.0.0.1"
	#define AUTOUPDATE_SERVER_NAME "au2rtcw2.activision.com"
#endif

// TTimo: allow override for easy dev/testing..
// FIXME: not planning to support more than 1 auto update server
// see cons -- update_server=myhost
#define MAX_AUTOUPDATE_SERVERS  5
#if !defined( AUTOUPDATE_SERVER_NAME )
  #define AUTOUPDATE_SERVER1_NAME   "au2rtcw1.activision.com"            // DHM - Nerve
  #define AUTOUPDATE_SERVER2_NAME   "au2rtcw2.activision.com"            // DHM - Nerve
  #define AUTOUPDATE_SERVER3_NAME   "au2rtcw3.activision.com"            // DHM - Nerve
  #define AUTOUPDATE_SERVER4_NAME   "au2rtcw4.activision.com"            // DHM - Nerve
  #define AUTOUPDATE_SERVER5_NAME   "au2rtcw5.activision.com"            // DHM - Nerve
#else
  #define AUTOUPDATE_SERVER1_NAME   AUTOUPDATE_SERVER_NAME
  #define AUTOUPDATE_SERVER2_NAME   AUTOUPDATE_SERVER_NAME
  #define AUTOUPDATE_SERVER3_NAME   AUTOUPDATE_SERVER_NAME
  #define AUTOUPDATE_SERVER4_NAME   AUTOUPDATE_SERVER_NAME
  #define AUTOUPDATE_SERVER5_NAME   AUTOUPDATE_SERVER_NAME
#endif

#define PORT_MASTER         27950
#define PORT_MOTD           27951
#ifdef AUTHORIZE_SUPPORT
#define PORT_AUTHORIZE      27952
#endif // AUTHORIZE_SUPPORT
#define PORT_SERVER         27960
#define NUM_SERVER_PORTS    4       // broadcast scan this many ports after
									// PORT_SERVER so a single machine can
									// run multiple servers


// the svc_strings[] array in cl_parse.c should mirror this
//
// server to client
//
enum svc_ops_e {
	svc_bad,
	svc_nop,
	svc_gamestate,
	svc_configstring,           // [short] [string] only in gamestate messages
	svc_baseline,               // only in gamestate messages
	svc_serverCommand,          // [string] to be executed by client game module
	svc_download,               // [short] size [size bytes]
	svc_snapshot,
	svc_EOF
};


//
// client to server
//
enum clc_ops_e {
	clc_bad,
	clc_nop,
	clc_move,               // [[usercmd_t]
	clc_moveNoDelta,        // [[usercmd_t]
	clc_clientCommand,      // [string] message
	clc_EOF
};

/*
==============================================================

VIRTUAL MACHINE

==============================================================
*/

typedef struct vm_s vm_t;

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

void    VM_Init( void );
vm_t    *VM_Create( const char *module, intptr_t ( *systemCalls )( intptr_t * ),
					vmInterpret_t interpret );
// module should be bare: "cgame", not "cgame.dll" or "vm/cgame.qvm"

void    VM_Free( vm_t *vm );
void    VM_Clear( void );
vm_t    *VM_Restart( vm_t *vm );

intptr_t QDECL VM_Call( vm_t *vm, int callNum, ... );

void    VM_Debug( int level );

void    *VM_ArgPtr( intptr_t intValue );
void    *VM_ExplicitArgPtr( vm_t *vm, intptr_t intValue );

/*
==============================================================

CMD

Command text buffering and command execution

==============================================================
*/

/*

Any number of commands can be added in a frame, from several different sources.
Most commands come from either keybindings or console line input, but entire text
files can be execed.

*/

void Cbuf_ExecuteText( int exec_when, const char *text );
// this can be used in place of either Cbuf_AddText or Cbuf_InsertText

void Cbuf_Execute( void );
// Pulls off \n terminated lines of text from the command buffer and sends
// them through Cmd_ExecuteString.  Stops when the buffer is empty.
// Normally called once per frame, but may be explicitly invoked.
// Do not call inside a command function, or current args will be destroyed.

//===========================================================================

/*

Command execution takes a null terminated string, breaks it into tokens,
then searches for a command or variable that matches the first token.

*/

void    Cmd_Init( void );

void    Cmd_ExecuteString( const char *text );
// Parses a single line of text into arguments and tries to execute it
// as if it was typed at the console


/*
==============================================================

CVAR

==============================================================
*/

/*

Cvar variables are used to hold scalar or string variables that can be changed
or displayed at the console or prog code as well as accessed directly
in C code.

The user can access cvars from the console in three ways:
r_draworder			prints the current value
r_draworder 0		sets the current value to 0
set r_draworder 0	as above, but creates the cvar if not present

Cvars are restricted from having the same names as commands to keep this
interface from being ambiguous.

The are also occasionally used to communicated information between different
modules of the program.

*/

void    Cvar_WriteVariables( fileHandle_t f );
// writes lines containing "set variable value" for all variables
// with the archive flag set to true.

/*
==============================================================

FILESYSTEM

No stdio calls should be used by any part of the game, because
we need to deal with all sorts of directory and seperator char
issues.
==============================================================
*/

#ifndef PRE_RELEASE_DEMO
//#define BASEGAME "main"
#define BASEGAME "etmain"
#else
#define BASEGAME "ettest"
#endif

// number of id paks that will never be autodownloaded from baseq3
#define NUM_ID_PAKS     9

#define MAX_FILE_HANDLES    64

#ifdef WIN32
	#define Q_rmdir _rmdir
#else
	#define Q_rmdir rmdir
#endif

qboolean FS_Initialized();

void    FS_InitFilesystem( void );
void    FS_Shutdown( qboolean closemfp );

qboolean    FS_ConditionalRestart( int checksumFeed );
void    FS_Restart( int checksumFeed );
// shutdown and restart the filesystem so changes to fs_gamedir can take effect

char    **FS_ListFiles( const char *directory, const char *extension, int *numfiles );
// directory should not have either a leading or trailing /
// if extension is "/", only subdirectories will be returned
// the returned files will not include any directories or /

void    FS_FreeFileList( char **list );

qboolean FS_FileExists( const char *file );
qboolean FS_OS_FileExists( const char *file ); // TTimo - test file existence given OS path

int     FS_LoadStack();

int     FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize );
int     FS_GetModList(  char *listbuf, int bufsize );

fileHandle_t    FS_FOpenFileWrite( const char *qpath );
// will properly create any needed paths and deal with seperater character issues

int     FS_filelength( fileHandle_t f );
fileHandle_t FS_SV_FOpenFileWrite( const char *filename );
int     FS_SV_FOpenFileRead( const char *filename, fileHandle_t *fp );
void    FS_SV_Rename( const char *from, const char *to );
int     FS_FOpenFileRead( const char *qpath, fileHandle_t *file, qboolean uniqueFILE );
/*
if uniqueFILE is true, then a new FILE will be fopened even if the file
is found in an already open pak file.  If uniqueFILE is false, you must call
FS_FCloseFile instead of fclose, otherwise the pak FILE would be improperly closed
It is generally safe to always set uniqueFILE to true, because the majority of
file IO goes through FS_ReadFile, which Does The Right Thing already.
*/
/* TTimo
show_bug.cgi?id=506
added exclude flag to filter out regular dirs or pack files on demand
would rather have used FS_FOpenFileRead(..., int filter_flag = 0)
but that's a C++ construct ..
*/
#define FS_EXCLUDE_DIR 0x1
#define FS_EXCLUDE_PK3 0x2
int FS_FOpenFileRead_Filtered( const char *qpath, fileHandle_t *file, qboolean uniqueFILE, int filter_flag );

int     FS_FileIsInPAK( const char *filename, int *pChecksum );
// returns 1 if a file is in the PAK file, otherwise -1

int     FS_Delete( char *filename );    // only works inside the 'save' directory (for deleting savegames/images)

int     FS_Write( const void *buffer, int len, fileHandle_t f );

int     FS_Read( void *buffer, int len, fileHandle_t f );
// properly handles partial reads and reads from other dlls

void    FS_FCloseFile( fileHandle_t f );
// note: you can't just fclose from another DLL, due to MS libc issues

int     FS_ReadFile( const char *qpath, void **buffer );
// returns the length of the file
// a null buffer will just return the file length without loading
// as a quick check for existance. -1 length == not present
// A 0 byte will always be appended at the end, so string ops are safe.
// the buffer should be considered read-only, because it may be cached
// for other uses.

void    FS_ForceFlush( fileHandle_t f );
// forces flush on files we're writing to.

void    FS_FreeFile( void *buffer );
// frees the memory returned by FS_ReadFile

void    FS_WriteFile( const char *qpath, const void *buffer, int size );
// writes a complete file, creating any subdirectories needed

int     FS_filelength( fileHandle_t f );
// doesn't work for files that are opened from a pack file

int     FS_FTell( fileHandle_t f );
// where are we?

void    FS_Flush( fileHandle_t f );

void QDECL FS_Printf( fileHandle_t f, const char *fmt, ... );
// like fprintf

int     FS_FOpenFileByMode( const char *qpath, fileHandle_t *f, fsMode_t mode );
// opens a file for reading, writing, or appending depending on the value of mode

int     FS_Seek( fileHandle_t f, long offset, int origin );
// seek on a file (doesn't work for zip files!!!!!!!!)

qboolean FS_FilenameCompare( const char *s1, const char *s2 );

const char *FS_GamePureChecksum( void );
// Returns the checksum of the pk3 from which the server loaded the qagame.qvm

const char *FS_LoadedPakNames( void );
const char *FS_LoadedPakChecksums( void );
const char *FS_LoadedPakPureChecksums( void );
// Returns a space separated string containing the checksums of all loaded pk3 files.
// Servers with sv_pure set will get this string and pass it to clients.

const char *FS_ReferencedPakNames( void );
const char *FS_ReferencedPakChecksums( void );
const char *FS_ReferencedPakPureChecksums( void );
// Returns a space separated string containing the checksums of all loaded
// AND referenced pk3 files. Servers with sv_pure set will get this string
// back from clients for pure validation

void FS_ClearPakReferences( int flags );
// clears referenced booleans on loaded pk3s

void FS_PureServerSetReferencedPaks( const char *pakSums, const char *pakNames );
void FS_PureServerSetLoadedPaks( const char *pakSums, const char *pakNames );
// If the string is empty, all data sources will be allowed.
// If not empty, only pk3 files that match one of the space
// separated checksums will be checked for files, with the
// sole exception of .cfg files.

qboolean FS_idPak( char *pak, char *base );
qboolean FS_VerifyOfficialPaks( void );
qboolean FS_ComparePaks( char *neededpaks, int len, qboolean dlstring );

void FS_Rename( const char *from, const char *to );

char *FS_BuildOSPath( const char *base, const char *game, const char *qpath );

#if !defined( DEDICATED )
extern int cl_connectedToPureServer;
qboolean FS_CL_ExtractFromPakFile( const char *path, const char *gamedir, const char *filename, const char *cvar_lastVersion );
#endif

#if defined( DO_LIGHT_DEDICATED )
int FS_RandChecksumFeed();
#endif

char *FS_ShiftedStrStr( const char *string, const char *substring, int shift );
char *FS_ShiftStr( const char *string, int shift );

void FS_CopyFile( char *fromOSPath, char *toOSPath );

int FS_CreatePath( const char *OSPath );

qboolean FS_VerifyPak( const char *pak );

qboolean FS_IsPure( void );

unsigned int FS_ChecksumOSPath( char *OSPath );

/*
==============================================================

DOWNLOAD

==============================================================
*/

#include "dl_public.h"

/*
==============================================================

Edit fields and command line history/completion

==============================================================
*/

void Field_CompleteCommand( field_t *edit );

/*
==============================================================

MISC

==============================================================
*/

// centralizing the declarations for cl_cdkey
// (old code causing buffer overflows)
extern char cl_cdkey[34];
void Com_AppendCDKey( const char *filename );
void Com_ReadCDKey( const char *filename );

typedef struct gameInfo_s {
	qboolean spEnabled;
	int spGameTypes;
	int defaultSPGameType;
	int coopGameTypes;
	int defaultCoopGameType;
	int defaultGameType;
	qboolean usesProfiles;
} gameInfo_t;

extern gameInfo_t com_gameInfo;

// returnbed by Sys_GetProcessorId
#define CPUID_GENERIC           0           // any unrecognized processor

#define CPUID_AXP               0x10

#define CPUID_INTEL_UNSUPPORTED 0x20            // Intel 386/486
#define CPUID_INTEL_PENTIUM     0x21            // Intel Pentium or PPro
#define CPUID_INTEL_MMX         0x22            // Intel Pentium/MMX or P2/MMX
#define CPUID_INTEL_KATMAI      0x23            // Intel Katmai

#define CPUID_AMD_3DNOW         0x30            // AMD K6 3DNOW!

char        *CopyString( const char *in );

void Com_BeginRedirect( char *buffer, int buffersize, void ( *flush )( char * ) );
void        Com_EndRedirect( void );
int QDECL Com_VPrintf( const char *fmt, va_list argptr ) id_attribute( ( format( printf,1,0 ) ) ); // conforms to vprintf prototype for print callback passing
void QDECL Com_Printf( const char *fmt, ... ) id_attribute( ( format( printf,1,2 ) ) ); // this one calls to Com_VPrintf now
void QDECL Com_DPrintf( const char *fmt, ... ) id_attribute( ( format( printf,1,2 ) ) );
void QDECL Com_Error( int code, const char *fmt, ... ) id_attribute( ( format( printf,2,3 ) ) );
void        Com_Quit_f( void );
int         Com_EventLoop( void );
int         Com_Milliseconds( void );   // will be journaled properly
int         Com_HashKey( char *string, int maxlen );
int         Com_RealTime( qtime_t *qtime );
qboolean    Com_SafeMode( void );

void        Com_StartupVariable( const char *match );
void        Com_SetRecommended();
// checks for and removes command line "+set var arg" constructs
// if match is NULL, all set commands will be executed, otherwise
// only a set with the exact name.  Only used during startup.

//bani - profile functions
void Com_TrackProfile( char *profile_path );
qboolean Com_CheckProfile( char *profile_path );
qboolean Com_WriteProfile( char *profile_path );

extern Cvar  *com_ignorecrash;    //bani

extern Cvar  *com_pid;    //bani

extern Cvar  *com_speeds;
extern Cvar  *com_sv_running;
extern Cvar  *com_cl_running;
extern Cvar  *com_version;
//extern	Cvar	*com_blood;
extern Cvar  *com_buildScript;        // for building release pak files
extern Cvar  *com_cameraMode;
extern Cvar  *com_logosPlaying;

// watchdog
extern Cvar  *com_watchdog;
extern Cvar  *com_watchdog_cmd;

// both client and server must agree to pause
extern Cvar  *cl_paused;
extern Cvar  *sv_paused;

// com_speeds times
extern int time_game;
extern int time_frontend;
extern int time_backend;            // renderer backend time

extern int com_frameMsec;
extern int com_expectedhunkusage;
extern int com_hunkusedvalue;

extern qboolean com_errorEntered;

extern fileHandle_t com_journalFile;
extern fileHandle_t com_journalDataFile;

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

#if defined( _DEBUG ) && !defined( BSPC )
#define ZONE_DEBUG
#endif

#ifdef ZONE_DEBUG
#define Z_TagMalloc( size, tag )          Z_TagMallocDebug( size, tag, # size, __FILE__, __LINE__ )
#define Z_Malloc( size )                  Z_MallocDebug( size, # size, __FILE__, __LINE__ )
#define S_Malloc( size )                  S_MallocDebug( size, # size, __FILE__, __LINE__ )
void *Z_TagMallocDebug( int size, int tag, char *label, char *file, int line ); // NOT 0 filled memory
void *Z_MallocDebug( int size, char *label, char *file, int line );         // returns 0 filled memory
void *S_MallocDebug( int size, char *label, char *file, int line );         // returns 0 filled memory
#else
void *Z_TagMalloc( int size, int tag ); // NOT 0 filled memory
void *Z_Malloc( int size );         // returns 0 filled memory
void *S_Malloc( int size );         // NOT 0 filled memory only for small allocations
#endif
void Z_Free( void *ptr );
void Z_FreeTags( int tag );
void Z_LogHeap( void );

void Hunk_Clear( void );
void Hunk_ClearToMark( void );
void Hunk_SetMark( void );
qboolean Hunk_CheckMark( void );
//void *Hunk_Alloc( int size );
// void *Hunk_Alloc( int size, ha_pref preference );
void Hunk_ClearTempMemory( void );
void *Hunk_AllocateTempMemory( int size );
void Hunk_FreeTempMemory( void *buf );
int Hunk_MemoryRemaining( void );
void Hunk_SmallLog( void );
void Hunk_Log( void );

void Com_TouchMemory( void );

// commandLine should not include the executable name (argv[0])
void Com_Init( char *commandLine );
void Com_Frame( void );
void Com_Shutdown( qboolean badProfile );


/*
==============================================================

CLIENT / SERVER SYSTEMS

==============================================================
*/

//
// client interface
//
void CL_InitKeyCommands( void );
// the keyboard binding interface must be setup before execing
// config files, but the rest of client startup will happen later

void CL_Init( void );
void CL_ClearStaticDownload( void );
void CL_Disconnect( qboolean showMainMenu );
void CL_Shutdown( void );
void CL_Frame( int msec );
qboolean CL_GameCommand( void );
void CL_KeyEvent( int key, qboolean down, unsigned time );

void CL_CharEvent( int key );
// char events are for field typing, not game control

void CL_MouseEvent( int dx, int dy, int time );

void CL_JoystickEvent( int axis, int value, int time );

void CL_PacketEvent( netadr_t from, msg_t *msg );

void CL_ConsolePrint( char *text );

void CL_MapLoading( void );
// do a screen update before starting to load a map
// when the server is going to load a new map, the entire hunk
// will be cleared, so the client must shutdown cgame, ui, and
// the renderer

void    CL_ForwardCommandToServer( const char *string );
// adds the current command line as a clc_clientCommand to the client message.
// things like godmode, noclip, etc, are commands directed to the server,
// so when they are typed in at the console, they will need to be forwarded.

void CL_CDDialog( void );
// bring up the "need a cd to play" dialog

void CL_ShutdownAll( void );
// shutdown all the client stuff

void CL_FlushMemory( void );
// dump all memory on an error

void CL_StartHunkUsers( void );
// start all the client stuff using the hunk

void CL_CheckAutoUpdate( void );
qboolean CL_NextUpdateServer( void );
void CL_GetAutoUpdate( void );

void Key_WriteBindings( fileHandle_t f );
// for writing the config files

void S_ClearSoundBuffer( qboolean killStreaming );  //----(SA)	modified
// call before filesystem access

void SCR_DebugGraph( float value, int color );   // FIXME: move logging to common?


//
// server interface
//
void SV_Init( void );
void SV_Shutdown( char *finalmsg );
void SV_Frame( int msec );
void SV_PacketEvent( netadr_t from, msg_t *msg );
qboolean SV_GameCommand( void );


//
// UI interface
//
qboolean UI_GameCommand( void );
qboolean UI_usesUniqueCDKey();

/*
==============================================================

NON-PORTABLE SYSTEM SERVICES

==============================================================
*/

typedef enum {
	AXIS_SIDE,
	AXIS_FORWARD,
	AXIS_UP,
	AXIS_ROLL,
	AXIS_YAW,
	AXIS_PITCH,
	MAX_JOYSTICK_AXIS
} joystickAxis_t;

typedef enum {
	// bk001129 - make sure SE_NONE is zero
	SE_NONE = 0,    // evTime is still valid
	SE_KEY,     // evValue is a key code, evValue2 is the down flag
	SE_CHAR,    // evValue is an ascii char
	SE_MOUSE,   // evValue and evValue2 are reletive signed x / y moves
	SE_JOYSTICK_AXIS,   // evValue is an axis number and evValue2 is the current state (-127 to 127)
	SE_CONSOLE, // evPtr is a char*
	SE_PACKET   // evPtr is a netadr_t followed by data bytes to evPtrLength
} sysEventType_t;

typedef struct {
	int evTime;
	sysEventType_t evType;
	int evValue, evValue2;
	int evPtrLength;                // bytes of data pointed to by evPtr, for journaling
	void            *evPtr;         // this must be manually freed if not NULL
} sysEvent_t;

sysEvent_t  Sys_GetEvent( void );

void    Sys_Init( void );
qboolean Sys_IsNumLockDown( void );

void *Sys_InitializeCriticalSection();
void Sys_EnterCriticalSection( void *ptr );
void Sys_LeaveCriticalSection( void *ptr );

char* Sys_GetDLLName( const char *name );
// fqpath param added 2/15/02 by T.Ray - Sys_LoadDll is only called in vm.c at this time
void    * QDECL Sys_LoadDll( const char *name, char *fqpath, intptr_t( QDECL * *entryPoint ) ( int, ... ),
							 intptr_t ( QDECL * systemcalls )( int, ... ) );
void    Sys_UnloadDll( void *dllHandle );

void    Sys_UnloadGame( void );
void    *Sys_GetGameAPI( void *parms );

void    Sys_UnloadCGame( void );
void    *Sys_GetCGameAPI( void );

void    Sys_UnloadUI( void );
void    *Sys_GetUIAPI( void );

//bot libraries
void    Sys_UnloadBotLib( void );
void    *Sys_GetBotLibAPI( void *parms );

char    *Sys_GetCurrentUser( void );

void QDECL Sys_Error( const char *error, ... );
void    Sys_Quit( void );
char    *Sys_GetClipboardData( void );  // note that this isn't journaled...

void    Sys_Print( const char *msg );


// Sys_Milliseconds should only be used for profiling purposes,
// any game related timing information should come from event timestamps
int     Sys_Milliseconds( void );

void    Sys_SnapVector( float *v );

// the system console is shown when a dedicated server is running
void    Sys_DisplaySystemConsole( qboolean show );

int     Sys_GetProcessorId( void );

void    Sys_BeginStreamedFile( fileHandle_t f, int readahead );
void    Sys_EndStreamedFile( fileHandle_t f );
int     Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f );
void    Sys_StreamSeek( fileHandle_t f, int offset, int origin );

void    Sys_ShowConsole( int level, qboolean quitOnClose );
void    Sys_SetErrorText( const char *text );

void    Sys_SendPacket( int length, const void *data, netadr_t to );

qboolean    Sys_StringToAdr( const char *s, netadr_t *a );
//Does NOT parse port numbers, only base addresses.

qboolean    Sys_IsLANAddress( netadr_t adr );
void        Sys_ShowIP( void );

qboolean    Sys_CheckCD( void );

void    Sys_Mkdir( const char *path );
char    *Sys_Cwd( void );
char    *Sys_DefaultCDPath( void );
char    *Sys_DefaultBasePath( void );
char    *Sys_DefaultInstallPath( void );
char    *Sys_DefaultHomePath( void );

char **Sys_ListFiles( const char *directory, const char *extension, char *filter, int *numfiles, qboolean wantsubs );
void    Sys_FreeFileList( char **list );

void    Sys_BeginProfiling( void );
void    Sys_EndProfiling( void );

qboolean Sys_LowPhysicalMemory();
unsigned int Sys_ProcessorCount();

// NOTE TTimo - on win32 the cwd is prepended .. non portable behaviour
void Sys_StartProcess( char *exeName, qboolean doexit );            // NERVE - SMF
void Sys_OpenURL( const char *url, qboolean doexit );                       // NERVE - SMF
int Sys_GetHighQualityCPU();
float Sys_GetCPUSpeed( void );

#ifdef __linux__
// TTimo only on linux .. maybe on Mac too?
// will OR with the existing mode (chmod ..+..)
void Sys_Chmod( char *file, int mode );
#endif

/* This is based on the Adaptive Huffman algorithm described in Sayood's Data
 * Compression book.  The ranks are not actually stored, but implicitly defined
 * by the location of a node within a doubly-linked list */

#define NYT HMAX                    /* NYT = Not Yet Transmitted */
#define INTERNAL_NODE ( HMAX + 1 )

typedef struct nodetype {
	struct  nodetype *left, *right, *parent; /* tree structure */
	struct  nodetype *next, *prev; /* doubly-linked list */
	struct  nodetype **head; /* highest ranked node in block */
	int weight;
	int symbol;
} node_t;

#define HMAX 256 /* Maximum symbol */

typedef struct {
	int blocNode;
	int blocPtrs;

	node_t*     tree;
	node_t*     lhead;
	node_t*     ltail;
	node_t*     loc[HMAX + 1];
	node_t**    freelist;

	node_t nodeList[768];
	node_t*     nodePtrs[768];
} huff_t;

typedef struct {
	huff_t compressor;
	huff_t decompressor;
} huffman_t;

void    Huff_Compress( msg_t *buf, int offset );
void    Huff_Decompress( msg_t *buf, int offset );
void    Huff_Init( huffman_t *huff );
void    Huff_addRef( huff_t* huff, byte ch );
int     Huff_Receive( node_t *node, int *ch, byte *fin );
void    Huff_transmit( huff_t *huff, int ch, byte *fout );
void    Huff_offsetReceive( node_t *node, int *ch, byte *fin, int *offset );
void    Huff_offsetTransmit( huff_t *huff, int ch, byte *fout, int *offset );
void    Huff_putBit( int bit, byte *fout, int *offset );
int     Huff_getBit( byte *fout, int *offset );

extern huffman_t clientHuffTables;

#define SV_ENCODE_START     4
#define SV_DECODE_START     12
#define CL_ENCODE_START     12
#define CL_DECODE_START     4

void Com_GetHunkInfo( int* hunkused, int* hunkexpected );

// TTimo
// dll checksuming stuff, centralizing OS-dependent parts
// *_SHIFT is the shifting we applied to the reference string

#if defined( _WIN32 )

// qagame_mp_x86.dll
#define SYS_DLLNAME_QAGAME_SHIFT 6
#define SYS_DLLNAME_QAGAME "wgmgskesve~><4jrr"

// cgame_mp_x86.dll
#define SYS_DLLNAME_CGAME_SHIFT 2
#define SYS_DLLNAME_CGAME "eicogaoraz:80fnn"

// ui_mp_x86.dll
#define SYS_DLLNAME_UI_SHIFT 5
#define SYS_DLLNAME_UI "zndrud}=;3iqq"

#elif defined( __linux__ )

// qagame.mp.i386.so
#define SYS_DLLNAME_QAGAME_SHIFT 6
#define SYS_DLLNAME_QAGAME "wgmgsk4sv4o9><4yu"

// cgame.mp.i386.so
#define SYS_DLLNAME_CGAME_SHIFT 2
#define SYS_DLLNAME_CGAME "eicog0or0k5:80uq"

// ui.mp.i386.so
#define SYS_DLLNAME_UI_SHIFT 5
#define SYS_DLLNAME_UI "zn3ru3n8=;3xt"

#elif __MACOS__

#ifdef _DEBUG
// qagame_d_mac
	#define SYS_DLLNAME_QAGAME_SHIFT 6
	#define SYS_DLLNAME_QAGAME "wgmgskejesgi"

// cgame_d_mac
	#define SYS_DLLNAME_CGAME_SHIFT 2
	#define SYS_DLLNAME_CGAME "eicogafaoce"

// ui_d_mac
	#define SYS_DLLNAME_UI_SHIFT 5
	#define SYS_DLLNAME_UI "zndidrfh"
#else
// qagame_mac
	#define SYS_DLLNAME_QAGAME_SHIFT 6
	#define SYS_DLLNAME_QAGAME "wgmgskesgi"

// cgame_mac
	#define SYS_DLLNAME_CGAME_SHIFT 2
	#define SYS_DLLNAME_CGAME "eicogaoce"

// ui_mac
	#define SYS_DLLNAME_UI_SHIFT 5
	#define SYS_DLLNAME_UI "zndrfh"
#endif

#else

#error unknown OS

#endif

#endif // _QCOMMON_H_
