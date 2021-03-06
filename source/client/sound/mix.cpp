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
//**
//**	Portable code to mix sounds for snd_dma.cpp
//**
//**************************************************************************

#include "local.h"
#include "../client_main.h"
#include "../../common/Common.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"
#include "../../common/command_buffer.h"

#define TALK_FUTURE_SEC 0.25		// go this far into the future (seconds)

unsigned char s_entityTalkAmplitude[ MAX_CLIENTS_WS ];

portable_samplepair_t paintbuffer[ PAINTBUFFER_SIZE ];
extern "C"
{
int* snd_p;
int snd_linear_count;
short* snd_out;
}
static int snd_vol;

#if id386 && defined _MSC_VER
static __declspec( naked ) void S_WriteLinearBlastStereo16() {
	__asm
	{
		push edi
		push ebx
		mov ecx,ds : dword ptr[snd_linear_count]
		mov ebx,ds : dword ptr[snd_p]
		mov edi,ds : dword ptr[snd_out]
	LWLBLoopTop :
		mov eax,ds : dword ptr[-8 + ebx + ecx * 4]
		sar eax,8
		cmp eax,07FFFh
		jg LClampHigh
		cmp eax,0FFFF8000h
		jnl LClampDone
		mov eax,0FFFF8000h
		jmp LClampDone
	LClampHigh :
		mov eax,07FFFh
		LClampDone :
		mov edx,ds : dword ptr[-4 + ebx + ecx * 4]
		sar edx,8
		cmp edx,07FFFh
		jg LClampHigh2
		cmp edx,0FFFF8000h
		jnl LClampDone2
		mov edx,0FFFF8000h
		jmp LClampDone2
	LClampHigh2 :
		mov edx,07FFFh
	LClampDone2 :
		shl edx,16
		and eax,0FFFFh
		or edx,eax
		mov ds : dword ptr[-4 + edi + ecx * 2],edx
		sub ecx,2
		jnz LWLBLoopTop
		pop ebx
		pop edi
		ret
	}
}
#elif id386 && defined __GNUC__
// forward declare, implementation somewhere else
extern "C" void S_WriteLinearBlastStereo16();
#else
static void S_WriteLinearBlastStereo16() {
	for ( int i = 0; i < snd_linear_count; i += 2 ) {
		int val = snd_p[ i ] >> 8;
		if ( val > 0x7fff ) {
			snd_out[ i ] = 0x7fff;
		} else if ( val < -32768 ) {
			snd_out[ i ] = -32768;
		} else {
			snd_out[ i ] = val;
		}

		val = snd_p[ i + 1 ] >> 8;
		if ( val > 0x7fff ) {
			snd_out[ i + 1 ] = 0x7fff;
		} else if ( val < -32768 ) {
			snd_out[ i + 1 ] = -32768;
		} else {
			snd_out[ i + 1 ] = val;
		}
	}
}
#endif

static void S_TransferStereo16( int endtime ) {
	snd_p = ( int* )paintbuffer;
	int lpaintedtime = s_paintedtime;

	while ( lpaintedtime < endtime ) {
		// handle recirculating buffer issues
		int lpos = lpaintedtime & ( ( dma.samples >> 1 ) - 1 );

		snd_out = ( short* )dma.buffer + ( lpos << 1 );

		snd_linear_count = ( dma.samples >> 1 ) - lpos;
		if ( lpaintedtime + snd_linear_count > endtime ) {
			snd_linear_count = endtime - lpaintedtime;
		}

		snd_linear_count <<= 1;

		// write a linear blast of samples
		S_WriteLinearBlastStereo16();

		snd_p += snd_linear_count;
		lpaintedtime += ( snd_linear_count >> 1 );
	}
}

static void S_TransferPaintBuffer( int endtime ) {
	if ( !dma.buffer ) {
		return;
	}

	if ( s_testsound->integer ) {
		// write a fixed sine wave
		int count = endtime - s_paintedtime;
		for ( int i = 0; i < count; i++ ) {
			paintbuffer[ i ].left = paintbuffer[ i ].right =
										sin( ( s_paintedtime + i ) * 0.1 ) * 20000 * 256;
		}
	}

	if ( dma.samplebits == 16 && dma.channels == 2 ) {
		// optimized case
		S_TransferStereo16( endtime );
	} else {
		// general case
		int* p = ( int* )paintbuffer;
		int count = ( endtime - s_paintedtime ) * dma.channels;
		int out_mask = dma.samples - 1;
		int out_idx = s_paintedtime * dma.channels & out_mask;
		int step = 3 - dma.channels;

		if ( dma.samplebits == 16 ) {
			short* out = ( short* )dma.buffer;
			while ( count-- ) {
				int val = *p >> 8;
				p += step;
				if ( val > 0x7fff ) {
					val = 0x7fff;
				} else if ( val < -32768 ) {
					val = -32768;
				}
				out[ out_idx ] = val;
				out_idx = ( out_idx + 1 ) & out_mask;
			}
		} else if ( dma.samplebits == 8 ) {
			byte* out = dma.buffer;
			while ( count-- ) {
				int val = *p >> 8;
				p += step;
				if ( val > 0x7fff ) {
					val = 0x7fff;
				} else if ( val < -32768 ) {
					val = -32768;
				}
				out[ out_idx ] = ( val >> 8 ) + 128;
				out_idx = ( out_idx + 1 ) & out_mask;
			}
		}
	}
}

//**************************************************************************
//
//	LIP SYNCING
//
//**************************************************************************

static void S_SetVoiceAmplitudeFrom16( const sfx_t* sc, int sampleOffset, int count, int entnum ) {
	if ( count <= 0 ) {
		return;	// must have gone ahead of the end of the sound
	}
	int sfx_count = 0;
	short* samples = sc->Data;
	for ( int i = 0; i < count; i++ ) {
		int data  = samples[ sampleOffset++ ];
		if ( abs( data ) > 5000 ) {
			sfx_count += ( data * 255 ) >> 8;
		}
	}
	// adjust the sfx_count according to the frametime (scale down for longer frametimes)
	sfx_count = abs( sfx_count );
	sfx_count = ( int )( ( float )sfx_count / ( 2.0 * ( float )count ) );
	if ( sfx_count > 255 ) {
		sfx_count = 255;
	}
	if ( sfx_count < 25 ) {
		sfx_count = 0;
	}
	// update the amplitude for this entity
	s_entityTalkAmplitude[ entnum ] = ( unsigned char )sfx_count;
}

int S_GetVoiceAmplitude( int entityNum ) {
	if ( !( GGameType & ( GAME_WolfSP | GAME_ET ) ) ) {
		return 0;
	}
	if ( entityNum >= ( GGameType & GAME_ET ? MAX_CLIENTS_ET : MAX_CLIENTS_WS ) ) {
		common->Printf( "Error: S_GetVoiceAmplitude() called for a non-client\n" );
		return 0;
	}

	return ( int )s_entityTalkAmplitude[ entityNum ];
}

//**************************************************************************
//
//	Wave file saving functions
//
//**************************************************************************

struct wav_hdr_t {
	unsigned int ChunkID;		// big endian
	unsigned int ChunkSize;		// little endian
	unsigned int Format;		// big endian

	unsigned int Subchunk1ID;	// big endian
	unsigned int Subchunk1Size;	// little endian
	unsigned short AudioFormat;	// little endian
	unsigned short NumChannels;	// little endian
	unsigned int SampleRate;	// little endian
	unsigned int ByteRate;		// little endian
	unsigned short BlockAlign;	// little endian
	unsigned short BitsPerSample;	// little endian

	unsigned int Subchunk2ID;	// big endian
	unsigned int Subchunk2Size;		// little indian ;)

	unsigned int NumSamples;
};

static wav_hdr_t hdr;

static portable_samplepair_t wavbuffer[ PAINTBUFFER_SIZE ];

static void CL_WavFilename( int number, char* fileName ) {
	if ( number < 0 || number > 9999 ) {
		String::Sprintf( fileName, MAX_QPATH, "wav9999" );		// fretn - removed .tga
		return;
	}

	String::Sprintf( fileName, MAX_QPATH, "wav%04i", number );
}

static void CL_WriteWaveHeader() {
	memset( &hdr, 0, sizeof ( hdr ) );

	hdr.ChunkID = 0x46464952;		// "RIFF"
	hdr.ChunkSize = 0;			// total filesize - 8 bytes
	hdr.Format = 0x45564157;		// "WAVE"

	hdr.Subchunk1ID = 0x20746d66;		// "fmt "
	hdr.Subchunk1Size = 16;			// 16 = pcm
	hdr.AudioFormat = 1;			// 1 = linear quantization
	hdr.NumChannels = 2;			// 2 = stereo

	hdr.SampleRate = dma.speed;

	hdr.BitsPerSample = 16;			// 16bits

	// SampleRate * NumChannels * BitsPerSample/8
	hdr.ByteRate = hdr.SampleRate * hdr.NumChannels * ( hdr.BitsPerSample / 8 );

	// NumChannels * BitsPerSample/8
	hdr.BlockAlign = hdr.NumChannels * ( hdr.BitsPerSample / 8 );

	hdr.Subchunk2ID = 0x61746164;		// "data"

	hdr.Subchunk2Size = 0;			// NumSamples * NumChannels * BitsPerSample/8

	// ...
	FS_Write( &hdr.ChunkID, 44, clc.wm_wavefile );
}

void CL_WriteWaveOpen() {
	if ( Cmd_Argc() > 2 ) {
		common->Printf( "wav_record <wavname>\n" );
		return;
	}

	if ( clc.wm_waverecording ) {
		common->Printf( "Already recording a wav file\n" );
		return;
	}

	char wavName[ MAX_QPATH ];
	char name[ MAX_QPATH ];
	if ( Cmd_Argc() == 2 ) {
		const char* s = Cmd_Argv( 1 );
		String::NCpyZ( wavName, s, sizeof ( wavName ) );
		String::Sprintf( name, sizeof ( name ), "wav/%s.wav", wavName );
	} else {
		for ( int number = 0; number <= 9999; number++ ) {
			char wavName[ MAX_QPATH ];
			CL_WavFilename( number, wavName );
			String::Sprintf( name, sizeof ( name ), "wav/%s.wav", wavName );

			int len = FS_FileExists( name );
			if ( len <= 0 ) {
				break;	// file doesn't exist
			}
		}
	}

	common->Printf( "recording to %s.\n", name );
	clc.wm_wavefile = FS_FOpenFileWrite( name );

	if ( !clc.wm_wavefile ) {
		common->Printf( "ERROR: couldn't open %s for writing.\n", name );
		return;
	}

	CL_WriteWaveHeader();
	clc.wm_wavetime = -1;

	clc.wm_waverecording = true;

	if ( GGameType & GAME_ET ) {
		Cvar_Set( "cl_waverecording", "1" );
		Cvar_Set( "cl_wavefilename", wavName );
		Cvar_Set( "cl_waveoffset", "0" );
	}
}

void CL_WriteWaveClose() {
	common->Printf( "Stopped recording\n" );

	hdr.Subchunk2Size = hdr.NumSamples * hdr.NumChannels * ( hdr.BitsPerSample / 8 );
	hdr.ChunkSize = 36 + hdr.Subchunk2Size;

	FS_Seek( clc.wm_wavefile, 4, FS_SEEK_SET );
	FS_Write( &hdr.ChunkSize, 4, clc.wm_wavefile );
	FS_Seek( clc.wm_wavefile, 40, FS_SEEK_SET );
	FS_Write( &hdr.Subchunk2Size, 4, clc.wm_wavefile );

	// and we're outta here
	FS_FCloseFile( clc.wm_wavefile );
	clc.wm_wavefile = 0;
}

void CL_WriteWaveFilePacket( int endtime ) {
	if ( !clc.wm_waverecording || !clc.wm_wavefile ) {
		return;
	}

	if ( clc.wm_wavetime == -1 ) {
		clc.wm_wavetime = s_soundtime;

		memcpy( wavbuffer, paintbuffer, sizeof ( wavbuffer ) );
		return;
	}

	if ( s_soundtime <= clc.wm_wavetime ) {
		return;
	}

	int total = s_soundtime - clc.wm_wavetime;

	if ( total < 1 ) {
		return;
	}

	clc.wm_wavetime = s_soundtime;

	for ( int i = 0; i < total; i++ ) {
		int parm;
		short out;

		parm = ( wavbuffer[ i ].left ) >> 8;
		if ( parm > 32767 ) {
			parm = 32767;
		}
		if ( parm < -32768 ) {
			parm = -32768;
		}
		out = parm;
		FS_Write( &out, 2, clc.wm_wavefile );

		parm = ( wavbuffer[ i ].right ) >> 8;
		if ( parm > 32767 ) {
			parm = 32767;
		}
		if ( parm < -32768 ) {
			parm = -32768;
		}
		out = parm;
		FS_Write( &out, 2, clc.wm_wavefile );
		hdr.NumSamples++;
	}
	memcpy( wavbuffer, paintbuffer, sizeof ( wavbuffer ) );

	if ( GGameType & GAME_ET ) {
		Cvar_Set( "cl_waveoffset", va( "%d", FS_FTell( clc.wm_wavefile ) ) );
	}
}

//**************************************************************************
//
//	CHANNEL MIXING
//
//**************************************************************************

static void S_PaintChannelFrom16( channel_t* ch, const sfx_t* sc, int count,
	int sampleOffset, int bufferOffset ) {
	portable_samplepair_t* samp = &paintbuffer[ bufferOffset ];

	if ( !ch->doppler || ch->dopplerScale == 1.0f ) {
		int leftvol = ch->leftvol * snd_vol;
		int rightvol = ch->rightvol * snd_vol;
		short* samples = sc->Data;
		for ( int i = 0; i < count; i++ ) {
			int data  = samples[ sampleOffset++ ];
			samp[ i ].left += ( data * leftvol ) >> 8;
			samp[ i ].right += ( data * rightvol ) >> 8;
		}
	} else {
		sampleOffset = sampleOffset * ch->oldDopplerScale;

		float fleftvol = ch->leftvol * snd_vol;
		float frightvol = ch->rightvol * snd_vol;

		float ooff = sampleOffset;
		short* samples = sc->Data;

		for ( int i = 0; i < count; i++ ) {
			int aoff = ooff;
			ooff = ooff + ch->dopplerScale;
			int boff = ooff;
			float fdata = 0;
			for ( int j = aoff; j < boff; j++ ) {
				fdata  += samples[ j ];
			}
			float fdiv = 256 * ( boff - aoff );
			samp[ i ].left += ( fdata * fleftvol ) / fdiv;
			samp[ i ].right += ( fdata * frightvol ) / fdiv;
		}
	}
}

void S_PaintChannels( int endtime ) {
	int i, si;
	int end;
	channel_t* ch;
	sfx_t* sc;
	int ltime, count;
	int sampleOffset;
	streamingSound_t* ss;
	bool firstPass = true;

	if ( s_mute->value ) {
		snd_vol = 0;
	} else {
		snd_vol = s_volume->value * 255;
	}

	if ( s_volCurrent < 1 ) {
		// only when fading (at map start/end)
		snd_vol = ( int )( ( float )snd_vol * s_volCurrent );
	}

	while ( s_paintedtime < endtime ) {
		// if paintbuffer is smaller than DMA buffer
		// we may need to fill it multiple times
		end = endtime;
		if ( endtime - s_paintedtime > PAINTBUFFER_SIZE ) {
			end = s_paintedtime + PAINTBUFFER_SIZE;
		}

		// start any playsounds
		while ( 1 ) {
			playsound_t* ps = s_pendingplays.next;
			if ( ps == &s_pendingplays ) {
				break;	// no more pending sounds
			}
			if ( ps->begin <= s_paintedtime ) {
				S_IssuePlaysound( ps );
				continue;
			}
			if ( ps->begin < end ) {
				end = ps->begin;		// stop here
			}
			break;
		}

		// clear pain buffer for the current time
		Com_Memset( paintbuffer, 0, ( end - s_paintedtime ) * sizeof ( portable_samplepair_t ) );
		// mix all streaming sounds into paint buffer
		for ( si = 0, ss = streamingSounds; si < MAX_STREAMING_SOUNDS; si++, ss++ ) {
			// if this streaming sound is still playing
			if ( s_rawend[ si ] >= s_paintedtime ) {
				// copy from the streaming sound source
				int s;
				int stop;

				stop = ( end < s_rawend[ si ] ) ? end : s_rawend[ si ];

				for ( i = s_paintedtime; i < stop; i++ ) {
					s = i & ( MAX_RAW_SAMPLES - 1 );
					paintbuffer[ i - s_paintedtime ].left += ( s_rawsamples[ si ][ s ].left * s_rawVolume[ si ].left ) >> 8;
					paintbuffer[ i - s_paintedtime ].right += ( s_rawsamples[ si ][ s ].right * s_rawVolume[ si ].right ) >> 8;
				}

				// rain - the announcer is ent -1, so make sure we're >= 0
				if ( ( GGameType & ( GAME_WolfSP | GAME_ET ) ) && firstPass && ss->channel == Q3CHAN_VOICE &&
					 ss->entnum >= 0 && ss->entnum < ( GGameType & GAME_ET ? MAX_CLIENTS_ET : MAX_CLIENTS_WS ) ) {
					int talktime;
					int sfx_count, vstop;
					int data;

					// we need to go into the future, since the interpolated behaviour of the facial
					// animation creates lag in the time it takes to display the current facial frame
					talktime = s_paintedtime + ( int )( TALK_FUTURE_SEC * ( float )s_khz->integer * 1000 );
					vstop = ( talktime + 100 < s_rawend[ si ] ) ? talktime + 100 : s_rawend[ si ];
					sfx_count = 0;

					for ( i = talktime; i < vstop; i++ ) {
						s = i & ( MAX_RAW_SAMPLES - 1 );
						data = abs( ( s_rawsamples[ si ][ s ].left ) / 8000 );
						if ( data > sfx_count ) {
							sfx_count = data;
						}
					}

					if ( sfx_count > 255 ) {
						sfx_count = 255;
					}
					if ( sfx_count < 25 ) {
						sfx_count = 0;
					}

					// update the amplitude for this entity
					s_entityTalkAmplitude[ ss->entnum ] = ( unsigned char )sfx_count;
				}
			}
		}

		// paint in the channels.
		ch = s_channels;
		for ( i = 0; i < MAX_CHANNELS; i++, ch++ ) {
			if ( ch->startSample == START_SAMPLE_IMMEDIATE || !ch->sfx ||
				 ( !ch->leftvol && !ch->rightvol ) ) {
				//(ch->leftvol < 0.25 && ch->rightvol < 0.25))
				continue;
			}

			ltime = s_paintedtime;
			sc = ch->sfx;

			while ( ltime < end ) {
				sampleOffset = ltime - ch->startSample;
				count = end - ltime;
				if ( sampleOffset + count > sc->Length ) {
					count = sc->Length - sampleOffset;
				}

				if ( count > 0 ) {
					// Talking animations
					// TODO: check that this entity has talking animations enabled!
					if ( ( GGameType & ( GAME_WolfSP | GAME_ET ) ) && firstPass && ch->entchannel == Q3CHAN_VOICE &&
						 ch->entnum < ( GGameType & GAME_ET ? MAX_CLIENTS_ET : MAX_CLIENTS_WS ) ) {
						int talkofs, talkcnt, talktime;
						// we need to go into the future, since the interpolated behaviour of the facial
						// animation creates lag in the time it takes to display the current facial frame
						talktime = ltime + ( int )( TALK_FUTURE_SEC * ( float )s_khz->integer * 1000 );
						talkofs = talktime - ch->startSample;
						talkcnt = 100;
						if ( talkofs + talkcnt < sc->Length ) {
							S_SetVoiceAmplitudeFrom16( sc, talkofs, talkcnt, ch->entnum );
						}
					}
					S_PaintChannelFrom16( ch, sc, count, sampleOffset, ltime - s_paintedtime );
					ltime += count;
				}

				// if at end of loop, restart
				if ( ltime >= ch->startSample + ch->sfx->Length ) {
					if ( ch->sfx->LoopStart >= 0 ) {
						ch->startSample = ltime - ch->sfx->LoopStart;
					} else {
						// channel just stopped
						if ( !( GGameType & GAME_Tech3 ) ) {
							ch->sfx = NULL;
						}
						break;
					}
				}
			}
		}

		// paint in the looped channels.
		ch = loop_channels;
		for ( i = 0; i < numLoopChannels; i++, ch++ ) {
			if ( !ch->sfx || ( !ch->leftvol && !ch->rightvol ) ) {
				continue;
			}

			ltime = s_paintedtime;
			sc = ch->sfx;

			if ( sc->Data == NULL || sc->Length == 0 ) {
				continue;
			}
			// we might have to make two passes if it
			// is a looping sound effect and the end of
			// the sample is hit
			do {
				sampleOffset = ( ltime % sc->Length );

				count = end - ltime;
				if ( sampleOffset + count > sc->Length ) {
					count = sc->Length - sampleOffset;
				}

				if ( count > 0 ) {
					// Ridah, talking animations
					// TODO: check that this entity has talking animations enabled!
					if ( ( GGameType & ( GAME_WolfSP | GAME_ET ) ) && firstPass && ch->entchannel == Q3CHAN_VOICE &&
						 ch->entnum < ( GGameType & GAME_ET ? MAX_CLIENTS_ET : MAX_CLIENTS_WS ) ) {
						int talkofs, talkcnt, talktime;
						// we need to go into the future, since the interpolated behaviour of the facial
						// animation creates lag in the time it takes to display the current facial frame
						talktime = ltime + ( int )( TALK_FUTURE_SEC * ( float )s_khz->integer * 1000 );
						talkofs = talktime % sc->Length;
						talkcnt = 100;
						if ( talkofs + talkcnt < sc->Length ) {
							S_SetVoiceAmplitudeFrom16( sc, talkofs, talkcnt, ch->entnum );
						}
					}
					S_PaintChannelFrom16( ch, sc, count, sampleOffset, ltime - s_paintedtime );
					ltime += count;
				}
			} while ( ltime < end );
		}

		// transfer out according to DMA format
		S_TransferPaintBuffer( end );
		CL_WriteWaveFilePacket( end );
		s_paintedtime = end;
		firstPass = false;
	}
}
