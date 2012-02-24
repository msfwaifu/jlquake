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


/*****************************************************************************
 * name:		files.c
 *
 * desc:		handle based filesystem for Quake III Arena
 *
 *
 *****************************************************************************/


#include "../game/q_shared.h"
#include "qcommon.h"
#include "../../core/unzip.h"

// TTimo: moved to qcommon.h
// NOTE: could really do with a cvar
//#define	BASEGAME			"main"
#define DEMOGAME            "demomain"

// every time a new demo pk3 file is built, this checksum must be updated.
// the easiest way to get it is to just run the game and see what it spits out
// NOTE TTimo: always needs the 'u' for unsigned int (gcc)
#define DEMO_PAK_CHECKSUM   2985661941u

// if this is defined, the executable positively won't work with any paks other
// than the demo pak, even if productid is present.  This is only used for our
// last demo release to prevent the mac and linux users from using the demo
// executable with the production windows pak before the mac/linux products
// hit the shelves a little later
// NOW defined in build files
//#define PRE_RELEASE_TADEMO

#define MAX_SEARCH_PATHS    4096

struct packfile_t
{
	char		name[MAX_QPATH];
	int			filepos;
	int			filelen;
};

struct pack_t
{
	char		filename[MAX_OSPATH];
	FILE*		handle;
	int			numfiles;
	packfile_t*	files;
};

struct fileInPack_t
{
	char                    *name;      // name of the file
	unsigned long pos;                  // file info position in zip
	fileInPack_t*   next;       // next file in the hash
};

struct pack3_t
{
	char pakFilename[MAX_OSPATH];               // c:\quake3\baseq3\pak0.pk3
	char pakBasename[MAX_OSPATH];               // pak0
	char pakGamename[MAX_OSPATH];               // baseq3
	unzFile handle;                             // handle to zip file
	int checksum;                               // regular checksum
	int pure_checksum;                          // checksum for pure
	int numfiles;                               // number of files in pk3
	int referenced;                             // referenced file flags
	int hashSize;                               // hash table size (power of 2)
	fileInPack_t*   *hashTable;                 // hash table
	fileInPack_t*   buildBuffer;                // buffer with the filenames etc.
};

struct directory_t
{
	char path[MAX_OSPATH];              // c:\quake3
	char gamedir[MAX_OSPATH];           // baseq3
	char			fullname[MAX_OSPATH];
};

struct searchpath_t
{
	searchpath_t*next;

	pack_t*			pack;
	pack3_t      *pack3;      // only one of pack / dir will be non NULL
	directory_t *dir;
};

static Cvar      *fs_basegame;
static Cvar      *fs_gamedirvar;
extern Cvar      *fs_restrict;
extern searchpath_t    *fs_searchpaths;
static int fs_readCount;                    // total bytes read
static int fs_loadCount;                    // total files read
static int fs_loadStack;                    // total files in memory

// last valid game folder used
char lastValidBase[MAX_OSPATH];
char lastValidGame[MAX_OSPATH];

#ifdef FS_MISSING
FILE*       missingFiles = NULL;
#endif

void S_ClearSoundBuffer()
{
}

bool CL_WWWBadChecksum(const char* pakname)
{
	return false;
}

/*
=================
FS_LoadStack
return load stack
=================
*/
int FS_LoadStack() {
	return fs_loadStack;
}

void FS_CopyFileOS( char *from, char *to ) {
	FILE    *f;
	int len;
	byte    *buf;
	char *fromOSPath, *toOSPath;

	fromOSPath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, from );
	toOSPath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, to );

	//Com_Printf( "copy %s to %s\n", fromOSPath, toOSPath );

	if ( strstr( fromOSPath, "journal.dat" ) || strstr( fromOSPath, "journaldata.dat" ) ) {
		Com_Printf( "Ignoring journal files\n" );
		return;
	}

	f = fopen( fromOSPath, "rb" );
	if ( !f ) {
		return;
	}
	fseek( f, 0, SEEK_END );
	len = ftell( f );
	fseek( f, 0, SEEK_SET );

	// we are using direct malloc instead of Z_Malloc here, so it
	// probably won't work on a mac... Its only for developers anyway...
	buf = (byte*)malloc( len );
	if ( fread( buf, 1, len, f ) != len ) {
		Com_Error( ERR_FATAL, "Short read in FS_Copyfiles()\n" );
	}
	fclose( f );

	if ( FS_CreatePath( toOSPath ) ) {
		return;
	}

	f = fopen( toOSPath, "wb" );
	if ( !f ) {
		return;
	}
	if ( fwrite( buf, 1, len, f ) != len ) {
		Com_Error( ERR_FATAL, "Short write in FS_Copyfiles()\n" );
	}
	fclose( f );
	free( buf );
}

/*
===========
FS_FileCompare

Do a binary check of the two files, return qfalse if they are different, otherwise qtrue
===========
*/
qboolean FS_FileCompare( const char *s1, const char *s2 ) {
	FILE    *f1, *f2;
	int len1, len2, pos;
	byte    *b1, *b2, *p1, *p2;

	f1 = fopen( s1, "rb" );
	if ( !f1 ) {
		Com_Error( ERR_FATAL, "FS_FileCompare: %s does not exist\n", s1 );
	}

	f2 = fopen( s2, "rb" );
	if ( !f2 ) {  // this file is allowed to not be there, since it might not exist in the previous build
		fclose( f1 );
		return qfalse;
		//Com_Error( ERR_FATAL, "FS_FileCompare: %s does not exist\n", s2 );
	}

	// first do a length test
	pos = ftell( f1 );
	fseek( f1, 0, SEEK_END );
	len1 = ftell( f1 );
	fseek( f1, pos, SEEK_SET );

	pos = ftell( f2 );
	fseek( f2, 0, SEEK_END );
	len2 = ftell( f2 );
	fseek( f2, pos, SEEK_SET );

	if ( len1 != len2 ) {
		fclose( f1 );
		fclose( f2 );
		return qfalse;
	}

	// now do a binary compare
	b1 = (byte*)malloc( len1 );
	if ( fread( b1, 1, len1, f1 ) != len1 ) {
		Com_Error( ERR_FATAL, "Short read in FS_FileCompare()\n" );
	}
	fclose( f1 );

	b2 = (byte*)malloc( len2 );
	if ( fread( b2, 1, len2, f2 ) != len2 ) {
		Com_Error( ERR_FATAL, "Short read in FS_FileCompare()\n" );
	}
	fclose( f2 );

	//if (!memcmp(b1, b2, (int)min(len1,len2) )) {
	p1 = b1;
	p2 = b2;
	for ( pos = 0; pos < len1; pos++, p1++, p2++ )
	{
		if ( *p1 != *p2 ) {
			free( b1 );
			free( b2 );
			return qfalse;
		}
	}
	//}

	// they are identical
	free( b1 );
	free( b2 );
	return qtrue;
}

/*
==============
FS_Delete
TTimo - this was not in the 1.30 filesystem code
using fs_homepath for the file to remove
==============
*/
int FS_Delete( char *filename ) {
	char *ospath;

	if ( !fs_searchpaths ) {
		Com_Error( ERR_FATAL, "Filesystem call made without initialization\n" );
	}

	if ( !filename || filename[0] == 0 ) {
		return 0;
	}

	// for safety, only allow deletion from the save directory
	if ( String::NCmp( filename, "save/", 5 ) != 0 ) {
		return 0;
	}

	ospath = FS_BuildOSPath( fs_homepath->string, fs_gamedir, filename );

	if ( remove( ospath ) != -1 ) {  // success
		return 1;
	}


	return 0;
}

//============================================================================

void Com_AppendCDKey( const char *filename );
void Com_ReadCDKey( const char *filename );

/*
================
FS_Startup
================
*/
static void FS_Startup( const char *gameName ) {
	Cvar  *fs;

	Com_Printf( "----- FS_Startup -----\n" );

	FS_SharedStartup();
	fs_basegame = Cvar_Get( "fs_basegame", "", CVAR_INIT );
	fs_gamedirvar = Cvar_Get( "fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO );

	// add search path elements in reverse priority order
	if ( fs_cdpath->string[0] ) {
		FS_AddGameDirectory( fs_cdpath->string, gameName, ADDPACKS_None );
	}
	if ( fs_basepath->string[0] ) {
		FS_AddGameDirectory( fs_basepath->string, gameName, ADDPACKS_None );
	}
	// fs_homepath is somewhat particular to *nix systems, only add if relevant
	// NOTE: same filtering below for mods and basegame
	if ( fs_basepath->string[0] && String::ICmp( fs_homepath->string,fs_basepath->string ) ) {
		FS_AddGameDirectory( fs_homepath->string, gameName, ADDPACKS_None );
	}

	// check for additional base game so mods can be based upon other mods
	if ( fs_basegame->string[0] && !String::ICmp( gameName, BASEGAME ) && String::ICmp( fs_basegame->string, gameName ) ) {
		if ( fs_cdpath->string[0] ) {
			FS_AddGameDirectory( fs_cdpath->string, fs_basegame->string, ADDPACKS_None );
		}
		if ( fs_basepath->string[0] ) {
			FS_AddGameDirectory( fs_basepath->string, fs_basegame->string, ADDPACKS_None );
		}
		if ( fs_homepath->string[0] && String::ICmp( fs_homepath->string,fs_basepath->string ) ) {
			FS_AddGameDirectory( fs_homepath->string, fs_basegame->string, ADDPACKS_None );
		}
	}

	// check for additional game folder for mods
	if ( fs_gamedirvar->string[0] && !String::ICmp( gameName, BASEGAME ) && String::ICmp( fs_gamedirvar->string, gameName ) ) {
		if ( fs_cdpath->string[0] ) {
			FS_AddGameDirectory( fs_cdpath->string, fs_gamedirvar->string, ADDPACKS_None );
		}
		if ( fs_basepath->string[0] ) {
			FS_AddGameDirectory( fs_basepath->string, fs_gamedirvar->string, ADDPACKS_None );
		}
		if ( fs_homepath->string[0] && String::ICmp( fs_homepath->string,fs_basepath->string ) ) {
			FS_AddGameDirectory( fs_homepath->string, fs_gamedirvar->string, ADDPACKS_None );
		}
	}

	Com_ReadCDKey( BASEGAME );
	fs = Cvar_Get( "fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO );
	if ( fs && fs->string[0] != 0 ) {
		Com_AppendCDKey( fs->string );
	}

	// print the current search paths
	FS_Path_f();

	fs_gamedirvar->modified = qfalse; // We just loaded, it's not modified

	Com_Printf( "----------------------\n" );

#ifdef FS_MISSING
	if ( missingFiles == NULL ) {
		missingFiles = fopen( "\\missing.txt", "ab" );
	}
#endif
	Com_Printf( "%d files in pk3 files\n", fs_packFiles );
}


/*
===================
FS_SetRestrictions

Looks for product keys and restricts media add on ability
if the full version is not found
===================
*/
static void FS_SetRestrictions( void ) {
//	searchpath_t	*path;

#ifndef WOLF_SP_DEMO
	// if fs_restrict is set, don't even look for the id file,
	// which allows the demo release to be tested even if
	// the full game is present
	if ( !fs_restrict->integer ) {
		return; // no restrictions
	}
#endif
	Cvar_Set( "fs_restrict", "1" );

	Com_Printf( "\nRunning in restricted demo mode.\n\n" );

	// restart the filesystem with just the demo directory
	FS_Shutdown();

#ifdef WOLF_SP_DEMO
	FS_Startup( DEMOGAME );
#else
	FS_Startup( BASEGAME );
#endif
}

/*
=====================
FS_GamePureChecksum

Returns the checksum of the pk3 from which the server loaded the qagame.qvm
=====================
*/
const char *FS_GamePureChecksum( void ) {
	static char info[MAX_STRING_TOKENS];
	searchpath_t *search;

	info[0] = 0;

	for ( search = fs_searchpaths ; search ; search = search->next ) {
		// is the element a pak file?
		if ( search->pack3 ) {
			if ( search->pack3->referenced & FS_QAGAME_REF ) {
				String::Sprintf( info, sizeof( info ), "%d", search->pack3->checksum );
			}
		}
	}

	return info;
}

/*
================
FS_InitFilesystem

Called only at inital startup, not when the filesystem
is resetting due to a game change
================
*/
void FS_InitFilesystem( void ) {
	// allow command line parms to override our defaults
	// we have to specially handle this, because normal command
	// line variable sets don't happen until after the filesystem
	// has already been initialized
	fs_PrimaryBaseGame = BASEGAME;
	Com_StartupVariable( "fs_cdpath" );
	Com_StartupVariable( "fs_basepath" );
	Com_StartupVariable( "fs_homepath" );
	Com_StartupVariable( "fs_game" );
	Com_StartupVariable( "fs_copyfiles" );
	Com_StartupVariable( "fs_restrict" );

	// try to start up normally
	FS_Startup( BASEGAME );

	// see if we are going to allow add-ons
	FS_SetRestrictions();

	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	if ( FS_ReadFile( "default.cfg", NULL ) <= 0 ) {
		Com_Error( ERR_FATAL, "Couldn't load default.cfg" );
	}

	String::NCpyZ( lastValidBase, fs_basepath->string, sizeof( lastValidBase ) );
	String::NCpyZ( lastValidGame, fs_gamedirvar->string, sizeof( lastValidGame ) );
}


/*
================
FS_Restart
================
*/
void FS_Restart( int checksumFeed ) {

	// free anything we currently have loaded
	FS_Shutdown();

	// set the checksum feed
	fs_checksumFeed = checksumFeed;

	// clear pak references
	FS_ClearPakReferences( 0 );

	// try to start up normally
	FS_Startup( BASEGAME );

	// see if we are going to allow add-ons
	FS_SetRestrictions();

	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	if ( FS_ReadFile( "default.cfg", NULL ) <= 0 ) {
		// this might happen when connecting to a pure server not using BASEGAME/pak0.pk3
		// (for instance a TA demo server)
		if ( lastValidBase[0] ) {
			FS_PureServerSetLoadedPaks( "", "" );
			Cvar_Set( "fs_basepath", lastValidBase );
			Cvar_Set( "fs_gamedirvar", lastValidGame );
			lastValidBase[0] = '\0';
			lastValidGame[0] = '\0';
			Cvar_Set( "fs_restrict", "0" );
			FS_Restart( checksumFeed );
			Com_Error( ERR_DROP, "Invalid game folder\n" );
			return;
		}
		Com_Error( ERR_FATAL, "Couldn't load default.cfg" );
	}

	// bk010116 - new check before safeMode
	if ( String::ICmp( fs_gamedirvar->string, lastValidGame ) ) {
		// skip the wolfconfig.cfg if "safe" is on the command line
		if ( !Com_SafeMode() ) {
			Cbuf_AddText( "exec wolfconfig.cfg\n" );
		}
	}

	String::NCpyZ( lastValidBase, fs_basepath->string, sizeof( lastValidBase ) );
	String::NCpyZ( lastValidGame, fs_gamedirvar->string, sizeof( lastValidGame ) );

}

/*
=================
FS_ConditionalRestart
restart if necessary
=================
*/
qboolean FS_ConditionalRestart( int checksumFeed ) {
	if ( fs_gamedirvar->modified || checksumFeed != fs_checksumFeed ) {
		FS_Restart( checksumFeed );
		return qtrue;
	}
	return qfalse;
}
