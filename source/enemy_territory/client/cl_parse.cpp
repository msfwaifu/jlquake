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

// cl_parse.c  -- parse a message received from the server

#include "client.h"

char *svc_strings[256] = {
	"svc_bad",

	"svc_nop",
	"svc_gamestate",
	"svc_configstring",
	"svc_baseline",
	"svc_serverCommand",
	"svc_download",
	"svc_snapshot",
	"svc_EOF"
};

void SHOWNET( QMsg *msg, char *s ) {
	if ( cl_shownet->integer >= 2 ) {
		Com_Printf( "%3i:%s\n", msg->readcount - 1, s );
	}
}


/*
=========================================================================

MESSAGE PARSING

=========================================================================
*/
#if 1

int entLastVisible[MAX_CLIENTS];

qboolean isEntVisible( entityState_t *ent ) {
	q3trace_t tr;
	vec3_t start, end, temp;
	vec3_t forward, up, right, right2;
	float view_height;

	VectorCopy( cl.cgameClientLerpOrigin, start );
	start[2] += ( cl.snap.ps.viewheight - 1 );
	if ( cl.snap.ps.leanf != 0 ) {
		vec3_t lright, v3ViewAngles;
		VectorCopy( cl.snap.ps.viewangles, v3ViewAngles );
		v3ViewAngles[2] += cl.snap.ps.leanf / 2.0f;
		AngleVectors( v3ViewAngles, NULL, lright, NULL );
		VectorMA( start, cl.snap.ps.leanf, lright, start );
	}

	VectorCopy( ent->pos.trBase, end );

	// Compute vector perpindicular to view to ent
	VectorSubtract( end, start, forward );
	VectorNormalizeFast( forward );
	VectorSet( up, 0, 0, 1 );
	CrossProduct( forward, up, right );
	VectorNormalizeFast( right );
	VectorScale( right, 10, right2 );
	VectorScale( right, 18, right );

	// Set viewheight
	if ( ent->animMovetype ) {
		view_height = 16;
	} else {
		view_height = 40;
	}

	// First, viewpoint to viewpoint
	end[2] += view_height;
	CM_BoxTraceQ3( &tr, start, end, NULL, NULL, 0, BSP46CONTENTS_SOLID, qfalse );
	if ( tr.fraction == 1.f ) {
		return qtrue;
	}

	// First-b, viewpoint to top of head
	end[2] += 16;
	CM_BoxTraceQ3( &tr, start, end, NULL, NULL, 0, BSP46CONTENTS_SOLID, qfalse );
	if ( tr.fraction == 1.f ) {
		return qtrue;
	}
	end[2] -= 16;

	// Second, viewpoint to ent's origin
	end[2] -= view_height;
	CM_BoxTraceQ3( &tr, start, end, NULL, NULL, 0, BSP46CONTENTS_SOLID, qfalse );
	if ( tr.fraction == 1.f ) {
		return qtrue;
	}

	// Third, to ent's right knee
	VectorAdd( end, right, temp );
	temp[2] += 8;
	CM_BoxTraceQ3( &tr, start, temp, NULL, NULL, 0, BSP46CONTENTS_SOLID, qfalse );
	if ( tr.fraction == 1.f ) {
		return qtrue;
	}

	// Fourth, to ent's right shoulder
	VectorAdd( end, right2, temp );
	if ( ent->animMovetype ) {
		temp[2] += 28;
	} else {
		temp[2] += 52;
	}
	CM_BoxTraceQ3( &tr, start, temp, NULL, NULL, 0, BSP46CONTENTS_SOLID, qfalse );
	if ( tr.fraction == 1.f ) {
		return qtrue;
	}

	// Fifth, to ent's left knee
	VectorScale( right, -1, right );
	VectorScale( right2, -1, right2 );
	VectorAdd( end, right2, temp );
	temp[2] += 2;
	CM_BoxTraceQ3( &tr, start, temp, NULL, NULL, 0, BSP46CONTENTS_SOLID, qfalse );
	if ( tr.fraction == 1.f ) {
		return qtrue;
	}

	// Sixth, to ent's left shoulder
	VectorAdd( end, right, temp );
	if ( ent->animMovetype ) {
		temp[2] += 16;
	} else {
		temp[2] += 36;
	}
	CM_BoxTraceQ3( &tr, start, temp, NULL, NULL, 0, BSP46CONTENTS_SOLID, qfalse );
	if ( tr.fraction == 1.f ) {
		return qtrue;
	}

	return qfalse;
}

#endif

/*
==================
CL_DeltaEntity

Parses deltas from the given base and adds the resulting entity
to the current frame
==================
*/
void CL_DeltaEntity( QMsg *msg, clSnapshot_t *frame, int newnum, entityState_t *old,
					 qboolean unchanged ) {
	entityState_t   *state;

	// save the parsed entity state into the big circular buffer so
	// it can be used as the source for a later delta
	state = &cl.parseEntities[cl.parseEntitiesNum & ( MAX_PARSE_ENTITIES - 1 )];

	if ( unchanged ) {
		*state = *old;
	} else {
		MSG_ReadDeltaEntity( msg, old, state, newnum );
	}

	if ( state->number == ( MAX_GENTITIES - 1 ) ) {
		return;     // entity was delta removed
	}

#if 1
	// DHM - Nerve :: Only draw clients if visible
	if ( clc.onlyVisibleClients ) {
		if ( state->number < MAX_CLIENTS ) {
			if ( isEntVisible( state ) ) {
				entLastVisible[state->number] = frame->serverTime;
				state->eFlags &= ~EF_NODRAW;
			} else {
				if ( entLastVisible[state->number] < ( frame->serverTime - 600 ) ) {
					state->eFlags |= EF_NODRAW;
				}
			}
		}
	}
#endif

	cl.parseEntitiesNum++;
	frame->numEntities++;
}

/*
==================
CL_ParsePacketEntities

==================
*/
void CL_ParsePacketEntities( QMsg *msg, clSnapshot_t *oldframe, clSnapshot_t *newframe ) {
	int newnum;
	entityState_t   *oldstate;
	int oldindex, oldnum;

	newframe->parseEntitiesNum = cl.parseEntitiesNum;
	newframe->numEntities = 0;

	// delta from the entities present in oldframe
	oldindex = 0;
	oldstate = NULL;
	if ( !oldframe ) {
		oldnum = 99999;
	} else {
		if ( oldindex >= oldframe->numEntities ) {
			oldnum = 99999;
		} else {
			oldstate = &cl.parseEntities[
				( oldframe->parseEntitiesNum + oldindex ) & ( MAX_PARSE_ENTITIES - 1 )];
			oldnum = oldstate->number;
		}
	}

	while ( 1 ) {
		// read the entity index number
		newnum = MSG_ReadBits( msg, GENTITYNUM_BITS );

		if ( newnum == ( MAX_GENTITIES - 1 ) ) {
			break;
		}

		if ( msg->readcount > msg->cursize ) {
			Com_Error( ERR_DROP,"CL_ParsePacketEntities: end of message" );
		}

		while ( oldnum < newnum ) {
			// one or more entities from the old packet are unchanged
			if ( cl_shownet->integer == 3 ) {
				Com_Printf( "%3i:  unchanged: %i\n", msg->readcount, oldnum );
			}
			CL_DeltaEntity( msg, newframe, oldnum, oldstate, qtrue );

			oldindex++;

			if ( oldindex >= oldframe->numEntities ) {
				oldnum = 99999;
			} else {
				oldstate = &cl.parseEntities[
					( oldframe->parseEntitiesNum + oldindex ) & ( MAX_PARSE_ENTITIES - 1 )];
				oldnum = oldstate->number;
			}
		}
		if ( oldnum == newnum ) {
			// delta from previous state
			if ( cl_shownet->integer == 3 ) {
				Com_Printf( "%3i:  delta: %i\n", msg->readcount, newnum );
			}
			CL_DeltaEntity( msg, newframe, newnum, oldstate, qfalse );

			oldindex++;

			if ( oldindex >= oldframe->numEntities ) {
				oldnum = 99999;
			} else {
				oldstate = &cl.parseEntities[
					( oldframe->parseEntitiesNum + oldindex ) & ( MAX_PARSE_ENTITIES - 1 )];
				oldnum = oldstate->number;
			}
			continue;
		}

		if ( oldnum > newnum ) {
			// delta from baseline
			if ( cl_shownet->integer == 3 ) {
				Com_Printf( "%3i:  baseline: %i\n", msg->readcount, newnum );
			}
			CL_DeltaEntity( msg, newframe, newnum, &cl.entityBaselines[newnum], qfalse );
			continue;
		}

	}

	// any remaining entities in the old frame are copied over
	while ( oldnum != 99999 ) {
		// one or more entities from the old packet are unchanged
		if ( cl_shownet->integer == 3 ) {
			Com_Printf( "%3i:  unchanged: %i\n", msg->readcount, oldnum );
		}
		CL_DeltaEntity( msg, newframe, oldnum, oldstate, qtrue );

		oldindex++;

		if ( oldindex >= oldframe->numEntities ) {
			oldnum = 99999;
		} else {
			oldstate = &cl.parseEntities[
				( oldframe->parseEntitiesNum + oldindex ) & ( MAX_PARSE_ENTITIES - 1 )];
			oldnum = oldstate->number;
		}
	}

	if ( cl_shownuments->integer ) {
		Com_Printf( "Entities in packet: %i\n", newframe->numEntities );
	}
}


/*
================
CL_ParseSnapshot

If the snapshot is parsed properly, it will be copied to
cl.snap and saved in cl.snapshots[].  If the snapshot is invalid
for any reason, no changes to the state will be made at all.
================
*/
void CL_ParseSnapshot( QMsg *msg ) {
	int len;
	clSnapshot_t    *old;
	clSnapshot_t newSnap;
	int deltaNum;
	int oldMessageNum;
	int i, packetNum;

	// get the reliable sequence acknowledge number
	// NOTE: now sent with all server to client messages
	//clc.reliableAcknowledge = MSG_ReadLong( msg );

	// read in the new snapshot to a temporary buffer
	// we will only copy to cl.snap if it is valid
	memset( &newSnap, 0, sizeof( newSnap ) );

	// we will have read any new server commands in this
	// message before we got to svc_snapshot
	newSnap.serverCommandNum = clc.serverCommandSequence;

	newSnap.serverTime = MSG_ReadLong( msg );

	newSnap.messageNum = clc.serverMessageSequence;

	deltaNum = MSG_ReadByte( msg );
	if ( !deltaNum ) {
		newSnap.deltaNum = -1;
	} else {
		newSnap.deltaNum = newSnap.messageNum - deltaNum;
	}
	newSnap.snapFlags = MSG_ReadByte( msg );

	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message
	if ( newSnap.deltaNum <= 0 ) {
		newSnap.valid = qtrue;      // uncompressed frame
		old = NULL;
		if ( clc.demorecording ) {
			clc.demowaiting = qfalse;   // we can start recording now
//			if(cl_autorecord->integer) {
//				Cvar_Set( "g_synchronousClients", "0" );
//			}
		} else {
			if ( cl_autorecord->integer /*&& Cvar_VariableValue( "g_synchronousClients")*/ ) {
				char name[256];
				char mapname[MAX_QPATH];
				char* period;
				qtime_t time;

				Com_RealTime( &time );

				String::NCpyZ( mapname, cl.mapname, MAX_QPATH );
				for ( period = mapname; *period; period++ ) {
					if ( *period == '.' ) {
						*period = '\0';
						break;
					}
				}

				for ( period = mapname; *period; period++ ) {
					if ( *period == '/' ) {
						break;
					}
				}
				if ( *period ) {
					period++;
				}

				String::Sprintf( name, sizeof( name ), "demos/%s_%i_%i.dm_%d", period, time.tm_mday, time.tm_mon + 1, PROTOCOL_VERSION );

				CL_Record( name );
			}
		}
	} else {
		old = &cl.snapshots[newSnap.deltaNum & PACKET_MASK];
		if ( !old->valid ) {
			// should never happen
			Com_Printf( "Delta from invalid frame (not supposed to happen!).\n" );
		} else if ( old->messageNum != newSnap.deltaNum ) {
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			Com_DPrintf( "Delta frame too old.\n" );
		} else if ( cl.parseEntitiesNum - old->parseEntitiesNum > MAX_PARSE_ENTITIES - 128 ) {
			Com_DPrintf( "Delta parseEntitiesNum too old.\n" );
		} else {
			newSnap.valid = qtrue;  // valid delta parse
		}
	}

	// read areamask
	len = MSG_ReadByte( msg );

	if ( len > sizeof( newSnap.areamask ) ) {
		Com_Error( ERR_DROP,"CL_ParseSnapshot: Invalid size %d for areamask.", len );
		return;
	}

	MSG_ReadData( msg, &newSnap.areamask, len );

	// read playerinfo
	SHOWNET( msg, "playerstate" );
	if ( old ) {
		MSG_ReadDeltaPlayerstate( msg, &old->ps, &newSnap.ps );
	} else {
		MSG_ReadDeltaPlayerstate( msg, NULL, &newSnap.ps );
	}

	// read packet entities
	SHOWNET( msg, "packet entities" );
	CL_ParsePacketEntities( msg, old, &newSnap );

	// if not valid, dump the entire thing now that it has
	// been properly read
	if ( !newSnap.valid ) {
		return;
	}

	// clear the valid flags of any snapshots between the last
	// received and this one, so if there was a dropped packet
	// it won't look like something valid to delta from next
	// time we wrap around in the buffer
	oldMessageNum = cl.snap.messageNum + 1;

	if ( newSnap.messageNum - oldMessageNum >= PACKET_BACKUP ) {
		oldMessageNum = newSnap.messageNum - ( PACKET_BACKUP - 1 );
	}
	for ( ; oldMessageNum < newSnap.messageNum ; oldMessageNum++ ) {
		cl.snapshots[oldMessageNum & PACKET_MASK].valid = qfalse;
	}

	// copy to the current good spot
	cl.snap = newSnap;
	cl.snap.ping = 999;
	// calculate ping time
	for ( i = 0 ; i < PACKET_BACKUP ; i++ ) {
		packetNum = ( clc.netchan.outgoingSequence - 1 - i ) & PACKET_MASK;
		if ( cl.snap.ps.commandTime >= cl.outPackets[ packetNum ].p_serverTime ) {
			cl.snap.ping = cls.realtime - cl.outPackets[ packetNum ].p_realtime;
			break;
		}
	}
	// save the frame off in the backup array for later delta comparisons
	cl.snapshots[cl.snap.messageNum & PACKET_MASK] = cl.snap;

	if ( cl_shownet->integer == 3 ) {
		Com_Printf( "   snapshot:%i  delta:%i  ping:%i\n", cl.snap.messageNum,
					cl.snap.deltaNum, cl.snap.ping );
	}

	cl.newSnapshots = qtrue;
}


//=====================================================================

int cl_connectedToPureServer;

/*
==================
CL_SystemInfoChanged

The systeminfo configstring has been changed, so parse
new information out of it.  This will happen at every
gamestate, and possibly during gameplay.
==================
*/
void CL_PurgeCache( void );
void CL_SystemInfoChanged( void ) {
	char            *systemInfo;
	const char      *s, *t;
	char key[BIG_INFO_KEY];
	char value[BIG_INFO_VALUE];

	systemInfo = cl.gameState.stringData + cl.gameState.stringOffsets[ CS_SYSTEMINFO ];
	// NOTE TTimo:
	// when the serverId changes, any further messages we send to the server will use this new serverId
	// show_bug.cgi?id=475
	// in some cases, outdated cp commands might get sent with this news serverId
	cl.serverId = String::Atoi( Info_ValueForKey( systemInfo, "sv_serverid" ) );

	memset( &entLastVisible, 0, sizeof( entLastVisible ) );

	// don't set any vars when playing a demo
	if ( clc.demoplaying ) {
		return;
	}

	s = Info_ValueForKey( systemInfo, "sv_cheats" );
	if ( String::Atoi( s ) == 0 ) {
		Cvar_SetCheatState();
	}

	// check pure server string
	s = Info_ValueForKey( systemInfo, "sv_paks" );
	t = Info_ValueForKey( systemInfo, "sv_pakNames" );
	FS_PureServerSetLoadedPaks( s, t );

	s = Info_ValueForKey( systemInfo, "sv_referencedPaks" );
	t = Info_ValueForKey( systemInfo, "sv_referencedPakNames" );
	FS_PureServerSetReferencedPaks( s, t );

	// scan through all the variables in the systeminfo and locally set cvars to match
	s = systemInfo;
	while ( s ) {
		Info_NextPair( &s, key, value );
		if ( !key[0] ) {
			break;
		}

		Cvar_Set( key, value );
	}

	// Arnout: big hack to clear the image cache on a pure change
	//cl_connectedToPureServer = Cvar_VariableValue( "sv_pure" );
	if ( Cvar_VariableValue( "sv_pure" ) ) {
		if ( !cl_connectedToPureServer && cls.state <= CA_CONNECTED ) {
			CL_PurgeCache();
		}
		cl_connectedToPureServer = qtrue;
	} else {
		if ( cl_connectedToPureServer && cls.state <= CA_CONNECTED ) {
			CL_PurgeCache();
		}
		cl_connectedToPureServer = qfalse;
	}
}

/*
==================
CL_ParseGamestate
==================
*/
void CL_ParseGamestate( QMsg *msg ) {
	int i;
	entityState_t   *es;
	int newnum;
	entityState_t nullstate;
	int cmd;
	char            *s;

	Con_Close();

	clc.connectPacketCount = 0;

	// wipe local client state
	CL_ClearState();

	// a gamestate always marks a server command sequence
	clc.serverCommandSequence = MSG_ReadLong( msg );

	// parse all the configstrings and baselines
	cl.gameState.dataCount = 1; // leave a 0 at the beginning for uninitialized configstrings
	while ( 1 ) {
		cmd = MSG_ReadByte( msg );

		if ( cmd == svc_EOF ) {
			break;
		}

		if ( cmd == svc_configstring ) {
			int len;

			i = MSG_ReadShort( msg );
			if ( i < 0 || i >= MAX_CONFIGSTRINGS ) {
				Com_Error( ERR_DROP, "configstring > MAX_CONFIGSTRINGS" );
			}
			s = MSG_ReadBigString( msg );
			len = String::Length( s );

			if ( len + 1 + cl.gameState.dataCount > MAX_GAMESTATE_CHARS ) {
				Com_Error( ERR_DROP, "MAX_GAMESTATE_CHARS exceeded" );
			}

			// append it to the gameState string buffer
			cl.gameState.stringOffsets[ i ] = cl.gameState.dataCount;
			memcpy( cl.gameState.stringData + cl.gameState.dataCount, s, len + 1 );
			cl.gameState.dataCount += len + 1;
		} else if ( cmd == svc_baseline ) {
			newnum = MSG_ReadBits( msg, GENTITYNUM_BITS );
			if ( newnum < 0 || newnum >= MAX_GENTITIES ) {
				Com_Error( ERR_DROP, "Baseline number out of range: %i", newnum );
			}
			memset( &nullstate, 0, sizeof( nullstate ) );
			es = &cl.entityBaselines[ newnum ];
			MSG_ReadDeltaEntity( msg, &nullstate, es, newnum );
		} else {
			Com_Error( ERR_DROP, "CL_ParseGamestate: bad command byte" );
		}
	}

	clc.clientNum = MSG_ReadLong( msg );
	// read the checksum feed
	clc.checksumFeed = MSG_ReadLong( msg );

	// parse serverId and other cvars
	CL_SystemInfoChanged();

	// Arnout: verify if we have all official pakfiles. As we won't
	// be downloading them, we should be kicked for not having them.
	if ( cl_connectedToPureServer && !FS_VerifyOfficialPaks() ) {
		Com_Error( ERR_DROP, "Couldn't load an official pak file; verify your installation and make sure it has been updated to the latest version." );
	}

	// reinitialize the filesystem if the game directory has changed
	FS_ConditionalRestart( clc.checksumFeed );

	// This used to call CL_StartHunkUsers, but now we enter the download state before loading the
	// cgame
	CL_InitDownloads();

	// make sure the game starts
	Cvar_Set( "cl_paused", "0" );
}


//=====================================================================

/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
void CL_ParseDownload( QMsg *msg ) {
	int size;
	unsigned char data[MAX_MSGLEN];
	int block;

	if ( !*cls.downloadTempName ) {
		Com_Printf( "Server sending download, but no download was requested\n" );
		CL_AddReliableCommand( "stopdl" );
		return;
	}

	// read the data
	block = MSG_ReadShort( msg );

	// TTimo - www dl
	// if we haven't acked the download redirect yet
	if ( block == -1 ) {
		if ( !clc.bWWWDl ) {
			// server is sending us a www download
			String::NCpyZ( cls.originalDownloadName, cls.downloadName, sizeof( cls.originalDownloadName ) );
			String::NCpyZ( cls.downloadName, MSG_ReadString( msg ), sizeof( cls.downloadName ) );
			clc.downloadSize = MSG_ReadLong( msg );
			clc.downloadFlags = MSG_ReadLong( msg );
			if ( clc.downloadFlags & ( 1 << DL_FLAG_URL ) ) {
				Sys_OpenURL( cls.downloadName, qtrue );
				Cbuf_ExecuteText( EXEC_APPEND, "quit\n" );
				CL_AddReliableCommand( "wwwdl bbl8r" ); // not sure if that's the right msg
				clc.bWWWDlAborting = qtrue;
				return;
			}
			Cvar_SetValue( "cl_downloadSize", clc.downloadSize );
			Com_DPrintf( "Server redirected download: %s\n", cls.downloadName );
			clc.bWWWDl = qtrue; // activate wwwdl client loop
			CL_AddReliableCommand( "wwwdl ack" );
			// make sure the server is not trying to redirect us again on a bad checksum
			if ( strstr( clc.badChecksumList, va( "@%s", cls.originalDownloadName ) ) ) {
				Com_Printf( "refusing redirect to %s by server (bad checksum)\n", cls.downloadName );
				CL_AddReliableCommand( "wwwdl fail" );
				clc.bWWWDlAborting = qtrue;
				return;
			}
			// make downloadTempName an OS path
			String::NCpyZ( cls.downloadTempName, FS_BuildOSPath( Cvar_VariableString( "fs_homepath" ), cls.downloadTempName, "" ), sizeof( cls.downloadTempName ) );
			cls.downloadTempName[String::Length( cls.downloadTempName ) - 1] = '\0';
			if ( !DL_BeginDownload( cls.downloadTempName, cls.downloadName, com_developer->integer ) ) {
				// setting bWWWDl to false after sending the wwwdl fail doesn't work
				// not sure why, but I suspect we have to eat all remaining block -1 that the server has sent us
				// still leave a flag so that CL_WWWDownload is inactive
				// we count on server sending us a gamestate to start up clean again
				CL_AddReliableCommand( "wwwdl fail" );
				clc.bWWWDlAborting = qtrue;
				Com_Printf( "Failed to initialize download for '%s'\n", cls.downloadName );
			}
			// Check for a disconnected download
			// we'll let the server disconnect us when it gets the bbl8r message
			if ( clc.downloadFlags & ( 1 << DL_FLAG_DISCON ) ) {
				CL_AddReliableCommand( "wwwdl bbl8r" );
				cls.bWWWDlDisconnected = qtrue;
			}
			return;
		} else
		{
			// server keeps sending that message till we ack it, eat and ignore
			//MSG_ReadLong( msg );
			MSG_ReadString( msg );
			MSG_ReadLong( msg );
			MSG_ReadLong( msg );
			return;
		}
	}

	if ( !block ) {
		// block zero is special, contains file size
		clc.downloadSize = MSG_ReadLong( msg );

		Cvar_SetValue( "cl_downloadSize", clc.downloadSize );

		if ( clc.downloadSize < 0 ) {
			Com_Error( ERR_DROP, MSG_ReadString( msg ) );
			return;
		}
	}

	size = MSG_ReadShort( msg );
	if ( size < 0 || size > sizeof( data ) ) {
		Com_Error( ERR_DROP, "CL_ParseDownload: Invalid size %d for download chunk.", size );
		return;
	}

	MSG_ReadData( msg, data, size );

	if ( clc.downloadBlock != block ) {
		Com_DPrintf( "CL_ParseDownload: Expected block %d, got %d\n", clc.downloadBlock, block );
		return;
	}

	// open the file if not opened yet
	if ( !clc.download ) {
		clc.download = FS_SV_FOpenFileWrite( cls.downloadTempName );

		if ( !clc.download ) {
			Com_Printf( "Could not create %s\n", cls.downloadTempName );
			CL_AddReliableCommand( "stopdl" );
			CL_NextDownload();
			return;
		}
	}

	if ( size ) {
		FS_Write( data, size, clc.download );
	}

	CL_AddReliableCommand( va( "nextdl %d", clc.downloadBlock ) );
	clc.downloadBlock++;

	clc.downloadCount += size;

	// So UI gets access to it
	Cvar_SetValue( "cl_downloadCount", clc.downloadCount );

	if ( !size ) { // A zero length block means EOF
		if ( clc.download ) {
			FS_FCloseFile( clc.download );
			clc.download = 0;

			// rename the file
			FS_SV_Rename( cls.downloadTempName, cls.downloadName );
		}
		*cls.downloadTempName = *cls.downloadName = 0;
		Cvar_Set( "cl_downloadName", "" );

		// send intentions now
		// We need this because without it, we would hold the last nextdl and then start
		// loading right away.  If we take a while to load, the server is happily trying
		// to send us that last block over and over.
		// Write it twice to help make sure we acknowledge the download
		CL_WritePacket();
		CL_WritePacket();

		// get another file if needed
		CL_NextDownload();
	}
}

/*
=====================
CL_ParseCommandString

Command strings are just saved off until cgame asks for them
when it transitions a snapshot
=====================
*/
void CL_ParseCommandString( QMsg *msg ) {
	char    *s;
	int seq;
	int index;

	seq = MSG_ReadLong( msg );
	s = MSG_ReadString( msg );

	// see if we have already executed stored it off
	if ( clc.serverCommandSequence >= seq ) {
		return;
	}
	clc.serverCommandSequence = seq;

	index = seq & ( MAX_RELIABLE_COMMANDS - 1 );
	String::NCpyZ( clc.serverCommands[ index ], s, sizeof( clc.serverCommands[ index ] ) );
}

/*
=====================
CL_ParseBinaryMessage
=====================
*/
void CL_ParseBinaryMessage( QMsg *msg ) {
	int size;

	MSG_BeginReadingUncompressed( msg );

	size = msg->cursize - msg->readcount;
	if ( size <= 0 || size > MAX_BINARY_MESSAGE ) {
		return;
	}

	CL_CGameBinaryMessageReceived( (char*)&msg->_data[msg->readcount], size, cl.snap.serverTime );
}

/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage( QMsg *msg ) {
	int cmd;
	QMsg msgback;

	msgback = *msg;

	if ( cl_shownet->integer == 1 ) {
		Com_Printf( "%i ",msg->cursize );
	} else if ( cl_shownet->integer >= 2 ) {
		Com_Printf( "------------------\n" );
	}

	MSG_Bitstream( msg );

	// get the reliable sequence acknowledge number
	clc.reliableAcknowledge = MSG_ReadLong( msg );
	//
	if ( clc.reliableAcknowledge < clc.reliableSequence - MAX_RELIABLE_COMMANDS ) {
		clc.reliableAcknowledge = clc.reliableSequence;
	}

	//
	// parse the message
	//
	while ( 1 ) {
		if ( msg->readcount > msg->cursize ) {
			Com_Error( ERR_DROP,"CL_ParseServerMessage: read past end of server message" );
			break;
		}

		cmd = MSG_ReadByte( msg );

		if ( cmd == svc_EOF ) {
			SHOWNET( msg, "END OF MESSAGE" );
			break;
		}

		if ( cl_shownet->integer >= 2 ) {
			if ( !svc_strings[cmd] ) {
				Com_Printf( "%3i:BAD CMD %i\n", msg->readcount - 1, cmd );
			} else {
				SHOWNET( msg, svc_strings[cmd] );
			}
		}

		// other commands
		switch ( cmd ) {
		default:
			Com_Error( ERR_DROP,"CL_ParseServerMessage: Illegible server message %d\n", cmd );
			break;
		case svc_nop:
			break;
		case svc_serverCommand:
			CL_ParseCommandString( msg );
			break;
		case svc_gamestate:
			CL_ParseGamestate( msg );
			break;
		case svc_snapshot:
			CL_ParseSnapshot( msg );
			break;
		case svc_download:
			CL_ParseDownload( msg );
			break;
		}
	}

	CL_ParseBinaryMessage( msg );
}
