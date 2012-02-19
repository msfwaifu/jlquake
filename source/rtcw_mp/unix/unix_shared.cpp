/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pwd.h>

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"

//=============================================================================

// Used to determine CD Path
static char cdPath[MAX_OSPATH];

// Used to determine local installation path
static char installPath[MAX_OSPATH];

// Used to determine where to store user-specific files
static char homePath[MAX_OSPATH];

/*
================
Sys_Milliseconds
================
*/
/* base time in seconds, that's our origin
   timeval:tv_sec is an int:
   assuming this wraps every 0x7fffffff - ~68 years since the Epoch (1970) - we're safe till 2038
   using unsigned long data type to work right with Sys_XTimeToSysTime */
unsigned long sys_timeBase = 0;
/* current time in ms, using sys_timeBase as origin
   NOTE: sys_timeBase*1000 + curtime -> ms since the Epoch
	 0x7fffffff ms - ~24 days
   although timeval:tv_usec is an int, I'm not sure wether it is actually used as an unsigned int
	 (which would affect the wrap period) */
int curtime;
int Sys_Milliseconds( void ) {
	struct timeval tp;

	gettimeofday( &tp, NULL );

	if ( !sys_timeBase ) {
		sys_timeBase = tp.tv_sec;
		return tp.tv_usec / 1000;
	}

	curtime = ( tp.tv_sec - sys_timeBase ) * 1000 + tp.tv_usec / 1000;

	return curtime;
}

#if !defined( DEDICATED )
/*
================
Sys_XTimeToSysTime
sub-frame timing of events returned by X
X uses the Time typedef - unsigned long
disable with in_subframe 0

 sys_timeBase*1000 is the number of ms since the Epoch of our origin
 xtime is in ms and uses the Epoch as origin
   Time data type is an unsigned long: 0xffffffff ms - ~49 days period
 I didn't find much info in the XWindow documentation about the wrapping
   we clamp sys_timeBase*1000 to unsigned long, that gives us the current origin for xtime
   the computation will still work if xtime wraps (at ~49 days period since the Epoch) after we set sys_timeBase

================
*/
extern cvar_t *in_subframe;
int Sys_XTimeToSysTime( unsigned long xtime ) {
	int ret, time, test;

	if ( !in_subframe->value ) {
		// if you don't want to do any event times corrections
		return Sys_Milliseconds();
	}

	// test the wrap issue
#if 0
	// reference values for test: sys_timeBase 0x3dc7b5e9 xtime 0x541ea451 (read these from a test run)
	// xtime will wrap in 0xabe15bae ms >~ 0x2c0056 s (33 days from Nov 5 2002 -> 8 Dec)
	//   NOTE: date -d '1970-01-01 UTC 1039384002 seconds' +%c
	// use sys_timeBase 0x3dc7b5e9+0x2c0056 = 0x3df3b63f
	// after around 5s, xtime would have wrapped around
	// we get 7132, the formula handles the wrap safely
	unsigned long xtime_aux,base_aux;
	int test;
//	Com_Printf("sys_timeBase: %p\n", sys_timeBase);
//	Com_Printf("xtime: %p\n", xtime);
	xtime_aux = 500; // 500 ms after wrap
	base_aux = 0x3df3b63f; // the base a few seconds before wrap
	test = xtime_aux - ( unsigned long )( base_aux * 1000 );
	Com_Printf( "xtime wrap test: %d\n", test );
#endif

	// show_bug.cgi?id=565
	// some X servers (like suse 8.1's) report weird event times
	// if the game is loading, resolving DNS, etc. we are also getting old events
	// so we only deal with subframe corrections that look 'normal'
	ret = xtime - ( unsigned long )( sys_timeBase * 1000 );
	time = Sys_Milliseconds();
	test = time - ret;
	//printf("delta: %d\n", test);
	if ( test < 0 || test > 30 ) { // in normal conditions I've never seen this go above
		return time;
	}

	return ret;
}
#endif

//#if 0 // bk001215 - see snapvector.nasm for replacement
#if ( defined __APPLE__ ) || defined __x86_64__ // rcg010206 - using this for PPC builds...
long fastftol( float f ) { // bk001213 - from win32/win_shared.c
	//static int tmp;
	//	__asm fld f
	//__asm fistp tmp
	//__asm mov eax, tmp
	return (long)f;
}

void Sys_SnapVector( float *v ) { // bk001213 - see win32/win_shared.c
	// bk001213 - old linux
	v[0] = rint( v[0] );
	v[1] = rint( v[1] );
	v[2] = rint( v[2] );
}
#endif


void    Sys_Mkdir( const char *path ) {
	mkdir( path, 0777 );
}

char *strlwr( char *s ) {
	if ( s == NULL ) { // bk001204 - paranoia
		assert( 0 );
		return s;
	}
	while ( *s ) {
		*s = tolower( *s );
		s++;
	}
	return s; // bk001204 - duh
}

//============================================

#define MAX_FOUND_FILES 0x1000

// bk001129 - new in 1.26
void Sys_ListFilteredFiles( const char *basedir, char *subdirs, char *filter, char **list, int *numfiles ) {
	char search[MAX_OSPATH], newsubdirs[MAX_OSPATH];
	char filename[MAX_OSPATH];
	DIR         *fdir;
	struct dirent *d;
	struct stat st;

	if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
		return;
	}

	if ( String::Length( subdirs ) ) {
		Com_sprintf( search, sizeof( search ), "%s/%s", basedir, subdirs );
	} else {
		Com_sprintf( search, sizeof( search ), "%s", basedir );
	}

	if ( ( fdir = opendir( search ) ) == NULL ) {
		return;
	}

	while ( ( d = readdir( fdir ) ) != NULL ) {
		Com_sprintf( filename, sizeof( filename ), "%s/%s", search, d->d_name );
		if ( stat( filename, &st ) == -1 ) {
			continue;
		}

		if ( st.st_mode & S_IFDIR ) {
			if ( String::ICmp( d->d_name, "." ) && String::ICmp( d->d_name, ".." ) ) {
				if ( String::Length( subdirs ) ) {
					Com_sprintf( newsubdirs, sizeof( newsubdirs ), "%s/%s", subdirs, d->d_name );
				} else {
					Com_sprintf( newsubdirs, sizeof( newsubdirs ), "%s", d->d_name );
				}
				Sys_ListFilteredFiles( basedir, newsubdirs, filter, list, numfiles );
			}
		}
		if ( *numfiles >= MAX_FOUND_FILES - 1 ) {
			break;
		}
		Com_sprintf( filename, sizeof( filename ), "%s/%s", subdirs, d->d_name );
		if ( !Com_FilterPath( filter, filename, qfalse ) ) {
			continue;
		}
		list[ *numfiles ] = CopyString( filename );
		( *numfiles )++;
	}

	closedir( fdir );
}

// bk001129 - in 1.17 this used to be
// char **Sys_ListFiles( const char *directory, const char *extension, int *numfiles, qboolean wantsubs )
char **Sys_ListFiles( const char *directory, const char *extension, char *filter, int *numfiles, qboolean wantsubs ) {
	struct dirent *d;
	// char *p; // bk001204 - unused
	DIR     *fdir;
	qboolean dironly = wantsubs;
	char search[MAX_OSPATH];
	int nfiles;
	char        **listCopy;
	char        *list[MAX_FOUND_FILES];
	//int			flag; // bk001204 - unused
	int i;
	struct stat st;

	int extLen;

	if ( filter ) {

		nfiles = 0;
		Sys_ListFilteredFiles( directory, "", filter, list, &nfiles );

		list[ nfiles ] = 0;
		*numfiles = nfiles;

		if ( !nfiles ) {
			return NULL;
		}

		listCopy = (char**)Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ) );
		for ( i = 0 ; i < nfiles ; i++ ) {
			listCopy[i] = list[i];
		}
		listCopy[i] = NULL;

		return listCopy;
	}

	if ( !extension ) {
		extension = "";
	}

	if ( extension[0] == '/' && extension[1] == 0 ) {
		extension = "";
		dironly = qtrue;
	}

	extLen = String::Length( extension );

	// search
	nfiles = 0;

	if ( ( fdir = opendir( directory ) ) == NULL ) {
		*numfiles = 0;
		return NULL;
	}

	while ( ( d = readdir( fdir ) ) != NULL ) {
		Com_sprintf( search, sizeof( search ), "%s/%s", directory, d->d_name );
		if ( stat( search, &st ) == -1 ) {
			continue;
		}
		if ( ( dironly && !( st.st_mode & S_IFDIR ) ) ||
			 ( !dironly && ( st.st_mode & S_IFDIR ) ) ) {
			continue;
		}

		if ( *extension ) {
			if ( String::Length( d->d_name ) < String::Length( extension ) ||
				 String::ICmp(
					 d->d_name + String::Length( d->d_name ) - String::Length( extension ),
					 extension ) ) {
				continue; // didn't match
			}
		}

		if ( nfiles == MAX_FOUND_FILES - 1 ) {
			break;
		}
		list[ nfiles ] = CopyString( d->d_name );
		nfiles++;
	}

	list[ nfiles ] = 0;

	closedir( fdir );

	// return a copy of the list
	*numfiles = nfiles;

	if ( !nfiles ) {
		return NULL;
	}

	listCopy = (char**)Z_Malloc( ( nfiles + 1 ) * sizeof( *listCopy ) );
	for ( i = 0 ; i < nfiles ; i++ ) {
		listCopy[i] = list[i];
	}
	listCopy[i] = NULL;

	return listCopy;
}

void    Sys_FreeFileList( char **list ) {
	int i;

	if ( !list ) {
		return;
	}

	for ( i = 0 ; list[i] ; i++ ) {
		Z_Free( list[i] );
	}

	Z_Free( list );
}

char *Sys_Cwd( void ) {
	static char cwd[MAX_OSPATH];

	getcwd( cwd, sizeof( cwd ) - 1 );
	cwd[MAX_OSPATH - 1] = 0;

	return cwd;
}

void Sys_SetDefaultCDPath( const char *path ) {
	String::NCpyZ( cdPath, path, sizeof( cdPath ) );
}

char *Sys_DefaultCDPath( void ) {
	return cdPath;
}

/*
==============
Sys_DefaultBasePath
==============
*/
char *Sys_DefaultBasePath( void ) {
	if ( installPath[0] ) {
		return installPath;
	} else {
		return Sys_Cwd();
	}
}

void Sys_SetDefaultInstallPath( const char *path ) {
	String::NCpyZ( installPath, path, sizeof( installPath ) );
}

char *Sys_DefaultInstallPath( void ) {
	if ( *installPath ) {
		return installPath;
	} else {
		return Sys_Cwd();
	}
}

void Sys_SetDefaultHomePath( const char *path ) {
	String::NCpyZ( homePath, path, sizeof( homePath ) );
}

char *Sys_DefaultHomePath( void ) {
	char *p;

	if ( *homePath ) {
		return homePath;
	}

	if ( ( p = getenv( "HOME" ) ) != NULL ) {
		String::NCpyZ( homePath, p, sizeof( homePath ) );
#ifdef MACOS_X
		String::Cat( homePath, sizeof( homePath ), "/Library/Application Support/WolfensteinMP" );
#else
		String::Cat( homePath, sizeof( homePath ), "/.wolf" );
#endif
		if ( mkdir( homePath, 0777 ) ) {
			if ( errno != EEXIST ) {
				Sys_Error( "Unable to create directory \"%s\", error is %s(%d)\n", homePath, strerror( errno ), errno );
			}
		}
		return homePath;
	}
	return ""; // assume current dir
}

//============================================

int Sys_GetProcessorId( void ) {
	// TODO TTimo add better CPU identification?
	// see Sys_GetHighQualityCPU
	return CPUID_GENERIC;
}

int Sys_GetHighQualityCPU() {
	// TODO TTimo see win_shared.c IsP3 || IsAthlon
	return 0;
}

void Sys_ShowConsole( int visLevel, qboolean quitOnClose ) {
}

char *Sys_GetCurrentUser( void ) {
	struct passwd *p;

	if ( ( p = getpwuid( getuid() ) ) == NULL ) {
		return "player";
	}
	return p->pw_name;
}

void *Sys_InitializeCriticalSection() {
	return (void *)-1;
}

void Sys_EnterCriticalSection( void *ptr ) {
}

void Sys_LeaveCriticalSection( void *ptr ) {
}
