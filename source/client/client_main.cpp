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

#include "client_main.h"
#include "public.h"
#include "game/particles.h"
#include "game/dynamic_lights.h"
#include "game/light_styles.h"
#include "game/quake/local.h"
#include "game/hexen2/local.h"
#include "game/quake_hexen2/connection.h"
#include "game/quake_hexen2/demo.h"
#include "game/quake_hexen2/main.h"
#include "game/quake_hexen2/network_channel.h"
#include "game/quake_hexen2/predict.h"
#include "game/quake2/local.h"
#include "game/tech3/local.h"
#include "game/wolfsp/local.h"
#include "game/et/dl_public.h"
#include "renderer/driver.h"
#include "ui/ui.h"
#include "ui/console.h"
#include "cinematic/public.h"
#include "../server/public.h"
#include "../common/Common.h"
#include "../common/common_defs.h"
#include "../common/strings.h"
#include "../common/player_move.h"
#include "../common/system.h"

Cvar* cl_inGameVideo;

Cvar* cl_timedemo;

Cvar* cl_timeout;

Cvar* clqh_nolerp;

// these two are not intended to be set directly
Cvar* clqh_name;
Cvar* clqh_color;

static Cvar* rcon_client_password;
static Cvar* rconAddress;

clientActive_t cl;
clientConnection_t clc;
clientStatic_t cls;

idSkinTranslation clq1_translation_info;
idSkinTranslation clh2_translation_info[ MAX_PLAYER_CLASS ];
byte* playerTranslation;

int color_offsets[ MAX_PLAYER_CLASS ] =
{
	2 * 14 * 256,
	0,
	1 * 14 * 256,
	2 * 14 * 256,
	2 * 14 * 256,
	2 * 14 * 256
};

int bitcounts[ 32 ];	/// just for protocol profiling

static void CL_ForwardToServer_f() {
	if ( cls.state < CA_CONNECTED || clc.demoplaying ) {
		common->Printf( "Not connected to a server.\n" );
		return;
	}

	if ( GGameType & GAME_QuakeWorld && String::ICmp( Cmd_Argv( 1 ), "snap" ) == 0 ) {
		Cbuf_InsertText( "snap\n" );
		return;
	}

	// don't forward the first argument
	if ( Cmd_Argc() > 1 ) {
		CL_AddReliableCommand( GGameType & GAME_Tech3 ? Cmd_Args() : Cmd_ArgsUnmodified() );
	}
}

void CL_Disconnect_f() {
	if ( GGameType & GAME_Tech3 ) {
		SCR_StopCinematic();
	}
	if ( GGameType & GAME_Quake3 ) {
		Cvar_Set( "ui_singlePlayerActive", "0" );
	}
	if ( GGameType & ( GAME_WolfSP | GAME_ET ) ) {
		// RF, make sure loading variables are turned off
		Cvar_Set( "savegame_loading", "0" );
		Cvar_Set( "g_reloading", "0" );
	}
	if ( !( GGameType & GAME_Tech3 ) || ( cls.state != CA_DISCONNECTED && cls.state != CA_CINEMATIC ) ) {
		common->Disconnect( "Disconnected from server" );
	}
}

//	Send the rest of the command line over as an unconnected command.
static void CL_Rcon_f() {
	if ( !rcon_client_password->string ) {
		common->Printf( "You must set '%s' before\n"
						"issuing an rcon command.\n", rcon_client_password->name );
		return;
	}

	char message[ 1024 ];
	message[ 0 ] = -1;
	message[ 1 ] = -1;
	message[ 2 ] = -1;
	message[ 3 ] = -1;
	message[ 4 ] = 0;

	if ( GGameType & GAME_Quake2 ) {
		NET_Config( true );			// allow remote
	}

	String::Cat( message, sizeof ( message ), "rcon " );

	String::Cat( message, sizeof ( message ), rcon_client_password->string );
	String::Cat( message, sizeof ( message ), " " );

	String::Cat( message, sizeof ( message ), Cmd_Cmd() + 5 );

	netadr_t to;
	if ( cls.state >= CA_CONNECTED ) {
		to = clc.netchan.remoteAddress;
	} else {
		if ( !String::Length( rconAddress->string ) ) {
			common->Printf( "You must either be connected,\n"
							"or set the '%s' cvar\n"
							"to issue rcon commands\n", rconAddress->name );
			return;
		}

		SOCK_StringToAdr( rconAddress->string, &to, GGameType & GAME_QuakeWorld ? QWPORT_SERVER :
			GGameType & GAME_HexenWorld ? HWPORT_SERVER :
			GGameType & GAME_Quake2 ? Q2PORT_SERVER : Q3PORT_SERVER );
	}

	NET_SendPacket( NS_CLIENT, String::Length( message ) + 1, message, to );
}

void CL_Init() {
	common->Printf( "----- Client Initialization -----\n" );

	Con_Init();

	UI_Init();

	CL_InitInput();

	SCR_Init();

	cls.state = CA_DISCONNECTED;	// no longer CA_UNINITIALIZED

	cl_inGameVideo = Cvar_Get( "r_inGameVideo", "1", CVAR_ARCHIVE );
	cl_shownet = Cvar_Get( "cl_shownet", "0", CVAR_TEMP );
	if ( !( GGameType & GAME_QuakeHexen ) ) {
		cl_timedemo = Cvar_Get( "timedemo", "0", CVAR_CHEAT );
	}

	//
	// register our commands
	//
	Cmd_AddCommand( "cmd", CL_ForwardToServer_f );
	Cmd_AddCommand( "disconnect", CL_Disconnect_f );

	if ( !( GGameType & GAME_QuakeHexen ) || GGameType & ( GAME_QuakeWorld | GAME_HexenWorld ) ) {
		if ( !( GGameType & GAME_Tech3 ) ) {
			rcon_client_password = Cvar_Get( "rcon_password", "", 0 );
			rconAddress = Cvar_Get( "rcon_address", "", 0 );
		} else {
			rcon_client_password = Cvar_Get( "rconPassword", "", CVAR_TEMP );
			rconAddress = Cvar_Get( "rconAddress", "", 0 );
		}
		Cmd_AddCommand( "rcon", CL_Rcon_f );
	}

	if ( GGameType & GAME_QuakeHexen ) {
		CLQH_Init();
	} else if ( GGameType & GAME_Quake2 ) {
		CLQ2_Init();
	} else {
		CLT3_Init();
	}

	IN_Init();

	common->Printf( "----- Client Initialization Complete -----\n" );
}

void CL_StartHunkUsers() {
	if ( GGameType & GAME_Tech3 ) {
		CLT3_StartHunkUsers();
	} else {
		if ( cls.state == CA_UNINITIALIZED ) {
			return;
		}
		V_Init();
		CL_InitRenderer();
		S_Init();
		CDAudio_Init();
	}
}

int CL_ScaledMilliseconds() {
	return Sys_Milliseconds() * com_timescale->value;
}

void CLQ1_InitPlayerTranslation()
{
	clq1_translation_info.topStartIndex = 16;
	clq1_translation_info.topEndIndex = 31;
	clq1_translation_info.bottomStartIndex = 96;
	clq1_translation_info.bottomEndIndex = 111;
	for ( int i = 0; i < 14; i++ ) {
		//	The artists made some backwards ranges. sigh.
		int sourceIndex;
		if ( i < 8 ) {
			sourceIndex = i * 16 + 15;
		} else {
			sourceIndex = i * 16;
		}
		clq1_translation_info.translatedTopColours[ i ][ 0 ] = r_palette[ sourceIndex ][ 0 ];
		clq1_translation_info.translatedTopColours[ i ][ 1 ] = r_palette[ sourceIndex ][ 1 ];
		clq1_translation_info.translatedTopColours[ i ][ 2 ] = r_palette[ sourceIndex ][ 2 ];
		clq1_translation_info.translatedTopColours[ i ][ 3 ] = r_palette[ sourceIndex ][ 3 ];
		clq1_translation_info.translatedBottomColours[ i ][ 0 ] = r_palette[ sourceIndex ][ 0 ];
		clq1_translation_info.translatedBottomColours[ i ][ 1 ] = r_palette[ sourceIndex ][ 1 ];
		clq1_translation_info.translatedBottomColours[ i ][ 2 ] = r_palette[ sourceIndex ][ 2 ];
		clq1_translation_info.translatedBottomColours[ i ][ 3 ] = r_palette[ sourceIndex ][ 3 ];
	}
}

void CLH2_InitPlayerTranslation() {
	FS_ReadFile( "gfx/player.lmp", ( void** )&playerTranslation );
	if ( !playerTranslation ) {
		common->FatalError( "Couldn't load gfx/player.lmp" );
	}
	for ( int playerClass = 0; playerClass < MAX_PLAYER_CLASS; playerClass++ ) {
		idSkinTranslation& translation = clh2_translation_info[ playerClass ];
		translation.topStartIndex = -1;
		translation.bottomStartIndex = -1;
		translation.topEndIndex = -1;
		translation.bottomEndIndex = -1;
		byte* colorA = playerTranslation + 256 + color_offsets[ playerClass ];
		byte* colorB = colorA + 256;
		for ( int i = 0; i < 256; i++ ) {
			if ( colorA[ i ] != 255 ) {
				if ( translation.topStartIndex == -1 ) {
					translation.topStartIndex = i;
				}
				translation.topEndIndex = i;
			}
			if ( colorB[ i ] != 255 ) {
				if ( translation.bottomStartIndex == -1 ) {
					translation.bottomStartIndex = i;
				}
				translation.bottomEndIndex = i;
			}
		}

		//	Translation 0 is original.
		translation.translatedTopColours[ 0 ][ 0 ] = r_palette[ translation.topEndIndex ][ 0 ];
		translation.translatedTopColours[ 0 ][ 1 ] = r_palette[ translation.topEndIndex ][ 1 ];
		translation.translatedTopColours[ 0 ][ 2 ] = r_palette[ translation.topEndIndex ][ 2 ];
		translation.translatedTopColours[ 0 ][ 3 ] = r_palette[ translation.topEndIndex ][ 3 ];
		translation.translatedBottomColours[ 0 ][ 0 ] = r_palette[ translation.bottomEndIndex ][ 0 ];
		translation.translatedBottomColours[ 0 ][ 1 ] = r_palette[ translation.bottomEndIndex ][ 1 ];
		translation.translatedBottomColours[ 0 ][ 2 ] = r_palette[ translation.bottomEndIndex ][ 2 ];
		translation.translatedBottomColours[ 0 ][ 3 ] = r_palette[ translation.bottomEndIndex ][ 3 ];

		for ( int i = 1; i < 11; i++ ) {
			byte* sourceA = colorB + i * 256;
			int sourceIndex = sourceA[ translation.topEndIndex ];
			translation.translatedTopColours[ i ][ 0 ] = r_palette[ sourceIndex ][ 0 ];
			translation.translatedTopColours[ i ][ 1 ] = r_palette[ sourceIndex ][ 1 ];
			translation.translatedTopColours[ i ][ 2 ] = r_palette[ sourceIndex ][ 2 ];
			translation.translatedTopColours[ i ][ 3 ] = r_palette[ sourceIndex ][ 3 ];
			sourceIndex = sourceA[ translation.bottomEndIndex ];
			translation.translatedBottomColours[ i ][ 0 ] = r_palette[ sourceIndex ][ 0 ];
			translation.translatedBottomColours[ i ][ 1 ] = r_palette[ sourceIndex ][ 1 ];
			translation.translatedBottomColours[ i ][ 2 ] = r_palette[ sourceIndex ][ 2 ];
			translation.translatedBottomColours[ i ][ 3 ] = r_palette[ sourceIndex ][ 3 ];
		}
	}
}

//	Determines the fraction between the last two messages that the objects
// should be put at.
float CLQH_LerpPoint() {
	float f = cl.qh_mtime[ 0 ] - cl.qh_mtime[ 1 ];

	if ( !f || clqh_nolerp->value || cls.qh_timedemo || SV_IsServerActive() ) {
		cl.qh_serverTimeFloat = cl.qh_mtime[ 0 ];
		cl.serverTime = ( int )( cl.qh_serverTimeFloat * 1000 );
		return 1;
	}

	if ( f > 0.1 ) {
		// dropped packet, or start of demo
		cl.qh_mtime[ 1 ] = cl.qh_mtime[ 0 ] - 0.1;
		f = 0.1;
	}
	float frac = ( cl.qh_serverTimeFloat - cl.qh_mtime[ 1 ] ) / f;
	if ( frac < 0 ) {
		if ( frac < -0.01 ) {
			cl.qh_serverTimeFloat = cl.qh_mtime[ 1 ];
			cl.serverTime = ( int )( cl.qh_serverTimeFloat * 1000 );
		}
		frac = 0;
	} else if ( frac > 1 ) {
		if ( frac > 1.01 ) {
			cl.qh_serverTimeFloat = cl.qh_mtime[ 0 ];
			cl.serverTime = ( int )( cl.qh_serverTimeFloat * 1000 );
		}
		frac = 1;
	}
	return frac;
}

void CL_ClearDrift() {
	cl.qh_nodrift = false;
	cl.qh_driftmove = 0;
}

void CLQH_StopDemoLoop() {
	cls.qh_demonum = -1;
}

void CL_ClearKeyCatchers() {
	in_keyCatchers = 0;
}

void CLQH_GetSpawnParams() {
	String::Cpy( cls.qh_spawnparms, "" );

	for ( int i = 2; i < Cmd_Argc(); i++ ) {
		String::Cat( cls.qh_spawnparms, sizeof ( cls.qh_spawnparms ), Cmd_Argv( i ) );
		String::Cat( cls.qh_spawnparms, sizeof ( cls.qh_spawnparms ), " " );
	}
}

bool CL_IsDemoPlaying() {
	return clc.demoplaying;
}

int CLQH_GetIntermission() {
	return cl.qh_intermission;
}

//	The given command will be transmitted to the server, and is gauranteed to
// not have future q3usercmd_t executed before it is executed
void CL_AddReliableCommand( const char* cmd ) {
	if ( GGameType & GAME_Tech3 ) {
		// if we would be losing an old command that hasn't been acknowledged,
		// we must drop the connection
		int maxReliableCommands = GGameType & GAME_Quake3 ? MAX_RELIABLE_COMMANDS_Q3 : MAX_RELIABLE_COMMANDS_WOLF;
		if ( clc.q3_reliableSequence - clc.q3_reliableAcknowledge > maxReliableCommands ) {
			common->Error( "Client command overflow" );
		}
		clc.q3_reliableSequence++;
		int index = clc.q3_reliableSequence & ( maxReliableCommands - 1 );
		String::NCpyZ( clc.q3_reliableCommands[ index ], cmd, sizeof ( clc.q3_reliableCommands[ index ] ) );
	} else {
		clc.netchan.message.WriteByte( GGameType & GAME_Quake ? q1clc_stringcmd :
			GGameType & GAME_Hexen2 ? h2clc_stringcmd : q2clc_stringcmd );
		clc.netchan.message.WriteString2( cmd );
	}
}

//	Adds the current command line as a clc_stringcmd to the client message.
// things like godmode, noclip, etc, are commands directed to the server,
// so when they are typed in at the console, they will need to be forwarded.
void CL_ForwardKnownCommandToServer() {
	if ( cls.state < CA_CONNECTED || clc.demoplaying ) {
		common->Printf( "Not connected to a server.\n" );
		return;
	}

	CL_AddReliableCommand( Cmd_Cmd() );
}

//	Adds the current command line as a clientCommand
// things like godmode, noclip, etc, are commands directed to the server,
// so when they are typed in at the console, they will need to be forwarded.
void CL_ForwardCommandToServer() {
	char* cmd = Cmd_Argv( 0 );

	// ignore key up commands
	if ( cmd[ 0 ] == '-' ) {
		return;
	}

	if ( clc.demoplaying || cls.state < CA_CONNECTED || cmd[ 0 ] == '+' ) {
		common->Printf( "Unknown command \"%s\"\n", cmd );
		return;
	}

	if ( Cmd_Argc() > 1 ) {
		CL_AddReliableCommand( Cmd_Cmd() );
	} else {
		CL_AddReliableCommand( cmd );
	}
}

void CL_CvarChanged( Cvar* var ) {
	if ( !( GGameType & ( GAME_QuakeWorld | GAME_HexenWorld ) ) ) {
		return;
	}

	if ( var->flags & CVAR_USERINFO && var->name[ 0 ] != '*' ) {
		Info_SetValueForKey( cls.qh_userinfo, var->name, var->string, MAX_INFO_STRING_QW, 64, 64,
			String::ICmp( var->name, "name" ) != 0, String::ICmp( var->name, "team" ) == 0 );
		if ( cls.state == CA_CONNECTED || cls.state == CA_LOADING || cls.state == CA_ACTIVE ) {
			CL_AddReliableCommand( va( "setinfo \"%s\" \"%s\"\n", var->name, var->string ) );
		}
	}
}

//	Called before parsing a gamestate
void CL_ClearState() {
	// wipe the entire cl structure
	Com_Memset( &cl, 0, sizeof ( cl ) );

	if ( GGameType & ( GAME_QuakeWorld | GAME_HexenWorld | GAME_Quake2 | GAME_WolfSP ) ) {
		S_StopAllSounds();
	}

	if ( !( GGameType & GAME_Tech3 ) ) {
		clc.netchan.message.Clear();

		CL_ClearParticles();
		CL_ClearDlights();
		CL_ClearLightStyles();
	}

	if ( GGameType & GAME_Quake ) {
		CLQ1_ClearState();
	} else if ( GGameType & GAME_Hexen2 ) {
		CLH2_ClearState();
	} else if ( GGameType & GAME_Quake2 ) {
		CLQ2_ClearState();
	}
}

//	Returns true if the file exists, otherwise it attempts
// to start a download from the server.
bool CL_CheckOrDownloadFile( const char* filename ) {
	if ( strstr( filename, ".." ) ) {
		common->Printf( "Refusing to download a path with ..\n" );
		return true;
	}

	if ( FS_ReadFile( filename, NULL ) != -1 ) {
		// it exists, no need to download
		return true;
	}

	//ZOID - can't download when recording
	if ( clc.demorecording ) {
		common->Printf( "Unable to download %s in record mode.\n", clc.downloadName );
		return true;
	}
	//ZOID - can't download when playback
	if ( clc.demoplaying ) {
		return true;
	}

	String::Cpy( clc.downloadName, filename );

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	String::StripExtension( clc.downloadName, clc.downloadTempName );
	String::Cat( clc.downloadTempName, sizeof ( clc.downloadTempName ), ".tmp" );

	common->Printf( "Downloading %s\n", clc.downloadName );
	CL_AddReliableCommand( va( "download %s", clc.downloadName ) );

	clc.downloadNumber++;

	return false;
}

void SHOWNET( QMsg& msg, const char* s ) {
	if ( cl_shownet->integer >= 2 ) {
		common->Printf( "%3i:%s\n", msg.readcount - 1, s );
	}
}

//	Called when a connection, demo, or cinematic is being terminated.
//	Goes from a connected state to either a menu state or a console state
//	Sends a disconnect message to the server
//	This is also called on Com_Error and Com_Quit_f, so it shouldn't cause any errors
void CL_Disconnect( bool showMainMenu ) {
	if ( GGameType & ( GAME_QuakeWorld | GAME_HexenWorld ) ) {
		CLQHW_Disconnect();
	} else if ( GGameType & GAME_QuakeHexen ) {
		CLQH_Disconnect();
	} else if ( GGameType & GAME_Quake2 ) {
		CLQ2_Disconnect();
	} else {
		CLT3_Disconnect( showMainMenu );
	}
}

//	Called when a demo or cinematic finishes
void CL_NextDemo() {
	if ( GGameType & GAME_QuakeHexen ) {
		CLQH_NextDemo();
	} else if ( GGameType & GAME_Tech3 ) {
		CLT3_NextDemo();
	}
}

void CL_Shutdown() {
	static bool recursive = false;

	common->Printf( "----- CL_Shutdown -----\n" );

	if ( recursive ) {
		printf( "recursive shutdown\n" );
		return;
	}
	recursive = true;

	if ( GGameType & GAME_ET && clc.wm_waverecording ) {		// fretn - write wav header when we quit
		CLET_WavStopRecord_f();
	}

	CL_Disconnect( true );

	if ( !( GGameType & GAME_Tech3 ) ) {
		Com_WriteConfiguration();
	}

	if ( GGameType & GAME_QuakeHexen && !( GGameType & ( GAME_QuakeWorld | GAME_HexenWorld ) ) ) {
		// keep common->Printf from trying to update the screen
		cls.disable_screen = true;

		CLQH_ShutdownNetwork();
	}
	if ( GGameType & GAME_Hexen2 ) {
		MIDI_Cleanup();
	}
	CDAudio_Shutdown();
	S_Shutdown();
	DL_Shutdown();
	R_Shutdown( true );
	IN_Shutdown();

	if ( GGameType & GAME_Tech3 ) {
		CLT3_ShutdownCGame();
		CLT3_ShutdownUI();

		Cmd_RemoveCommand( "cmd" );
		Cmd_RemoveCommand( "configstrings" );
		Cmd_RemoveCommand( "userinfo" );
		Cmd_RemoveCommand( "snd_restart" );
		Cmd_RemoveCommand( "vid_restart" );
		Cmd_RemoveCommand( "disconnect" );
		Cmd_RemoveCommand( "record" );
		Cmd_RemoveCommand( "demo" );
		Cmd_RemoveCommand( "cinematic" );
		Cmd_RemoveCommand( "stoprecord" );
		Cmd_RemoveCommand( "connect" );
		Cmd_RemoveCommand( "localservers" );
		Cmd_RemoveCommand( "globalservers" );
		Cmd_RemoveCommand( "rcon" );
		Cmd_RemoveCommand( "ping" );
		Cmd_RemoveCommand( "serverstatus" );
		Cmd_RemoveCommand( "showip" );
		Cmd_RemoveCommand( "model" );
		Cmd_RemoveCommand( "cache_startgather" );
		Cmd_RemoveCommand( "cache_usedfile" );
		Cmd_RemoveCommand( "cache_setindex" );
		Cmd_RemoveCommand( "cache_mapchange" );
		Cmd_RemoveCommand( "cache_endgather" );
		Cmd_RemoveCommand( "wav_record" );
		Cmd_RemoveCommand( "wav_stoprecord" );

		Cvar_Set( "cl_running", "0" );
	}

	recursive = false;

	Com_Memset( &cls, 0, sizeof ( cls ) );

	common->Printf( "-----------------------\n" );
}

void CL_Frame( int msec ) {
	if ( GGameType & GAME_Quake2 ) {
		static int extratime;

		if ( com_dedicated->value ) {
			return;
		}

		extratime += msec;

		if ( !cl_timedemo->value ) {
			if ( cls.state == CA_CONNECTED && extratime < 100 ) {
				return;			// don't flood packets out while connecting
			}
			if ( extratime < 1000 / clq2_maxfps->value ) {
				return;			// framerate is too high
			}
		}
		msec = extratime;
		extratime = 0;

		cl.serverTime += msec;
	}

	if ( GGameType & GAME_Tech3 ) {
		if ( !com_cl_running->integer ) {
			return;
		}

		if ( GGameType & GAME_WolfSP && cls.ws_endgamemenu ) {
			cls.ws_endgamemenu = false;
			UIWS_SetEndGameMenu();
		} else if ( cls.state == CA_DISCONNECTED && !( in_keyCatchers & KEYCATCH_UI ) &&
					!com_sv_running->integer ) {
			// if disconnected, bring up the menu
			S_StopAllSounds();
			UI_SetMainMenu();
		}

		// if recording an avi, lock to a fixed fps
		if ( clt3_avidemo->integer && msec ) {
			// save the current screen
			if ( cls.state == CA_ACTIVE || clt3_forceavidemo->integer ) {
				Cbuf_ExecuteText( EXEC_NOW, "screenshot silent\n" );
			}
			// fixed time for next frame
			msec = ( 1000 / clt3_avidemo->integer ) * com_timescale->value;
			if ( msec == 0 ) {
				msec = 1;
			}
		}
	}

	// save the msec before checking pause
	cls.realFrametime = msec;

	// decide the simulation time
	cls.frametime = msec;

	cls.realtime += cls.frametime;

	if ( GGameType & GAME_Quake2 && cls.frametime > 200 ) {
		cls.frametime = 200;
	}

	if ( GGameType & GAME_Tech3 && cl_timegraph->integer ) {
		SCR_DebugGraph( cls.realFrametime * 0.25, 0 );
	}

	// if in the debugger last frame, don't timeout
	if ( GGameType & GAME_Quake2 && msec > 5000 ) {
		clc.netchan.lastReceived = Sys_Milliseconds();
	}

	if ( GGameType & GAME_HexenWorld && !( GGameType & ( GAME_QuakeWorld | GAME_HexenWorld ) ) ) {
		NET_Poll();
	}

	// fetch results from server
	if ( GGameType & ( GAME_QuakeWorld | GAME_HexenWorld ) ) {
		CLQHW_ReadPackets();
	} else if ( GGameType & GAME_QuakeHexen ) {
		if ( cls.state == CA_ACTIVE ) {
			CLQH_ReadFromServer();
		}
	} else if ( GGameType & GAME_Quake2 ) {
		CLQ2_ReadPackets();
	}

	if ( GGameType & GAME_Quake2 ) {
		// fix any cheating cvars
		CLQ2_FixCvarCheats();
	}

	if ( GGameType & GAME_Tech3 ) {
		// see if we need to update any userinfo
		CLT3_CheckUserinfo();

		// if we haven't gotten a packet in a long time,
		// drop the connection
		CLT3_CheckTimeout();
	}

	// wwwdl download may survive a server disconnect
	if ( GGameType & GAME_ET && ( ( cls.state == CA_CONNECTED && clc.et_bWWWDl ) || cls.et_bWWWDlDisconnected ) ) {
		CLET_WWWDownload();
	}

	// send intentions now
	if ( GGameType & GAME_QuakeWorld ) {
		CLQW_SendCmd();
	} else if ( GGameType & GAME_HexenWorld ) {
		CLHW_SendCmd();
	} else if ( GGameType & GAME_QuakeHexen ) {
		CLQH_SendCmd();
	} else if ( GGameType & GAME_Quake2 ) {
		CLQ2_SendCmd();
	} else {
		CLT3_SendCmd();
	}

	// resend a connection request if necessary
	if ( GGameType & ( GAME_QuakeWorld | GAME_HexenWorld ) ) {
		CLQHW_CheckForResend();
	} else if ( GGameType & GAME_Quake2 ) {
		CLQ2_CheckForResend();
	} else if ( GGameType & GAME_Tech3 ) {
		CLT3_CheckForResend();
	}

	if ( GGameType & GAME_QuakeWorld ) {
		// Set up prediction for other players
		CLQW_SetUpPlayerPrediction( false );

		// do client side motion prediction
		CLQW_PredictMove();

		// Set up prediction for other players
		CLQW_SetUpPlayerPrediction( true );
	} else if ( GGameType & GAME_HexenWorld ) {
		// Set up prediction for other players
		CLHW_SetUpPlayerPrediction( false );

		// do client side motion prediction
		CLHW_PredictMove();

		// Set up prediction for other players
		CLHW_SetUpPlayerPrediction( true );
	} else if ( GGameType & GAME_Quake2 ) {
		// predict all unacknowledged movements
		CLQ2_PredictMovement();

		// allow rendering DLL change
		CLQ2_CheckVidChanges();
		if ( !cl.q2_refresh_prepped && cls.state == CA_ACTIVE ) {
			CLQ2_PrepRefresh();
		}
	} else if ( GGameType & GAME_Tech3 ) {
		// decide on the serverTime to render
		CLT3_SetCGameTime();
	}

	if ( com_speeds->value ) {
		time_before_ref = Sys_Milliseconds();
	}

	// update the screen
	SCR_UpdateScreen();

	if ( com_speeds->value ) {
		time_after_ref = Sys_Milliseconds();
	}

	// update audio
	if ( cls.state == CA_ACTIVE && !( GGameType & GAME_Tech3 ) ) {
		if ( GGameType & GAME_Quake2 ) {
			CLQ2_UpdateSounds();
		}

		S_Respatialize( cl.viewentity, cl.refdef.vieworg, cl.refdef.viewaxis, 0 );
	}

	S_Update();

	if ( GGameType & GAME_QuakeHexen ) {
		//JL why not Quake 2?
		CDAudio_Update();
	}

	// advance local effects for next frame
	if ( !( GGameType & GAME_Tech3 ) ) {
		CL_RunDLights();
		CL_RunLightStyles();
	}
	if ( cls.state == CA_ACTIVE ) {
		if ( GGameType & GAME_QuakeWorld ) {
			CL_UpdateParticles( 800 );
		} else if ( GGameType & GAME_HexenWorld ) {
			CL_UpdateParticles( movevars.gravity );
		} else if ( GGameType & GAME_QuakeHexen && clc.qh_signon == SIGNONS ) {
			if ( cl.qh_serverTimeFloat != cl.qh_oldtime ) {
				CL_UpdateParticles( Cvar_VariableValue( "sv_gravity" ) );
			}
		}
	}
	SCR_RunCinematic();

	Con_RunConsole();

	cls.framecount++;
}

//	FS code calls this when doing FS_ComparePaks
// we can detect files that we got from a www dl redirect with a wrong checksum
// this indicates that the redirect setup is broken, and next dl attempt should NOT redirect
bool CL_WWWBadChecksum( const char* pakname ) {
	if ( GGameType & GAME_ET && strstr( clc.et_redirectedList, va( "@%s", pakname ) ) ) {
		common->Printf( "WARNING: file %s obtained through download redirect has wrong checksum\n", pakname );
		common->Printf( "         this likely means the server configuration is broken\n" );
		if ( String::Length( clc.et_badChecksumList ) + String::Length( pakname ) + 1 >= ( int )sizeof ( clc.et_badChecksumList ) ) {
			common->Printf( "ERROR: badChecksumList overflowed (%s)\n", clc.et_badChecksumList );
			return false;
		}
		String::Cat( clc.et_badChecksumList, sizeof ( clc.et_badChecksumList ), "@" );
		String::Cat( clc.et_badChecksumList, sizeof ( clc.et_badChecksumList ), pakname );
		common->DPrintf( "bad checksums: %s\n", clc.et_badChecksumList );
		return true;
	}
	return false;
}

void CL_ShutdownOnSignal() {
	if ( GGameType & GAME_QuakeHexen ) {
		ComQH_HostShutdown();
	} else if ( GGameType & GAME_Quake2 ) {
		CL_Shutdown();
	} else {
		GLimp_Shutdown();	// bk010104 - shouldn't this be CL_Shutdown
	}
}

void CL_ShutdownOnWindowsError() {
	if ( GGameType & GAME_QuakeHexen ) {
		ComQH_HostShutdown();
	} else if ( GGameType & GAME_Quake2 ) {
		CL_Shutdown();
	} else {
		IN_Shutdown();
	}
}

bool CL_IsTimeDemo() {
	if ( GGameType & GAME_QuakeHexen ) {
		return cls.qh_timedemo;
	}
	if ( GGameType & GAME_Tech3 ) {
		return comt3_timedemo->integer;
	}
	return false;
}
