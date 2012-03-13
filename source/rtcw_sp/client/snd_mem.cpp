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
 * name:		snd_mem.c
 *
 * desc:		sound caching
 *
 * $Archive: /Wolf5/src/client/snd_mem.c $
 *
 *****************************************************************************/

#include "snd_local.h"

/*
===============================================================================

WAV loading

===============================================================================
*/

static byte    *data_p;
static byte    *iff_end;
static byte    *last_chunk;
static byte    *iff_data;
static int iff_chunk_len;

/*
================
GetLittleShort
================
*/
static short GetLittleShort( void ) {
	short val = 0;
	val = *data_p;
	val = val + ( *( data_p + 1 ) << 8 );
	data_p += 2;
	return val;
}

/*
================
GetLittleLong
================
*/
static int GetLittleLong( void ) {
	int val = 0;
	val = *data_p;
	val = val + ( *( data_p + 1 ) << 8 );
	val = val + ( *( data_p + 2 ) << 16 );
	val = val + ( *( data_p + 3 ) << 24 );
	data_p += 4;
	return val;
}

/*
================
FindNextChunk
================
*/
static void FindNextChunk( char *name ) {
	while ( 1 )
	{
		data_p = last_chunk;

		if ( data_p >= iff_end ) { // didn't find the chunk
			data_p = NULL;
			return;
		}

		data_p += 4;
		iff_chunk_len = GetLittleLong();
		if ( iff_chunk_len < 0 ) {
			data_p = NULL;
			return;
		}
		data_p -= 8;
		last_chunk = data_p + 8 + ( ( iff_chunk_len + 1 ) & ~1 );
		if ( !String::NCmp( (char *)data_p, name, 4 ) ) {
			return;
		}
	}
}

/*
================
FindChunk
================
*/
static void FindChunk( char *name ) {
	last_chunk = iff_data;
	FindNextChunk( name );
}

/*
============
GetWavinfo
============
*/
static wavinfo_t GetWavinfo( char *name, byte *wav, int wavlength ) {
	wavinfo_t info;

	Com_Memset( &info, 0, sizeof( info ) );

	if ( !wav ) {
		return info;
	}

	iff_data = wav;
	iff_end = wav + wavlength;

// find "RIFF" chunk
	FindChunk( "RIFF" );
	if ( !( data_p && !String::NCmp( (char *)data_p + 8, "WAVE", 4 ) ) ) {
		Com_Printf( "Missing RIFF/WAVE chunks\n" );
		return info;
	}

// get "fmt " chunk
	iff_data = data_p + 12;
// DumpChunks ();

	FindChunk( "fmt " );
	if ( !data_p ) {
		Com_Printf( "Missing fmt chunk\n" );
		return info;
	}
	data_p += 8;
	info.format = GetLittleShort();
	info.channels = GetLittleShort();
	info.rate = GetLittleLong();
	data_p += 4 + 2;
	info.width = GetLittleShort() / 8;

	if ( info.format != 1 ) {
		Com_Printf( "Microsoft PCM format only\n" );
		return info;
	}


// find data chunk
	FindChunk( "data" );
	if ( !data_p ) {
		Com_Printf( "Missing data chunk\n" );
		return info;
	}

	data_p += 4;
	info.samples = GetLittleLong() / info.width;
	info.dataofs = data_p - wav;

	return info;
}

/*
================
ResampleSfx

resample / decimate to the current source rate
================
*/
static void ResampleSfx( sfx_t *sfx, int inrate, int inwidth, byte *data, qboolean compressed ) {
	int outcount;
	int srcsample;
	float stepscale;
	int i;
	int sample, samplefrac, fracstep;

	stepscale = (float)inrate / dma.speed;  // this is usually 0.5, 1, or 2

	outcount = sfx->Length / stepscale;
	sfx->Length = outcount;

	samplefrac = 0;
	fracstep = stepscale * 256;
	sfx->Data = new short[outcount];

	for ( i = 0 ; i < outcount ; i++ )
	{
		srcsample = samplefrac >> 8;
		samplefrac += fracstep;
		if ( inwidth == 2 ) {
			sample = LittleShort( ( (short *)data )[srcsample] );
		} else {
			sample = (int)( ( unsigned char )( data[srcsample] ) - 128 ) << 8;
		}
		sfx->Data[i] = sample;
	}
}

/*
================
ResampleSfx

resample / decimate to the current source rate
================
*/
static int ResampleSfxRaw( short *sfx, int inrate, int inwidth, int samples, byte *data ) {
	int outcount;
	int srcsample;
	float stepscale;
	int i;
	int sample, samplefrac, fracstep;

	stepscale = (float)inrate / dma.speed;  // this is usually 0.5, 1, or 2

	outcount = samples / stepscale;

	samplefrac = 0;
	fracstep = stepscale * 256;

	for ( i = 0 ; i < outcount ; i++ )
	{
		srcsample = samplefrac >> 8;
		samplefrac += fracstep;
		if ( inwidth == 2 ) {
			sample = LittleShort( ( (short *)data )[srcsample] );
		} else {
			sample = (int)( ( unsigned char )( data[srcsample] ) - 128 ) << 8;
		}
		sfx[i] = sample;
	}
	return outcount;
}


//=============================================================================

/*
==============
S_LoadSound

The filename may be different than sfx->name in the case
of a forced fallback of a player specific sound
==============
*/
qboolean S_LoadSound( sfx_t *sfx ) {
	byte    *data;
	short   *samples;
	wavinfo_t info;
	int size;

	// player specific sounds are never directly loaded
	if ( sfx->Name[0] == '*' ) {
		return qfalse;
	}

	// load it in
	size = FS_ReadFile( sfx->Name, (void **)&data );
	if ( !data ) {
		return qfalse;
	}

	info = GetWavinfo( sfx->Name, data, size );
	if ( info.channels != 1 ) {
		Com_Printf( "%s is a stereo wav file\n", sfx->Name );
		FS_FreeFile( data );
		return qfalse;
	}

	if ( info.width == 1 ) {
		Com_DPrintf( S_COLOR_YELLOW "WARNING: %s is a 8 bit wav file\n", sfx->Name );
	}

	if ( info.rate != 22050 ) {
		Com_DPrintf( S_COLOR_YELLOW "WARNING: %s is not a 22kHz wav file\n", sfx->Name );
	}

	samples = (short*)Hunk_AllocateTempMemory( info.samples * sizeof( short ) * 2 );

	sfx->LastTimeUsed = Sys_Milliseconds() + 1;

	// each of these compression schemes works just fine
	// but the 16bit quality is much nicer and with a local
	// install assured we can rely upon the sound memory
	// manager to do the right thing for us and page
	// sound in as needed


	sfx->Length = info.samples;
	sfx->Data = NULL;
	ResampleSfx( sfx, info.rate, info.width, data + info.dataofs, qfalse );
	Hunk_FreeTempMemory( samples );
	FS_FreeFile( data );

	return qtrue;
}
