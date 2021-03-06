//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#ifndef __FILES_H__
#define __FILES_H__

#include "containers/List.h"

struct Cvar;

#if defined __MACOS__
#define PATH_SEP    ':'
#elif defined _WIN32
#define PATH_SEP    '\\'
#else
#define PATH_SEP    '/'
#endif

#define MAX_QPATH           64		// max length of a quake game pathname
#ifdef PATH_MAX
#define MAX_OSPATH          PATH_MAX
#else
#define MAX_OSPATH          256		// max length of a filesystem pathname
#endif

typedef int fileHandle_t;

enum fsOrigin_t
{
	FS_SEEK_CUR,
	FS_SEEK_END,
	FS_SEEK_SET
};

// referenced flags
// these are in loop specific order so don't change the order
#define FS_GENERAL_REF  0x01
#define FS_UI_REF       0x02
#define FS_CGAME_REF    0x04
#define FS_QAGAME_REF   0x08

// mode parm for FS_FOpenFile
enum fsMode_t
{
	FS_READ,
	FS_WRITE,
	FS_APPEND,
	FS_APPEND_SYNC
};

#define FS_EXCLUDE_DIR 0x1
#define FS_EXCLUDE_PK3 0x2

extern bool com_portals;

// never load anything from pk3 files that are not present at the server when pure
extern Cvar* fs_homepath;
extern Cvar* fs_basepath;
extern Cvar* fs_gamedirvar;

extern bool fs_ProtectKeyFile;
extern char fs_gamedir[ MAX_OSPATH ];
extern char fsqhw_gamedirfile[ MAX_OSPATH ];	//JL What's the difference?
extern const char* fs_PrimaryBaseGame;

void FS_CopyFile( const char* fromOSPath, const char* toOSPath );
void FS_CopyFileOS( const char* from, const char* to );
char* FS_NextPath( char* prevpath );
int FS_GetQuake2GameType();

bool FS_Initialized();

char* FS_BuildOSPath( const char* Base, const char* Game, const char* QPath );

bool FS_CreatePath( const char* OSPath );

bool FS_FileExists( const char* file );

int FS_FOpenFileRead( const char* FileName, fileHandle_t* File, bool UniqueFile );
// if uniqueFILE is true, then a new FILE will be fopened even if the file
// is found in an already open pak file.  If uniqueFILE is false, you must call
// FS_FCloseFile instead of fclose, otherwise the pak FILE would be improperly closed
// It is generally safe to always set uniqueFILE to true, because the majority of
// file IO goes through FS_ReadFile, which Does The Right Thing already.

int FS_FOpenFileRead_Filtered( const char* qpath, fileHandle_t* file, bool uniqueFILE, int filter_flag );
// added exclude flag to filter out regular dirs or pack files on demand
// would rather have used FS_FOpenFileRead(..., int filter_flag = 0)
// but that's a C++ construct ..

fileHandle_t FS_FOpenFileWrite( const char* filename );
// will properly create any needed paths and deal with seperater character issues

int FS_FOpenFileByMode( const char* qpath, fileHandle_t* f, fsMode_t mode );
// opens a file for reading, writing, or appending depending on the value of mode

void FS_FCloseFile( fileHandle_t f );

int FS_Read( void* buffer, int len, fileHandle_t f );
// properly handles partial reads and reads from other dlls

int FS_Write( const void* buffer, int len, fileHandle_t f );

void FS_Printf( fileHandle_t f, const char* fmt, ... );
// like fprintf

void FS_Flush( fileHandle_t f );

int FS_Seek( fileHandle_t f, int offset, int origin );
// seek on a file (doesn't work for zip files!!!!!!!!)

int FS_FTell( fileHandle_t f );
// where are we?

void FS_Remove( const char* osPath );

void FS_Rename( const char* from, const char* to );

int FS_Delete( const char* filename );		// only works inside the 'save' directory (for deleting savegames/images)

int FS_SV_FOpenFileRead( const char* filename, fileHandle_t* fp );
fileHandle_t FS_SV_FOpenFileWrite( const char* filename );
void FS_SV_Rename( const char* from, const char* to );

void FS_ForceFlush( fileHandle_t f );
// forces flush on files we're writing to.

int FS_FileIsInPAK( const char* filename, int* pChecksum );
// returns 1 if a file is in the PAK file, otherwise -1

int FS_ReadFile( const char* qpath, void** buffer );
int FS_ReadFile( const char* qpath, idList<byte>& Buffer );
// returns the length of the file
// a null buffer will just return the file length without loading
// as a quick check for existance. -1 length == not present
// A 0 byte will always be appended at the end, so string ops are safe.
// the buffer should be considered read-only, because it may be cached
// for other uses.

void FS_FreeFile( void* buffer );
// frees the memory returned by FS_ReadFile

void FS_WriteFile( const char* qpath, const void* buffer, int size );
// writes a complete file, creating any subdirectories needed

char** FS_ListFiles( const char* directory, const char* extension, int* numfiles );
// directory should not have either a leading or trailing /
// if extension is "/", only subdirectories will be returned
// the returned files will not include any directories or /

void FS_FreeFileList( char** list );

int FS_GetFileList( const char* path, const char* extension, char* listbuf, int bufsize );
int FS_GetModList( char* listbuf, int bufsize );

const char* FS_LoadedPakNames();
const char* FS_LoadedPakChecksums();
const char* FS_LoadedPakPureChecksums();
// Returns a space separated string containing the checksums of all loaded pk3 files.
// Servers with sv_pure set will get this string and pass it to clients.

const char* FS_ReferencedPakNames();
const char* FS_ReferencedPakChecksums();
const char* FS_ReferencedPakPureChecksums();
// Returns a space separated string containing the checksums of all loaded
// AND referenced pk3 files. Servers with sv_pure set will get this string
// back from clients for pure validation

void FS_ClearPakReferences( int flags );
// clears referenced booleans on loaded pk3s

void FS_PureServerSetReferencedPaks( const char* pakSums, const char* pakNames );
void FS_PureServerSetLoadedPaks( const char* pakSums, const char* pakNames );
// If the string is empty, all data sources will be allowed.
// If not empty, only pk3 files that match one of the space
// separated checksums will be checked for files, with the
// sole exception of .cfg files.

bool FS_idPak( const char* pak, const char* base );
bool FS_ComparePaks( char* neededpaks, int len, bool dlstring );

bool FS_VerifyOfficialPaks();

bool FS_VerifyPak( const char* pak );

bool FS_CL_ExtractFromPakFile( const char* path, const char* gamedir, const char* filename );

void FS_InitGame( bool dedicatedBuild );
void FS_InitFilesystem();
void FS_Shutdown();

// shutdown and restart the filesystem so changes to fs_gamedir can take effect
void FS_Restart( int checksumFeed );

const char* FS_Gamedir();
void FS_SetGamedirQHW( const char* dir );
void FS_SetGamedir( const char* dir );
void FS_ExecAutoexec();
bool FS_ConditionalRestart( int checksumFeed );

#endif
