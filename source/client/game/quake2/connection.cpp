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

#include "local.h"
#include "../../public.h"
#include "../../client_main.h"
#include "../../system.h"
#include "../../ui/ui.h"
#include "../../cinematic/public.h"
#include "../../../server/public.h"
#include "../../../common/file_formats/md2.h"
#include "menu.h"
#include "../../../common/Common.h"
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"
#include "../../../common/message_utils.h"
#include "../../../common/system.h"
#include "../../../common/endian.h"

#define PLAYER_MULT 5

// ENV_CNT is map load, ENV_CNT+1 is first env map
#define ENV_CNT ( Q2CS_PLAYERSKINS + MAX_CLIENTS_Q2 * PLAYER_MULT )
#define TEXTURE_CNT ( ENV_CNT + 13 )

int clq2_precache_check;	// for autodownload of precache items
int clq2_precache_model_skin;
byte* clq2_precache_model;	// used for skin checking in alias models
static int clq2_precache_tex;
int clq2_precache_spawncount;

static const char* env_suf[ 6 ] = {"rt", "bk", "lf", "ft", "up", "dn"};

void CLQ2_RegisterSounds() {
	S_BeginRegistration();
	CLQ2_RegisterTEntSounds();
	for ( int i = 1; i < MAX_SOUNDS_Q2; i++ ) {
		if ( !cl.q2_configstrings[ Q2CS_SOUNDS + i ][ 0 ] ) {
			break;
		}
		cl.sound_precache[ i ] = S_RegisterSound( cl.q2_configstrings[ Q2CS_SOUNDS + i ] );
	}
	S_EndRegistration();
}

void CLQ2_RequestNextDownload() {
	dmd2_t* pheader;

	if ( cls.state != CA_CONNECTED ) {
		return;
	}

	if ( !allow_download->value && clq2_precache_check < ENV_CNT ) {
		clq2_precache_check = ENV_CNT;
	}

	if ( clq2_precache_check == Q2CS_MODELS ) {	// confirm map
		clq2_precache_check = Q2CS_MODELS + 2;	// 0 isn't used
		if ( allow_download_maps->value ) {
			if ( !CL_CheckOrDownloadFile( cl.q2_configstrings[ Q2CS_MODELS + 1 ] ) ) {
				return;	// started a download
			}
		}
	}
	if ( clq2_precache_check >= Q2CS_MODELS && clq2_precache_check < Q2CS_MODELS + MAX_MODELS_Q2 ) {
		if ( allow_download_models->value ) {
			while ( clq2_precache_check < Q2CS_MODELS + MAX_MODELS_Q2 &&
					cl.q2_configstrings[ clq2_precache_check ][ 0 ] ) {
				if ( cl.q2_configstrings[ clq2_precache_check ][ 0 ] == '*' ||
					 cl.q2_configstrings[ clq2_precache_check ][ 0 ] == '#' ) {
					clq2_precache_check++;
					continue;
				}
				if ( clq2_precache_model_skin == 0 ) {
					if ( !CL_CheckOrDownloadFile( cl.q2_configstrings[ clq2_precache_check ] ) ) {
						clq2_precache_model_skin = 1;
						return;	// started a download
					}
					clq2_precache_model_skin = 1;
				}

				// checking for skins in the model
				if ( !clq2_precache_model ) {

					FS_ReadFile( cl.q2_configstrings[ clq2_precache_check ], ( void** )&clq2_precache_model );
					if ( !clq2_precache_model ) {
						clq2_precache_model_skin = 0;
						clq2_precache_check++;
						continue;	// couldn't load it
					}
					if ( LittleLong( *( unsigned* )clq2_precache_model ) != IDMESH2HEADER ) {
						// not an alias model
						FS_FreeFile( clq2_precache_model );
						clq2_precache_model = 0;
						clq2_precache_model_skin = 0;
						clq2_precache_check++;
						continue;
					}
					pheader = ( dmd2_t* )clq2_precache_model;
					if ( LittleLong( pheader->version ) != MESH2_VERSION ) {
						clq2_precache_check++;
						clq2_precache_model_skin = 0;
						continue;	// couldn't load it
					}
				}

				pheader = ( dmd2_t* )clq2_precache_model;

				while ( clq2_precache_model_skin - 1 < LittleLong( pheader->num_skins ) ) {
					if ( !CL_CheckOrDownloadFile( ( char* )clq2_precache_model +
							 LittleLong( pheader->ofs_skins ) +
							 ( clq2_precache_model_skin - 1 ) * MAX_MD2_SKINNAME ) ) {
						clq2_precache_model_skin++;
						return;	// started a download
					}
					clq2_precache_model_skin++;
				}
				if ( clq2_precache_model ) {
					FS_FreeFile( clq2_precache_model );
					clq2_precache_model = 0;
				}
				clq2_precache_model_skin = 0;
				clq2_precache_check++;
			}
		}
		clq2_precache_check = Q2CS_SOUNDS;
	}
	if ( clq2_precache_check >= Q2CS_SOUNDS && clq2_precache_check < Q2CS_SOUNDS + MAX_SOUNDS_Q2 ) {
		if ( allow_download_sounds->value ) {
			if ( clq2_precache_check == Q2CS_SOUNDS ) {
				clq2_precache_check++;	// zero is blank
			}
			while ( clq2_precache_check < Q2CS_SOUNDS + MAX_SOUNDS_Q2 &&
					cl.q2_configstrings[ clq2_precache_check ][ 0 ] ) {
				if ( cl.q2_configstrings[ clq2_precache_check ][ 0 ] == '*' ) {
					clq2_precache_check++;
					continue;
				}
				char fn[ MAX_OSPATH ];
				String::Sprintf( fn, sizeof ( fn ), "sound/%s", cl.q2_configstrings[ clq2_precache_check++ ] );
				if ( !CL_CheckOrDownloadFile( fn ) ) {
					return;	// started a download
				}
			}
		}
		clq2_precache_check = Q2CS_IMAGES;
	}
	if ( clq2_precache_check >= Q2CS_IMAGES && clq2_precache_check < Q2CS_IMAGES + MAX_IMAGES_Q2 ) {
		if ( clq2_precache_check == Q2CS_IMAGES ) {
			clq2_precache_check++;	// zero is blank
		}
		while ( clq2_precache_check < Q2CS_IMAGES + MAX_IMAGES_Q2 &&
				cl.q2_configstrings[ clq2_precache_check ][ 0 ] ) {
			char fn[ MAX_OSPATH ];
			String::Sprintf( fn, sizeof ( fn ), "pics/%s.pcx", cl.q2_configstrings[ clq2_precache_check++ ] );
			if ( !CL_CheckOrDownloadFile( fn ) ) {
				return;	// started a download
			}
		}
		clq2_precache_check = Q2CS_PLAYERSKINS;
	}
	// skins are special, since a player has three things to download:
	// model, weapon model and skin
	// so clq2_precache_check is now *3
	if ( clq2_precache_check >= Q2CS_PLAYERSKINS && clq2_precache_check < Q2CS_PLAYERSKINS + MAX_CLIENTS_Q2 * PLAYER_MULT ) {
		if ( allow_download_players->value ) {
			while ( clq2_precache_check < Q2CS_PLAYERSKINS + MAX_CLIENTS_Q2 * PLAYER_MULT ) {
				int i, n;
				char model[ MAX_QPATH ], skin[ MAX_QPATH ], * p;

				i = ( clq2_precache_check - Q2CS_PLAYERSKINS ) / PLAYER_MULT;
				n = ( clq2_precache_check - Q2CS_PLAYERSKINS ) % PLAYER_MULT;

				if ( !cl.q2_configstrings[ Q2CS_PLAYERSKINS + i ][ 0 ] ) {
					clq2_precache_check = Q2CS_PLAYERSKINS + ( i + 1 ) * PLAYER_MULT;
					continue;
				}

				if ( ( p = strchr( cl.q2_configstrings[ Q2CS_PLAYERSKINS + i ], '\\' ) ) != NULL ) {
					p++;
				} else {
					p = cl.q2_configstrings[ Q2CS_PLAYERSKINS + i ];
				}
				String::Cpy( model, p );
				p = strchr( model, '/' );
				if ( !p ) {
					p = strchr( model, '\\' );
				}
				if ( p ) {
					*p++ = 0;
					String::Cpy( skin, p );
				} else {
					*skin = 0;
				}

				char fn[ MAX_OSPATH ];
				switch ( n ) {
				case 0:	// model
					String::Sprintf( fn, sizeof ( fn ), "players/%s/tris.md2", model );
					if ( !CL_CheckOrDownloadFile( fn ) ) {
						clq2_precache_check = Q2CS_PLAYERSKINS + i * PLAYER_MULT + 1;
						return;	// started a download
					}
					n++;
				/*FALL THROUGH*/

				case 1:	// weapon model
					String::Sprintf( fn, sizeof ( fn ), "players/%s/weapon.md2", model );
					if ( !CL_CheckOrDownloadFile( fn ) ) {
						clq2_precache_check = Q2CS_PLAYERSKINS + i * PLAYER_MULT + 2;
						return;	// started a download
					}
					n++;
				/*FALL THROUGH*/

				case 2:	// weapon skin
					String::Sprintf( fn, sizeof ( fn ), "players/%s/weapon.pcx", model );
					if ( !CL_CheckOrDownloadFile( fn ) ) {
						clq2_precache_check = Q2CS_PLAYERSKINS + i * PLAYER_MULT + 3;
						return;	// started a download
					}
					n++;
				/*FALL THROUGH*/

				case 3:	// skin
					String::Sprintf( fn, sizeof ( fn ), "players/%s/%s.pcx", model, skin );
					if ( !CL_CheckOrDownloadFile( fn ) ) {
						clq2_precache_check = Q2CS_PLAYERSKINS + i * PLAYER_MULT + 4;
						return;	// started a download
					}
					n++;
				/*FALL THROUGH*/

				case 4:	// skin_i
					String::Sprintf( fn, sizeof ( fn ), "players/%s/%s_i.pcx", model, skin );
					if ( !CL_CheckOrDownloadFile( fn ) ) {
						clq2_precache_check = Q2CS_PLAYERSKINS + i * PLAYER_MULT + 5;
						return;	// started a download
					}
					// move on to next model
					clq2_precache_check = Q2CS_PLAYERSKINS + ( i + 1 ) * PLAYER_MULT;
				}
			}
		}
		// precache phase completed
		clq2_precache_check = ENV_CNT;
	}

	if ( clq2_precache_check == ENV_CNT ) {
		clq2_precache_check = ENV_CNT + 1;

		int map_checksum;		// for detecting cheater maps
		CM_LoadMap( cl.q2_configstrings[ Q2CS_MODELS + 1 ], true, &map_checksum );

		if ( map_checksum != String::Atoi( cl.q2_configstrings[ Q2CS_MAPCHECKSUM ] ) ) {
			common->Error( "Local map version differs from server: %i != '%s'\n",
				map_checksum, cl.q2_configstrings[ Q2CS_MAPCHECKSUM ] );
			return;
		}
	}

	if ( clq2_precache_check > ENV_CNT && clq2_precache_check < TEXTURE_CNT ) {
		if ( allow_download->value && allow_download_maps->value ) {
			while ( clq2_precache_check < TEXTURE_CNT ) {
				int n = clq2_precache_check++ - ENV_CNT - 1;

				char fn[ MAX_OSPATH ];
				if ( n & 1 ) {
					String::Sprintf( fn, sizeof ( fn ), "env/%s%s.pcx",
						cl.q2_configstrings[ Q2CS_SKY ], env_suf[ n / 2 ] );
				} else {
					String::Sprintf( fn, sizeof ( fn ), "env/%s%s.tga",
						cl.q2_configstrings[ Q2CS_SKY ], env_suf[ n / 2 ] );
				}
				if ( !CL_CheckOrDownloadFile( fn ) ) {
					return;	// started a download
				}
			}
		}
		clq2_precache_check = TEXTURE_CNT;
	}

	if ( clq2_precache_check == TEXTURE_CNT ) {
		clq2_precache_check = TEXTURE_CNT + 1;
		clq2_precache_tex = 0;
	}

	// confirm existance of textures, download any that don't exist
	if ( clq2_precache_check == TEXTURE_CNT + 1 ) {
		if ( allow_download->value && allow_download_maps->value ) {
			while ( clq2_precache_tex < CM_GetNumTextures() ) {
				char fn[ MAX_OSPATH ];

				sprintf( fn, "textures/%s.wal", CM_GetTextureName( clq2_precache_tex++ ) );
				if ( !CL_CheckOrDownloadFile( fn ) ) {
					return;	// started a download
				}
			}
		}
		clq2_precache_check = TEXTURE_CNT + 999;
	}

	CLQ2_RegisterSounds();
	CLQ2_PrepRefresh();

	CL_AddReliableCommand( va( "begin %i\n", clq2_precache_spawncount ) );
}

void CLQ2_SendCmd() {
	// build a command even if not connected

	// save this command off for prediction
	int i = clc.netchan.outgoingSequence & ( CMD_BACKUP_Q2 - 1 );
	q2usercmd_t* cmd = &cl.q2_cmds[ i ];
	cl.q2_cmd_time[ i ] = cls.realtime;		// for netgraph ping calculation

	in_usercmd_t inCmd = CL_CreateCmd();
	Com_Memset( cmd, 0, sizeof ( *cmd ) );
	cmd->forwardmove = inCmd.forwardmove;
	cmd->sidemove = inCmd.sidemove;
	cmd->upmove = inCmd.upmove;
	cmd->buttons = inCmd.buttons;
	for ( int i = 0; i < 3; i++ ) {
		cmd->angles[ i ] = inCmd.angles[ i ];
	}
	cmd->impulse = inCmd.impulse;
	cmd->msec = inCmd.msec;
	cmd->lightlevel = inCmd.lightlevel;

	if ( cls.state == CA_DISCONNECTED || cls.state == CA_CONNECTING ) {
		return;
	}

	if ( cls.state == CA_CONNECTED ) {
		if ( clc.netchan.message.cursize || com_frameTime - clc.netchan.lastSent > 1000 ) {
			Netchan_Transmit( &clc.netchan, 0, clc.netchan.message._data );
			clc.netchan.lastSent = com_frameTime;
		}
		return;
	}

	// send a userinfo update if needed
	if ( cvar_modifiedFlags & CVAR_USERINFO ) {
		CLQ2_FixUpGender();
		cvar_modifiedFlags &= ~CVAR_USERINFO;
		clc.netchan.message.WriteByte( q2clc_userinfo );
		clc.netchan.message.WriteString2( Cvar_InfoString(
				CVAR_USERINFO, MAX_INFO_STRING_Q2, MAX_INFO_KEY_Q2, MAX_INFO_VALUE_Q2, true, false ) );
	}

	QMsg buf;
	byte data[ 128 ];
	buf.InitOOB( data, sizeof ( data ) );

	if ( cmd->buttons ) {
		// skip the rest of the cinematic
		CIN_SkipCinematic();
	}

	// begin a client move command
	buf.WriteByte( q2clc_move );

	// save the position for a checksum byte
	int checksumIndex = buf.cursize;
	buf.WriteByte( 0 );

	// let the server know what the last frame we
	// got was, so the next message can be delta compressed
	if ( cl_nodelta->value || !cl.q2_frame.valid || cls.q2_demowaiting ) {
		buf.WriteLong( -1 );	// no compression
	} else {
		buf.WriteLong( cl.q2_frame.serverframe );
	}

	// send this and the previous cmds in the message, so
	// if the last packet was dropped, it can be recovered
	i = ( clc.netchan.outgoingSequence - 2 ) & ( CMD_BACKUP_Q2 - 1 );
	cmd = &cl.q2_cmds[ i ];
	q2usercmd_t nullcmd;
	Com_Memset( &nullcmd, 0, sizeof ( nullcmd ) );
	MSGQ2_WriteDeltaUsercmd( &buf, &nullcmd, cmd );
	q2usercmd_t* oldcmd = cmd;

	i = ( clc.netchan.outgoingSequence - 1 ) & ( CMD_BACKUP_Q2 - 1 );
	cmd = &cl.q2_cmds[ i ];
	MSGQ2_WriteDeltaUsercmd( &buf, oldcmd, cmd );
	oldcmd = cmd;

	i = ( clc.netchan.outgoingSequence ) & ( CMD_BACKUP_Q2 - 1 );
	cmd = &cl.q2_cmds[ i ];
	MSGQ2_WriteDeltaUsercmd( &buf, oldcmd, cmd );

	// calculate a checksum over the move commands
	buf._data[ checksumIndex ] = COMQ2_BlockSequenceCRCByte(
		buf._data + checksumIndex + 1, buf.cursize - checksumIndex - 1,
		clc.netchan.outgoingSequence );

	//
	// deliver the message
	//
	Netchan_Transmit( &clc.netchan, buf.cursize, buf._data );
	clc.netchan.lastSent = com_frameTime;
}

void CLQ2_Disconnect() {
	if ( cls.state == CA_DISCONNECTED ) {
		return;
	}

	if ( cl_timedemo && cl_timedemo->value ) {
		int time = Sys_Milliseconds() - cl.q2_timedemo_start;
		if ( time > 0 ) {
			common->Printf( "%i frames, %3.1f seconds: %3.1f fps\n", cl.q2_timedemo_frames,
				time / 1000.0, cl.q2_timedemo_frames * 1000.0 / time );
		}
	}

	UI_ForceMenuOff();

	cls.q2_connect_time = 0;

	SCR_StopCinematic();

	if ( clc.demorecording ) {
		CLQ2_Stop_f();
	}

	// send a disconnect message to the server
	byte final[ 32 ];
	final[ 0 ] = q2clc_stringcmd;
	String::Cpy( ( char* )final + 1, "disconnect" );
	Netchan_Transmit( &clc.netchan, String::Length( ( char* )final ), final );
	Netchan_Transmit( &clc.netchan, String::Length( ( char* )final ), final );
	Netchan_Transmit( &clc.netchan, String::Length( ( char* )final ), final );
	clc.netchan.lastSent = Sys_Milliseconds();

	CL_ClearState();

	// stop download
	if ( clc.download ) {
		FS_FCloseFile( clc.download );
		clc.download = 0;
	}

	cls.state = CA_DISCONNECTED;
}

//	Called after an ERR_DROP was thrown
void CLQ2_Drop() {
	if ( cls.state == CA_UNINITIALIZED ) {
		return;
	}
	if ( cls.state == CA_DISCONNECTED ) {
		return;
	}

	CL_Disconnect( true );

	// drop loading plaque unless this is the initial game start
	if ( cls.q2_disable_servercount != -1 ) {
		SCR_EndLoadingPlaque();		// get rid of loading plaque
	}
}

//	We have gotten a challenge from the server, so try and connect.
static void CLQ2_SendConnectPacket() {
	netadr_t adr;
	if ( !SOCK_StringToAdr( cls.servername, &adr, Q2PORT_SERVER ) ) {
		common->Printf( "Bad server address\n" );
		cls.q2_connect_time = 0;
		return;
	}

	int port = Cvar_VariableValue( "qport" );
	cvar_modifiedFlags &= ~CVAR_USERINFO;

	NET_OutOfBandPrint( NS_CLIENT, adr, "connect %i %i %i \"%s\"\n",
		Q2PROTOCOL_VERSION, port, cls.challenge,
		Cvar_InfoString( CVAR_USERINFO, MAX_INFO_STRING_Q2, MAX_INFO_KEY_Q2,
			MAX_INFO_VALUE_Q2, true, false ) );
}

//	Resend a connect message if the last one has timed out
void CLQ2_CheckForResend() {
	// if the local server is running and we aren't
	// then connect
	if ( cls.state == CA_DISCONNECTED && ComQ2_ServerState() ) {
		cls.state = CA_CONNECTING;
		String::NCpy( cls.servername, "localhost", sizeof ( cls.servername ) - 1 );
		// we don't need a challenge on the localhost
		CLQ2_SendConnectPacket();
		return;
	}

	// resend if we haven't gotten a reply yet
	if ( cls.state != CA_CONNECTING ) {
		return;
	}

	if ( cls.realtime - cls.q2_connect_time < 3000 ) {
		return;
	}

	netadr_t adr;
	if ( !SOCK_StringToAdr( cls.servername, &adr, Q2PORT_SERVER ) ) {
		common->Printf( "Bad server address\n" );
		cls.state = CA_DISCONNECTED;
		return;
	}

	cls.q2_connect_time = cls.realtime;	// for retransmit requests

	common->Printf( "Connecting to %s...\n", cls.servername );

	NET_OutOfBandPrint( NS_CLIENT, adr, "getchallenge\n" );
}

void CLQ2_Connect_f() {
	if ( Cmd_Argc() != 2 ) {
		common->Printf( "usage: connect <server>\n" );
		return;
	}

	if ( ComQ2_ServerState() ) {// if running a local server, kill it and reissue
		SV_Shutdown( "Server quit\n" );
	} else {
		CL_Disconnect( true );
	}

	const char* server = Cmd_Argv( 1 );

	NET_Config( true );			// allow remote

	CL_Disconnect( true );

	cls.state = CA_CONNECTING;
	String::NCpy( cls.servername, server, sizeof ( cls.servername ) - 1 );
	cls.q2_connect_time = -99999;	// CLQ2_CheckForResend() will fire immediately
}

//	The server is changing levels
void CLQ2_Reconnect_f() {
	//ZOID
	//if we are downloading, we don't change!  This so we don't suddenly stop downloading a map
	if ( clc.download ) {
		return;
	}

	S_StopAllSounds();
	if ( cls.state == CA_CONNECTED ) {
		common->Printf( "reconnecting...\n" );
		cls.state = CA_CONNECTED;
		CL_AddReliableCommand( "new" );
		return;
	}

	if ( *cls.servername ) {
		if ( cls.state >= CA_CONNECTED ) {
			CL_Disconnect( true );
			cls.q2_connect_time = cls.realtime - 1500;
		} else {
			cls.q2_connect_time = -99999;	// fire immediately

		}
		cls.state = CA_CONNECTING;
		common->Printf( "reconnecting...\n" );
	}
}

//	Handle a reply from a ping
static void CLQ2_ParseStatusMessage( QMsg& message, netadr_t& from ) {
	const char* s = message.ReadString2();

	common->Printf( "%s\n", s );
	MQ2_AddToServerList( from, s );
}

//	Responses to broadcasts, etc
static void CLQ2_ConnectionlessPacket( QMsg& net_message, netadr_t& net_from ) {
	net_message.BeginReadingOOB();
	net_message.ReadLong();	// skip the -1

	const char* s = net_message.ReadStringLine2();

	Cmd_TokenizeString( s, false );

	const char* c = Cmd_Argv( 0 );

	common->Printf( "%s: %s\n", SOCK_AdrToString( net_from ), c );

	// server connection
	if ( !String::Cmp( c, "client_connect" ) ) {
		if ( cls.state == CA_CONNECTED ) {
			common->Printf( "Dup connect received.  Ignored.\n" );
			return;
		}
		Netchan_Setup( NS_CLIENT, &clc.netchan, net_from, cls.quakePort );
		clc.netchan.lastReceived = Sys_Milliseconds();
		CL_AddReliableCommand( "new" );
		cls.state = CA_CONNECTED;
		return;
	}

	// server responding to a status broadcast
	if ( !String::Cmp( c, "info" ) ) {
		CLQ2_ParseStatusMessage( net_message, net_from );
		return;
	}

	// remote command from gui front end
	if ( !String::Cmp( c, "cmd" ) ) {
		if ( !SOCK_IsLocalAddress( net_from ) ) {
			common->Printf( "Command packet from remote host.  Ignored.\n" );
			return;
		}
		Sys_AppActivate();
		s = net_message.ReadString2();
		Cbuf_AddText( s );
		Cbuf_AddText( "\n" );
		return;
	}
	// print command from somewhere
	if ( !String::Cmp( c, "print" ) ) {
		s = net_message.ReadString2();
		common->Printf( "%s", s );
		return;
	}

	// ping from somewhere
	if ( !String::Cmp( c, "ping" ) ) {
		NET_OutOfBandPrint( NS_CLIENT, net_from, "ack" );
		return;
	}

	// challenge from the server we are connecting to
	if ( !String::Cmp( c, "challenge" ) ) {
		cls.challenge = String::Atoi( Cmd_Argv( 1 ) );
		CLQ2_SendConnectPacket();
		return;
	}

	// echo request from server
	if ( !String::Cmp( c, "echo" ) ) {
		NET_OutOfBandPrint( NS_CLIENT, net_from, "%s", Cmd_Argv( 1 ) );
		return;
	}

	common->Printf( "Unknown command.\n" );
}

void CLQ2_ReadPackets() {
	netadr_t net_from;
	QMsg net_message;
	byte net_message_buffer[ MAX_MSGLEN_Q2 ];
	net_message.InitOOB( net_message_buffer, sizeof ( net_message_buffer ) );

	while ( NET_GetPacket( NS_CLIENT, &net_from, &net_message ) ) {
		//
		// remote command packet
		//
		if ( *( int* )net_message._data == -1 ) {
			CLQ2_ConnectionlessPacket( net_message, net_from );
			continue;
		}

		if ( cls.state == CA_DISCONNECTED || cls.state == CA_CONNECTING ) {
			continue;		// dump it if not connected

		}
		if ( net_message.cursize < 8 ) {
			common->Printf( "%s: Runt packet\n",SOCK_AdrToString( net_from ) );
			continue;
		}

		//
		// packet from server
		//
		if ( !SOCK_CompareAdr( net_from, clc.netchan.remoteAddress ) ) {
			common->DPrintf( "%s:sequenced packet without connection\n",
				SOCK_AdrToString( net_from ) );
			continue;
		}
		if ( !Netchan_Process( &clc.netchan, &net_message ) ) {
			continue;		// wasn't accepted for some reason
		}
		clc.netchan.lastReceived = Sys_Milliseconds();
		CLQ2_ParseServerMessage( net_message );

		CLQ2_AddNetgraph();

		//
		// we don't know if it is ok to save a demo message until
		// after we have parsed the frame
		//
		if ( clc.demorecording && !cls.q2_demowaiting ) {
			CLQ2_WriteDemoMessage( &net_message, 8 );
		}
	}

	//
	// check timeout
	//
	if ( cls.state >= CA_CONNECTED &&
		 cls.realtime - clc.netchan.lastReceived > cl_timeout->value * 1000 ) {
		if ( ++cl.timeoutcount > 5 ) {	// timeoutcount saves debugger
			common->Printf( "\nServer connection timed out.\n" );
			CL_Disconnect( true );
			return;
		}
	} else {
		cl.timeoutcount = 0;
	}
}
